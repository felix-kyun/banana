/*
    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/banana
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob
 */
#define _GNU_SOURCE
#include "server.h"
#include "shared.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int
main(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = INADDR_ANY };
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    listen(fd, 1);
    fprintf(stderr, "Listening on port %d\n", PORT);

    struct sockaddr_in client_addr;
    int                client_fd = accept(fd, (struct sockaddr*)&client_addr, &(socklen_t) { sizeof(client_addr) });
    if (client_fd < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    header_t header;
    if (read_all(client_fd, &header, sizeof(header)) == -1) {
        perror("read");
        fputs("Failed to read header\n", stderr);
        exit(EXIT_FAILURE);
    }
    puts("Header received");
    printf("Name: %s\n", header.name);
    printf("ID: %x:%x\n", header.id.vendor, header.id.product);

    int device_fd = create_virtual_device(header);
    if (device_fd == -1) {
        exit(EXIT_FAILURE);
    }
    usleep(100000); // 100ms

    puts("Relaying events. Ctrl+C to stop.");
    struct input_event ev;
    while (read_all(client_fd, &ev, sizeof(ev)) > 0) {
        printf("Event: type=%u, code=%u, value=%u\n", ev.type, ev.code, ev.value);
        if (write(device_fd, &ev, sizeof(ev)) == -1) {
            perror("write");
            break;
        }
    }

    puts("Client disconnected");
    close(client_fd);
    ioctl(device_fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}

ssize_t
read_all(int fd, void* buf, size_t len)
{
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, ((char*)buf) + got, len - got);
        if (n <= 0) {
            return -1;
        }
        got += (size_t)n;
    }

    return (ssize_t)got;
}

int
create_virtual_device(header_t header)
{
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        fputs("Hint: modprobe uinput or fix permissions", stderr);
        return -1;
    }

    // enable events
    for (int ev = 0; ev < EV_MAX; ev++) {
        if (!(header.ev_bits[ev / 8] & (1 << (ev % 8)))) {
            continue;
        }
        ioctl(fd, UI_SET_EVBIT, ev);

        if (ev == EV_KEY) {
            for (int k = 0; k < KEY_MAX; k++) {
                if (header.key[k / 8] & (1 << (k % 8))) {
                    ioctl(fd, UI_SET_KEYBIT, k);
                }
            }
        }

        if (ev == EV_REL) {
            for (int r = 0; r < REL_MAX; r++) {
                if (header.rel[r / 8] & (1 << (r % 8))) {
                    ioctl(fd, UI_SET_RELBIT, r);
                }
            }
        }

        if (ev == EV_ABS) {
            for (int a = 0; a < ABS_MAX; a++) {
                if (header.abs[a / 8] & (1 << (a % 8))) {
                    ioctl(fd, UI_SET_ABSBIT, a);
                    struct uinput_abs_setup abs_setup = { .code = (uint16_t)a, .absinfo = header.abs_info[a] };
                    ioctl(fd, UI_ABS_SETUP, &abs_setup);
                }
            }
        }
    }

    struct uinput_setup setup = { .id = header.id };
    strncpy(setup.name, header.name, UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(fd, UI_DEV_SETUP, &setup) == -1) {
        perror("UI_DEV_SETUP");
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_DEV_CREATE) == -1) {
        perror("UI_DEV_CREATE");
        close(fd);
        return -1;
    }

    printf("Virtual device created: %s\n", header.name);

    return fd;
}
