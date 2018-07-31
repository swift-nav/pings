/*
 * parsers.c

 *
 *  Created on: Jun 15, 2018
 *      Author: miche
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "parsers.h"

static id_map_t GPS_RTCM_DF395 = {[2] = "1C",  [3] = "1P",  [4] = "1W",
                                  [8] = "2C",  [9] = "2P",  [10] = "2W",
                                  [15] = "2S", [16] = "2L", [17] = "2X",
                                  [22] = "5I", [23] = "5Q", [24] = "5X",
                                  [30] = "1S", [31] = "1L", [32] = "1X"};

static id_map_t GLO_RTCM_DF395 = {[2] = "1C", [3] = "1P", [8] = "2C",
                                  [9] = "2P"};

static id_map_t BDS_RTCM_DF395 = {[2] = "2I",  [3] = "2Q",  [4] = "2X",
                                  [8] = "6I",  [9] = "6Q",  [10] = "6X",
                                  [14] = "7I", [15] = "7Q", [16] = "7X"};

static id_map_t GAL_RTCM_DF395 = {[2] = "1C",  [3] = "1A",  [4] = "1B",
                                  [5] = "1X",  [6] = "1Z",  [8] = "6C",
                                  [9] = "6A",  [10] = "6B", [11] = "6X",
                                  [12] = "6Z", [14] = "7I", [15] = "7Q",
                                  [16] = "7X", [18] = "8I", [19] = "8Q",
                                  [20] = "8X", [22] = "5I", [23] = "5Q",
                                  [24] = "5X"};

static id_map_t SBS_RTCM_DF395 = {[2] = "1C", [22] = "5I", [23] = "5Q",
                                  [24] = "5X"};

static id_map_t QZS_RTCM_DF395 = {[2] = "1C",  [9] = "6S",  [10] = "6L",
                                  [11] = "6X", [15] = "2S", [16] = "2L",
                                  [17] = "2X", [22] = "5I", [23] = "5Q",
                                  [24] = "5X", [30] = "1S", [31] = "1L",
                                  [32] = "1X"};

static char CONSTELL_ID[NUM_CONSTELL] = {[NAVSTAR] = 'G', [GLONASS] = 'R',
                                         [SBAS] = 'S',    [BEIDOU] = 'C',
                                         [GALILEO] = 'E', [QZSS] = 'J'};

static id_map_t *RTCM_DF395[NUM_CONSTELL] = {[NAVSTAR] = &GPS_RTCM_DF395,
                                             [GLONASS] = &GLO_RTCM_DF395,
                                             [SBAS] = &SBS_RTCM_DF395,
                                             [BEIDOU] = &BDS_RTCM_DF395,
                                             [GALILEO] = &GAL_RTCM_DF395,
                                             [QZSS] = &QZS_RTCM_DF395};

static rtcm_cb parser[NUM_RTCM_MSG] = {
        /* GPS MSM */
        [1071] = msm_1071, [1072] = msm_1072, [1073] = msm_1073,
        [1074] = msm_1074, [1075] = msm_1075, [1076] = msm_1076,
        [1077] = msm_1077,
        /* Glonass MSM */
        [1081] = msm_1081, [1082] = msm_1082, [1083] = msm_1083,
        [1084] = msm_1084, [1085] = msm_1085, [1086] = msm_1086,
        [1087] = msm_1087,
        /* Galileo MSM */
        [1091] = msm_1091, [1092] = msm_1092, [1093] = msm_1093,
        [1094] = msm_1094, [1095] = msm_1095, [1096] = msm_1096,
        [1097] = msm_1097,
        /* SBAS MSM */
        [1101] = msm_1101, [1102] = msm_1102, [1103] = msm_1103,
        [1104] = msm_1104, [1105] = msm_1105, [1106] = msm_1106,
        [1107] = msm_1107,
        /* QZSS MSM */
        [1111] = msm_1111, [1112] = msm_1112, [1113] = msm_1113,
        [1114] = msm_1114, [1115] = msm_1115, [1116] = msm_1116,
        [1117] = msm_1117,
        /* Beidou MSM */
        [1121] = msm_1121, [1122] = msm_1122, [1123] = msm_1123,
        [1124] = msm_1124, [1125] = msm_1125, [1126] = msm_1126,
        [1127] = msm_1127};

static u64 getbitu(const u8 *buff, u32 pos, u8 len) {
  u64 bits = 0;
  for (u32 i = pos; i < pos + len; i++) {
    bits = (bits << 1) + ((buff[i / 8] >> (7 - i % 8)) & 1u);
  }
  return bits;
}
static const u8 bitn[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
static u8 count_bits_u64(u64 v, u8 bv) {
  u8 r = 0;
  for (int i = 0; i < 16; i++)
    r += bitn[(v >> (i * 4)) & 0xf];
  return bv == 1 ? r : 64 - r;
}

static void msm_header(uint8_t *msg, u16 size, constell_t c) {
  u16 stat_id = getbitu(msg, 24 + 12, 12);
  u32 epoch = getbitu(msg, 24 + 24, 30);
  u8 mmbit = getbitu(msg, 24 + 54, 1);
  u8 iods = getbitu(msg, 24 + 55, 3);
  //  u8 rsv1 = getbitu(msg, 24+58, 7);
  u8 clk_steering = getbitu(msg, 24 + 65, 2);
  u8 ext_clk = getbitu(msg, 24 + 67, 2);
  u8 divergence = getbitu(msg, 24 + 69, 1);
  u8 smooth_intvl = getbitu(msg, 24 + 70, 3);
  u64 sat_mask = getbitu(msg, 24 + 73, 64);
  u32 signal_mask = getbitu(msg, 24 + 137, 32);

  u8 x = count_bits_u64(sat_mask, 1) * count_bits_u64(signal_mask, 1);
  u64 cell_mask = getbitu(msg, 24 + 169, x);

  fprintf(stdout,
          "%s "
          "stat_id %4u, epoch %8u, mmbit %1u, iods %1u, "
          "clksteer %1u, ext_clk %1u, divergence %1u, smooth_intvl %1u, "
          "sat_mask %16lX, signal_mask %08X, signal_mask %16lX\n",
          __FUNCTION__, stat_id, epoch, mmbit, iods, clk_steering, ext_clk,
          divergence, smooth_intvl, sat_mask, signal_mask, cell_mask);

  fprintf(stdout, "%s sats %d, ", __FUNCTION__, count_bits_u64(sat_mask, 1));
  for (s8 k = 63; k >= 0; k--) {
    if ((sat_mask >> k) & 1) {
      fprintf(stdout, "%c%02d ", CONSTELL_ID[c], 64 - k);
    }
  }
  fprintf(stdout, "\n");
  fprintf(stdout, "%s signals %d, ", __FUNCTION__,
          count_bits_u64(signal_mask, 1));
  for (s8 k = 31; k >= 0; k--) {
    if ((signal_mask >> k) & 1) {
      fprintf(stdout, "%s ", (*RTCM_DF395[c])[32 - k]);
    }
  }
  fprintf(stdout, "\n");
}

/* GPS MSM1 */
void msm_1071(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, NAVSTAR);
}
/* GPS MSM2 */
void msm_1072(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, NAVSTAR);
}
/* GPS MSM3 */
void msm_1073(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, NAVSTAR);
}
/* GPS MSM4 */
void msm_1074(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, NAVSTAR);
}
/* GPS MSM5 */
void msm_1075(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, NAVSTAR);
}
/* GPS MSM6 */
void msm_1076(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, NAVSTAR);
}
/* GPS MSM7 */
void msm_1077(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, NAVSTAR);
}

/* Glonass MSM1 */
void msm_1081(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GLONASS);
}
/* Glonass MSM2 */
void msm_1082(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GLONASS);
}
/* Glonass MSM3 */
void msm_1083(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GLONASS);
}
/* Glonass MSM4 */
void msm_1084(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GLONASS);
}
/* Glonass MSM5 */
void msm_1085(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GLONASS);
}
/* Glonass MSM6 */
void msm_1086(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GLONASS);
}
/* Glonass MSM7 */
void msm_1087(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GLONASS);
}

/* Galileo MSM1 */
void msm_1091(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GALILEO);
}
/* Galileo MSM2 */
void msm_1092(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GALILEO);
}
/* Galileo MSM3 */
void msm_1093(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GALILEO);
}
/* Galileo MSM4 */
void msm_1094(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GALILEO);
}
/* Galileo MSM5 */
void msm_1095(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GALILEO);
}
/* Galileo MSM6 */
void msm_1096(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GALILEO);
}
/* Galileo MSM7 */
void msm_1097(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, GALILEO);
}

/* SBAS MSM1 */
void msm_1101(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, SBAS);
}
/* SBAS MSM2 */
void msm_1102(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, SBAS);
}
/* SBAS MSM3 */
void msm_1103(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, SBAS);
}
/* SBAS MSM4 */
void msm_1104(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, SBAS);
}
/* SBAS MSM5 */
void msm_1105(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, SBAS);
}
/* SBAS MSM6 */
void msm_1106(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, SBAS);
}
/* SBAS MSM7 */
void msm_1107(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, SBAS);
}

/* QZSS MSM1 */
void msm_1111(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, QZSS);
}
/* QZSS MSM2 */
void msm_1112(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, QZSS);
}
/* QZSS MSM3 */
void msm_1113(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, QZSS);
}
/* QZSS MSM4 */
void msm_1114(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, QZSS);
}
/* QZSS MSM5 */
void msm_1115(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, QZSS);
}
/* QZSS MSM6 */
void msm_1116(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, QZSS);
}
/* QZSS MSM7 */
void msm_1117(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, QZSS);
}

/* Beidou MSM1 */
void msm_1121(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, BEIDOU);
}
/* Beidou MSM2 */
void msm_1122(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, BEIDOU);
}
/* Beidou MSM3 */
void msm_1123(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, BEIDOU);
}
/* Beidou MSM4 */
void msm_1124(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, BEIDOU);
}
/* Beidou MSM5 */
void msm_1125(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, BEIDOU);
}
/* Beidou MSM6 */
void msm_1126(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, BEIDOU);
}
/* Beidou MSM7 */
void msm_1127(uint8_t *msg, u16 size) {
  if (NULL == msg || 0 == size)
    return;
  msm_header(msg, size, BEIDOU);
}

void rtcm_parse(uint8_t *msg, u16 size) {
  u16 msg_type = getbitu(msg, 24, 12);

  if (NULL != parser[msg_type]) {
    parser[msg_type](msg, size);
  }
}
