CC = arm-linux-gnueabihf-gcc

TARGET = embedded_projects_pi

DEPS = $(HOME)/rpi_deps

CFLAGS = -Wall -Wextra -O2 -std=gnu11 -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -Iinclude -I$(DEPS)/include -I$(DEPS)/include/cjson

LDLIBS = \
	$(DEPS)/lib/libwebsockets.a \
	$(DEPS)/lib/libcjson.a \
	$(DEPS)/lib/libmbedtls.a \
	$(DEPS)/lib/libmbedx509.a \
	$(DEPS)/lib/libmbedcrypto.a \
	-pthread \
	-lm \
	-ldl \
	-lrt

SRC = src/main.c \
      src/queue.c \
      src/cpu.c \
      src/producer.c \
      src/consumer.c \
      src/monitor.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) embedded_projects
