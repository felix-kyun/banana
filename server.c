/*
    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/shl
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob
 */

#include "server.h"
#include "shared.h"
#include <arpa/inet.h>
#include <linux/input.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
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

    // create device here

    puts("Relaying events. Ctrl+C to stop.");
    struct input_event ev;
    while (read_all(client_fd, &ev, sizeof(ev)) > 0) {
        printf("Event: type=%u, code=%u, value=%u\n", ev.type, ev.code, ev.value);
    }

    puts("Client disconnected");
    close(client_fd);
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
