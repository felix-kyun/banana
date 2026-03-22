#include "./shared.h"
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct termios original_t;
void
enable_raw_mode()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &original_t);
    t = original_t;
    t.c_lflag &= ~(ICANON | ECHO);
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
show_usage(char* program_name)
{
    fprintf(stderr, "Usage: %s <ip> <device>\n", program_name);
    fputs("    ip: IP address of the server\n", stderr);
    fputs("    device: Path to the input device\n", stderr);
    exit(EXIT_FAILURE);
}

int
main(int argc, char** argv)
{
    if (argc < 3) {
        show_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char* ip     = argv[1];
    char* device = argv[2];

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

    printf("Device Info\n");
    printf("Name: %s\n", header.name);
    printf("ID: 0x%x:0x%x\n", header.id.vendor, header.id.product);

    // try to gain exclusive read on the device
    if (ioctl(fd, EVIOCGRAB, 1) == -1) {
        fprintf(stderr, "Failed to grab device %s\n", device);
        perror("ioctl");
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
        if (read(STDIN_FILENO, &c, 1)) {
            if (c == 'q') {
                printf("Releasing exclusive mode\n");
                ioctl(fd, EVIOCGRAB, 0);
                break;
            }
        }
        if (read(fd, &ev, sizeof(ev))) {
            printf("Event: type=%u, code=%u, value=%u\n", ev.type, ev.code, ev.value);
        }
    }

    close(fd);
    return 0;
}
