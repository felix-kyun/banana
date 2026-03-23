/*
    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/banana
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob
 */

#include "client.h"
#include "shared.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

int
main(int argc, char** argv)
{
    if (argc < 3) {
        show_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char* ip     = argv[1];
    char* device = argv[2];

    int                fd          = open_device(device);
    struct sockaddr_in server_addr = create_server_address(ip, PORT);
    header_t           header      = create_device_header(fd);

    printf("Device Info\n");
    printf("Name: %s\n", header.name);
    printf("ID: %x:%x\n", header.id.vendor, header.id.product);

    printf("Connecting to server %s\n", ip);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        close(fd);
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &(int) { 1 }, sizeof(int)) < 0) {
        perror("setsockopt");
        close(sock);
        close(fd);
        exit(EXIT_FAILURE);
    }
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(sock);
        close(fd);
        perror("connect");
        exit(EXIT_FAILURE);
    }
    puts("Connected");

    // send header
    if (write_all(sock, &header, sizeof(header)) < 0) {
        fputs("Failed to send header\n", stderr);
        close(sock);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // try to gain exclusive read on the device
    if (ioctl(fd, EVIOCGRAB, 1) == -1) {
        fprintf(stderr, "Failed to grab device %s\n", device);
        perror("ioctl");
        close(sock);
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Device %s is now exclusive\n", device);
    printf("Press q to release\n");

    atexit(disable_raw_mode);
    enable_raw_mode();

    char               c;
    struct input_event ev;
    while (1) {
        if (read(STDIN_FILENO, &c, 1) == 1 && c == 'q') {
            printf("Releasing exclusive mode\n");
            ioctl(fd, EVIOCGRAB, 0);
            break;
        }
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == sizeof(ev)) {
            printf("Event: type=%u, code=%u, value=%u\n", ev.type, ev.code, ev.value);
            if (write_all(sock, &ev, sizeof(ev)) == -1) {
                fputs("Failed to send event\n", stderr);
                break;
            }
        }
    }

    close(sock);
    close(fd);
    return 0;
}

struct termios original_t;
void
enable_raw_mode()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &original_t);
    t = original_t;
    t.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    t.c_cc[VMIN]  = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
}

void
disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_t);
}

void
show_usage(char* name)
{
    fprintf(stderr, "Usage: %s <ip> <device>\n", name);
    fputs("    ip: IP address of the server\n", stderr);
    fputs("    device: Path to the input device\n", stderr);
    exit(EXIT_FAILURE);
}

int
open_device(char* device)
{

    // check if the iodev exists
    if (access(device, F_OK) == -1) {
        fprintf(stderr, "Error: Device '%s' does not exist\n", device);
        exit(EXIT_FAILURE);
    }

    // check read permissions
    if (access(device, R_OK | W_OK) == -1) {
        fprintf(stderr, "Error: insufficient permissions on '%s'\n", device);
        fprintf(stderr, "Hint: try running with sudo\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(device, O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    return fd;
}

struct sockaddr_in
create_server_address(char* ip, uint16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    return addr;
}

header_t
create_device_header(int fd)
{
    // fill header for the device
    header_t header = { };
    ioctl(fd, EVIOCGNAME(sizeof header.name), header.name);
    ioctl(fd, EVIOCGID, &header.id);
    ioctl(fd, EVIOCGBIT(0, sizeof(header.ev_bits)), header.ev_bits);
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(header.key)), header.key);
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(header.rel)), header.rel);
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(header.abs)), header.abs);

    for (int i = 0; i < ABS_MAX; i++) {
        if (header.abs[i / 8] & (1 << (i % 8))) {
            ioctl(fd, EVIOCGABS(i), &header.abs_info[i]);
        }
    }

    return header;
}

ssize_t
write_all(int fd, const void* buf, size_t len)
{
    const char* ptr       = buf;
    size_t      remaining = len;

    while (remaining > 0) {
        ssize_t n = send(fd, ptr, remaining, MSG_NOSIGNAL);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        remaining -= (size_t)n;
        ptr += n;
    }

    return (ssize_t)len;
}
