#include <asm-generic/errno-base.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

struct termios original_t;
void enable_raw_mode() {
	struct termios t;
	tcgetattr(STDIN_FILENO, &original_t);
	t = original_t;
	t.c_lflag &= ~(ICANON | ECHO);
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
}

void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_t);
}

void show_usage(char* program_name) {
	fprintf(stderr, "Usage: %s <iodevice>\n", program_name);
	exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
	if (argc < 2) {
		show_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	char* iodevice = argv[1];

	// check if the iodev exists
	if (access(iodevice, F_OK) == -1) {
		fprintf(stderr, "Error: Device '%s' does not exist\n", iodevice);
		exit(EXIT_FAILURE);
	}

	// check read permissions
	if (access(iodevice, R_OK | W_OK) == -1) {
		fprintf(stderr, "Error: insufficient permissions on '%s'\n", iodevice);
		fprintf(stderr, "Hint: try running with sudo\n");
		exit(EXIT_FAILURE);
	}

	int fd = open(iodevice, O_RDWR);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// try to gain exclusive read on the iodevice
	if(ioctl(fd, EVIOCGRAB, 1) == -1) {
		fprintf(stderr, "Failed to grab device %s\n", iodevice);
		perror("ioctl");
		close(fd);
		exit(EXIT_FAILURE);
	}

	printf("Device %s is now exclusive\n", iodevice);
	printf("Press q to release\n");

	atexit(disable_raw_mode);
	enable_raw_mode();

	char c;
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
