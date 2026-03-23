#include <sys/types.h>
#include "shared.h"

ssize_t read_all(int fd, void* buf, size_t len);
int create_virtual_device(header_t header);
