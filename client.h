/*
    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/banana
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob
 */

#include <arpa/inet.h>
#include <stddef.h>
#include "shared.h"

int open_device(char* device);
struct sockaddr_in create_server_address(char* ip, uint16_t port);
header_t create_device_header(int fd);

ssize_t write_all(int fd, const void* buf, size_t len);

void enable_raw_mode();
void disable_raw_mode();
void show_usage(char* name);
