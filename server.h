/*
    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/banana
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob
 */

#include <sys/types.h>
#include "shared.h"

ssize_t read_all(int fd, void* buf, size_t len);
int create_virtual_device(header_t header);
