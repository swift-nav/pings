#include <config.h>

#include <getopt.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>

#include <curl/curl.h>

char *url = "http://swiftnav:demo@ntrip.swiftnav.com:2102/RTCM3_SWFT";
char *lat = "37.77101999622968";
char *lon = "-122.40315159140708";
char *height = "-5.549358852471994";

void usage(char *command)
{
  printf("Usage: %s\n", command);
  puts("\nMain options");
  puts("\t--url    <url>");
  puts("\t--lat    <latitude>");
  puts("\t--lon    <longitude>");
  puts("\t--height <height>");
}

int parse_options(int argc, char *argv[])
{
  enum {
    OPT_URL = 1,
    OPT_LAT,
    OPT_LON,
    OPT_HEIGHT,
  };

  struct option long_opts[] = {
    {"url",    required_argument, NULL, OPT_URL},
    {"lat",    required_argument, NULL, OPT_LAT},
    {"lon",    required_argument, NULL, OPT_LON},
    {"height", required_argument, NULL, OPT_HEIGHT},
    {NULL,     0,                 NULL, 0},
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
    switch (opt) {
      case OPT_URL: {
        url = optarg;
      }
      break;
      case OPT_LAT: {
        lat = optarg;
      }
      break;
      case OPT_LON: {
        lon = optarg;
      }
      break;
      case OPT_HEIGHT: {
        height = optarg;
      }
      break;
      default: {
        return -1;
      }
      break;
    }
  }

  return 0;
}

size_t download(char *buf, size_t size, size_t n, void *data)
{
  size_t ret = fwrite(buf, size, n, stdout);
  fflush(stdout);
  return ret;
}

void checksum(char *s) {
  uint8_t sum = 0;
  char *p = s;

  p++;
  while (*p) {
    sum ^= *p;
    p++;
  }

  sprintf(p, "*%02X\r\n", sum);
}

time_t last;

size_t upload(char *buf, size_t size, size_t n, void *data)
{
  time_t now = time(NULL);
  if (now - last > 10) {
    last = now;
    double latf = atof(lat);
    double lonf = atof(lon);
    double heightf = atof(height);

    double latn = fabs(round(latf * 1e8) / 1e8);
    double lonn = fabs(round(lonf * 1e8) / 1e8);

    uint16_t lat_deg = (uint16_t)latn;
    uint16_t lon_deg = (uint16_t)lonn;

    double lat_min = (latn - (double)lat_deg) * 60.0;
    double lon_min = (lonn - (double)lon_deg) * 60.0;

    char lat_dir = latf < 0.0 ? 'S' : 'N';
    char lon_dir = lonf < 0.0 ? 'W' : 'E';

    char gpgga_buf[1024];
    sprintf(gpgga_buf, "$GPGGA,200544.70,%02u%010.7f,%c,%03u%010.7f,%c,4,12,1.3,%.2f,M,0.0,M,1.7,0078",
            lat_deg, lat_min, lat_dir, lon_deg, lon_min, lon_dir, heightf);

    checksum(gpgga_buf);

    size_t ret = sprintf(buf, "%s", gpgga_buf);
    return ret;
  }

  return CURL_READFUNC_PAUSE;
}

CURL *curl;

int progress(void *data, curl_off_t dltot, curl_off_t dlnow, curl_off_t ultot, curl_off_t ulnow)
{
  time_t now = time(NULL);
  if (now - last > 10) {
    curl_easy_pause(curl, CURLPAUSE_CONT);
  }

  return 0;
}

int request(void)
{
  CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
  if (code != CURLE_OK) {
    puts("Bad global init");
    return -1;
  }

  curl = curl_easy_init();
  if (curl == NULL) {
    puts("Bad easy init");
    return -1;
  }

  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, "Transfer-Encoding:");
  chunk = curl_slist_append(chunk, "Ntrip-Version: Ntrip/2.0");

  char error_buf[CURL_ERROR_SIZE];
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER,       chunk);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,      error_buf);
  curl_easy_setopt(curl, CURLOPT_USERAGENT,        "NTRIP ntrip-client/1.0");
  curl_easy_setopt(curl, CURLOPT_URL,              url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,    download);
  curl_easy_setopt(curl, CURLOPT_READFUNCTION,     upload);
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0L);
  curl_easy_setopt(curl, CURLOPT_PUT,              1L);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,    "GET");

  code = curl_easy_perform(curl);
  if (code != CURLE_OK) {
    printf("Bad easy perform %s\n", error_buf);
    return -1;
  }

  return 0;
}

int main (int argc, char *argv[])
{
  if (parse_options(argc, argv) != 0) {
    usage(argv[0]);

    exit(EXIT_FAILURE);
  }

  if (request() != 0) {
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
