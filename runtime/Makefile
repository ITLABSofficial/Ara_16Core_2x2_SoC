CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -fPIC -I../driver
LDFLAGS ?= -shared

all: libara_runtime.so test_ara_runtime

libara_runtime.so: libara_runtime.c libara_runtime.h ../driver/ara_wse_driver.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o libara_runtime.so libara_runtime.c

test_ara_runtime: test_ara_runtime.c libara_runtime.c libara_runtime.h ../driver/ara_wse_driver.h
	$(CC) $(CFLAGS) -o test_ara_runtime test_ara_runtime.c libara_runtime.c

test: all
	./test_ara_runtime

clean:
	rm -f libara_runtime.so test_ara_runtime
