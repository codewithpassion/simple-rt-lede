
all: network.o networkmanager.o adk.o accessory.c linux/tun.o linux/utils.o unix_common/resolver.o main.o 
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 -O3 -Wall -o simple-rt-cli $^ $(LDLIBS) -lrt
