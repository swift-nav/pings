/*
 * edc.h
 *
 *  Created on: Jun 15, 2018
 *      Author: miche
 */

#ifndef SRC_EDC_H_
#define SRC_EDC_H_

#include "lsn_common.h"

u32 crc24q(const u8 *buf, u32 len, u32 crc);
u32 crc24q_bits(u32 crc, const u8 *buf, u32 n_bits, bool invert);
u16 crc16_ccitt(const u8 *buf, u32 len, u16 crc);

#endif /* SRC_EDC_H_ */
