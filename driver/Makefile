CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11

TARGET = ara_wse_16384core_test_runner
PCIE_TARGET = ara_pcie_test_runner
SRCS = ara_wse_16384core_test_runner.c ara_wse_16384core_driver.c

all: $(TARGET) $(PCIE_TARGET)

$(TARGET): $(SRCS) ara_wse_16384core_driver.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lm

$(PCIE_TARGET): ara_pcie_test_runner.c
	$(CC) $(CFLAGS) -o $(PCIE_TARGET) ara_pcie_test_runner.c -lm

test: $(TARGET) $(PCIE_TARGET)
	./$(TARGET)
	./$(PCIE_TARGET)

clean:
	rm -f $(TARGET) $(PCIE_TARGET) *.o
