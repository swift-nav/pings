#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/rtcm3_sbp_internal.h"
#include "bits.h"

#include "lsn_common.h"
#include "lsn_edc.h"
#include "lsn_fifo.h"
#include "parsers.h"

#ifdef _WIN32
#include <windows.h>
#include "getopt.h"
#else
#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#endif

static bool stop;

#define RTCM_PREAM 0xD3
#define BUFSZ (16 * 1024)
#define READSZ (1024)

#ifdef _WIN32
BOOL WINAPI sighandler(int signum) {
  if (CTRL_C_EVENT == signum) {
    fprintf(stderr, "Signal caught, exiting!\n");
    stop = true;
    return TRUE;
  }
  return FALSE;
}
#else
void sigint_callback_handler(int signum) {
  fprintf(stderr, "Caught signal %d\n", signum);
  stop = true;
}
#endif

/**
 *
 * */
static double expected_L1CA_bias = 0.0;
static double expected_L1P_bias = 0.0;
static double expected_L2CA_bias = 0.0;
static double expected_L2P_bias = 0.0;

static sbp_gps_time_t previous_obs_time = {.tow = 0, .wn = INVALID_TIME};
static u8 previous_n_meas = 0;
static u8 previous_num_obs = 0;

static gps_time_sec_t current_time;

static struct rtcm3_sbp_state state;

static FILE *fid_i = NULL;
static FILE *fid_o = NULL;

/**
 * STATIC FUNCTIONS
 * */
static void Usage(char *argv0);
static void update_obs_time(const msg_obs_t *msg);
static void sbp_callback(u16 msg_id, u8 length, u8 *buffer, u16 sender_id);
static double sbp_diff_time(const sbp_gps_time_t *e, const sbp_gps_time_t *b);
static void my_sbp_send_message(u16 type, const u8 *buff, u8 len);

/**
 * PROGRAM ENTRY POINT
 * */
int main(int argc, char *argv[]) {
  char *fn_i = NULL;
  char *fn_o = NULL;

  s32 opt;
  while ((opt = getopt(argc, argv, "i:o:h")) != -1) {
    switch (opt) {
      case 'i':
        fn_i = optarg;
        break;
      case 'o':
        fn_o = optarg;
        break;
      case 'h':
      default:
        Usage(argv[0]);
        return 0;
        break;
    }
  }

  if (optind > argc) {
    fprintf(stderr, "Expected argument after options\n");
    return 0;
  }

  current_time.wn = 1945;
  current_time.tow = 277500;

  rtcm2sbp_init(&state, sbp_callback, NULL);
  rtcm2sbp_set_gps_time(&current_time, &state);
  rtcm2sbp_set_leap_second(18, &state);

#ifdef _WIN32
  _setmode(_fileno(stdin), O_BINARY);
#endif

  /* input stream */
  if (NULL == fn_i) {
    fid_i = stdin;
  } else {
    fid_i = fopen(fn_i, "rb");
    if (NULL == fid_i) {
      goto CLEANUP;
    }
  }

  /* output stream */
  if (NULL == fn_o) {
    fid_o = stdout;
  } else {
    fid_o = fopen(fn_o, "wb");
    if (NULL == fid_o) {
      goto CLEANUP;
    }
  }

#ifdef _WIN32
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)sighandler, TRUE);
#else
  signal(SIGINT, &sigint_callback_handler);
  signal(SIGILL, &sigint_callback_handler);
  signal(SIGFPE, &sigint_callback_handler);
  signal(SIGSEGV, &sigint_callback_handler);
  signal(SIGTERM, &sigint_callback_handler);
  signal(SIGABRT, &sigint_callback_handler);
#endif

  u8 *tmp_buff = calloc(BUFSZ, sizeof(u8));
  if (NULL == tmp_buff) {
    goto CLEANUP;
  }

  fifo_t tmp_fifo;
  fifo_init(&tmp_fifo, tmp_buff, BUFSZ);

  u8 *rtcm_buff = calloc(BUFSZ, sizeof(u8));
  if (NULL == rtcm_buff) {
    goto CLEANUP;
  }

  u8 *rtcm_msg;
  while (!stop) {
    /* reading from a pipe as file descriptor prevents us to do non-blocking
     * reads and therefore empty completely the buffer.. so the last message in
     * the buffer will be read later, as soon as READSZ buffer are available
     * reading 1 byte at a time might be very inefficient.. */
    ssize_t read_sz = fread(rtcm_buff, sizeof(u8), 1, fid_i);
    if (read_sz < 0) {
      perror("error reading file\n");
      break;
    }
    if (read_sz == 0) {
      usleep(1000);
      continue;
    }
    // fprintf(stderr, "read %u Bytes\n", read_sz);

    /* push newly read data to FIFO */
    u32 num_wr = fifo_write(&tmp_fifo, rtcm_buff, read_sz);
    if (num_wr < read_sz) {
      fprintf(stderr, "could only write %u out of %ld\n", num_wr, read_sz);
      // FIFO full???
    }

    /* peek available data from FIFO */
    u32 num_rd = fifo_peek(&tmp_fifo, rtcm_buff, BUFSZ);
    if (num_rd < 7) {
      // not enough data in the FIFO yet
      continue;
    }

    /* Parse as much as possible */
    u32 rd_pt = 0;
    while (rd_pt < (num_rd - 3)) {
      if (RTCM_PREAM != rtcm_buff[rd_pt]) {
        /* no preamble, move on */
        rd_pt++;
        fprintf(stderr, ".");
        continue;
      }

      u16 msg_len = ((u16)rtcm_buff[rd_pt + 1] << 8) | rtcm_buff[rd_pt + 2];
      msg_len &= 0x3FF;
      if ((rd_pt + 6 + msg_len) > num_rd) {
        /* not enough data in the buffer, get out here */
        break;
      }

      rtcm_msg = &rtcm_buff[rd_pt];

      u32 crc_rx = getbitu(rtcm_msg, 24 + msg_len * 8, 24);
      u32 crc_calc = crc24q(rtcm_msg, 3 + msg_len, 0);
      if (crc_rx != crc_calc) {
        fprintf(stderr, "crc_rx %6x, crc_cal %6x\n", crc_rx, crc_calc);
        rd_pt++;
        continue;
      }
      u16 msg_type = getbitu(rtcm_msg, 24, 12);
      // fprintf(fid_o, "mid %04d, len %4d\n", msg_type, msg_len);

      /* parse */
      // rtcm_parse(rtcm_msg, 6 + msg_len);
      rtcm2sbp_decode_frame(rtcm_msg, msg_len + 6, &state);

      /* mark this data as parsed and continue */
      rd_pt += (6 + msg_len);
    }

    /* removed parsed from FIFO */
    fifo_remove(&tmp_fifo, rd_pt);
    // if (rd_pt > 0) {
    //   fprintf(stderr,
    //           "removed %d, fifo_length %d\n",
    //           rd_pt,
    //           fifo_length(&tmp_fifo));
    // }
  }

CLEANUP:

  if (NULL != rtcm_buff) {
    free(rtcm_buff);
    rtcm_buff = NULL;
  }
  if (NULL != tmp_buff) {
    free(tmp_buff);
    tmp_buff = NULL;
  }
  if ((NULL != fid_i) && (stdin != fid_i)) {
    fclose(fid_i);
    fid_i = NULL;
  }
  if ((NULL != fid_o) && (stdout != fid_o)) {
    fclose(fid_o);
    fid_o = NULL;
  }

  fprintf(stderr, "main() exit clean\n");

  return 0;
}

static void Usage(char *argv0) {
  fprintf(stderr, "Usage : %s\n", argv0);
  fprintf(stderr, "  -i input (stdin if none)\n");
  fprintf(stderr, "  -o output (stdout if none)\n");
}

static void sbp_callback(u16 msg_id, u8 length, u8 *buffer, u16 sender_id) {
  (void)sender_id;
  const u32 MAX_OBS_GAP_S = 1;

  if (msg_id == SBP_MSG_OBS) {
    msg_obs_t *sbp_obs = (msg_obs_t *)buffer;

    if (previous_obs_time.wn != INVALID_TIME) {
      double dt = sbp_diff_time(&sbp_obs->header.t, &previous_obs_time);
      /* make sure time does not run backwards */
      if (dt < 0) {
        fprintf(stderr, "dt %.3f, time goes backward?\n", dt);
      }
      /* check there's an observation for every second */
      if (dt > MAX_OBS_GAP_S) {
        fprintf(stderr, "dt %.3f, too long gap?\n", dt);
      }

      u8 n_meas = sbp_obs->header.n_obs;
      u8 seq_counter = n_meas & 0x0F;
      u8 seq_size = n_meas >> 4;
      u8 prev_seq_counter = previous_n_meas & 0x0F;
      u8 prev_seq_size = previous_n_meas >> 4;

      if (seq_counter == 0) {
        /* new observation sequence, verify that the last one did complete */
        if (prev_seq_counter != prev_seq_size - 1) {
          fprintf(stderr,
                  "prev_seq_counter %d, prev_seq_size %d, did not complete?\n",
                  prev_seq_counter,
                  prev_seq_size);
        }
      } else {
        /* verify that the previous sequence continues */
        if (sbp_obs->header.t.wn != previous_obs_time.wn) {
          fprintf(stderr,
                  "this wn %d, prev wn %d\n",
                  sbp_obs->header.t.wn,
                  previous_obs_time.wn);
        }
        if (sbp_obs->header.t.tow != previous_obs_time.tow) {
          fprintf(stderr,
                  "this tow %d, prev tow %d\n",
                  sbp_obs->header.t.tow,
                  previous_obs_time.tow);
        }
        if (seq_size != prev_seq_size) {
          fprintf(stderr,
                  "seq_size %d, prev_seq_size %d\n",
                  seq_size,
                  prev_seq_size);
        }
        if (seq_counter != prev_seq_counter + 1) {
          fprintf(stderr,
                  "seq_counter %d, prev_seq_counter %d\n",
                  seq_counter,
                  prev_seq_counter);
        }
      }

      if (seq_counter < seq_size - 1) {
        /* verify that all but the last packet in the sequence are full */
        if (length != 249) {
          fprintf(stderr,
                  "seq_counter %d, seq_size %d, length %d\n",
                  seq_counter,
                  seq_size,
                  length);
        }
      } else {
        /* last message in sequence, check the total number of observations */
        u8 num_obs = (seq_size - 1) * 14 + (length - 11) / 17;
        fprintf(stderr, "received %d obs\n", num_obs);

        if (previous_num_obs > 0) {
          /* must not lose more than 5 observations between epochs */
          if (num_obs < previous_num_obs - 5) {
            fprintf(stderr,
                    "num_obs %d, previous_num_obs %d\n",
                    num_obs,
                    previous_num_obs);
          }
        }
        previous_num_obs = num_obs;
      }

      my_sbp_send_message(SBP_MSG_OBS, buffer, length);
    }
    previous_obs_time = sbp_obs->header.t;
    previous_n_meas = sbp_obs->header.n_obs;
    update_obs_time(sbp_obs);
  }
}

/* difference between two sbp time stamps */
static double sbp_diff_time(const sbp_gps_time_t *end,
                            const sbp_gps_time_t *beginning) {
  s16 week_diff = end->wn - beginning->wn;
  double dt = (double)end->tow / SECS_MS - (double)beginning->tow / SECS_MS;
  dt += week_diff * SEC_IN_WEEK;
  return dt;
}

static void update_obs_time(const msg_obs_t *msg) {
  gps_time_sec_t obs_time = {.tow = msg[0].header.t.tow * MS_TO_S,
                             .wn = msg[0].header.t.wn};
  rtcm2sbp_set_gps_time(&obs_time, &state);
}

static void my_sbp_send_message(u16 type, const u8 *buff, u8 len) {
  /* Check our payload data pointer isn't NULL unless len = 0. */
  if (len != 0 && buff == 0) return;

  u8 pream = 0x55;
  fwrite(&pream, 1, 1, fid_o);

  fwrite(&type, 1, 2, fid_o);

  u16 sender_id = 0x1234;
  fwrite(&sender_id, 1, 2, fid_o);

  fwrite(&len, 1, 1, fid_o);

  fwrite(buff, 1, len, fid_o);

  u16 crc = crc16_ccitt((u8 *)&(type), 2, 0);
  crc = crc16_ccitt((u8 *)&(sender_id), 2, crc);
  crc = crc16_ccitt(&(len), 1, crc);
  crc = crc16_ccitt(buff, len, crc);

  fwrite(&crc, 1, 2, fid_o);
}
