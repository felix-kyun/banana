/*
    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/banana
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob
 */

#include <stdint.h>
#include <linux/input.h>

#ifndef SHARED_H
#define SHARED_H

// default port that server runs on
#define PORT 54321

// initial header
typedef struct header_t {
	char name[256];
	struct input_id id;

	uint8_t ev_bits[EV_MAX / 8 + 1];
	uint8_t key[KEY_MAX / 8 + 1];
	uint8_t rel[REL_MAX / 8 + 1];
	uint8_t abs[ABS_MAX / 8 + 1];
	struct input_absinfo abs_info[ABS_MAX];
} header_t;

#endif
