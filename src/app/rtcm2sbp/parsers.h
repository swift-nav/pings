/*
 * parsers.h
 *
 *  Created on: Jun 15, 2018
 *      Author: miche
 */

#ifndef PARSERS_H_
#define PARSERS_H_

#include "lsn_common.h"

#define NUM_RTCM_MSG (1 << 12)

typedef enum {
  NAVSTAR = 0,
  GLONASS = 1,
  SBAS = 2,
  BEIDOU = 3,
  GALILEO = 4,
  QZSS = 5,
  NUM_CONSTELL
} constell_t;

typedef char id_map_t[33][3];

typedef void (*rtcm_cb)(uint8_t *buff, u16 size);

/* GPS MSM */
void msm_1071(u8 *buff, u16 size);
void msm_1072(u8 *buff, u16 size);
void msm_1073(u8 *buff, u16 size);
void msm_1074(u8 *buff, u16 size);
void msm_1075(u8 *buff, u16 size);
void msm_1076(u8 *buff, u16 size);
void msm_1077(u8 *buff, u16 size);

/* Glonass MSM */
void msm_1081(u8 *buff, u16 size);
void msm_1082(u8 *buff, u16 size);
void msm_1083(u8 *buff, u16 size);
void msm_1084(u8 *buff, u16 size);
void msm_1085(u8 *buff, u16 size);
void msm_1086(u8 *buff, u16 size);
void msm_1087(u8 *buff, u16 size);

/* Galileo MSM */
void msm_1091(u8 *buff, u16 size);
void msm_1092(u8 *buff, u16 size);
void msm_1093(u8 *buff, u16 size);
void msm_1094(u8 *buff, u16 size);
void msm_1095(u8 *buff, u16 size);
void msm_1096(u8 *buff, u16 size);
void msm_1097(u8 *buff, u16 size);

/* SBAS MSM */
void msm_1101(u8 *msg, u16 size);
void msm_1102(u8 *msg, u16 size);
void msm_1103(u8 *msg, u16 size);
void msm_1104(u8 *msg, u16 size);
void msm_1105(u8 *msg, u16 size);
void msm_1106(u8 *msg, u16 size);
void msm_1107(u8 *msg, u16 size);

/* QZSS MSM */
void msm_1111(u8 *msg, u16 size);
void msm_1112(u8 *msg, u16 size);
void msm_1113(u8 *msg, u16 size);
void msm_1114(u8 *msg, u16 size);
void msm_1115(u8 *msg, u16 size);
void msm_1116(u8 *msg, u16 size);
void msm_1117(u8 *msg, u16 size);

/* Beidou MSM */
void msm_1121(u8 *msg, u16 size);
void msm_1122(u8 *msg, u16 size);
void msm_1123(u8 *msg, u16 size);
void msm_1124(u8 *msg, u16 size);
void msm_1125(u8 *msg, u16 size);
void msm_1126(u8 *msg, u16 size);
void msm_1127(u8 *msg, u16 size);

void rtcm_parse(u8 *msg, u16 size);

#endif /* PARSERS_H_ */
