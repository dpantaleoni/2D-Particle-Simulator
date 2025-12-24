CC = clang
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm -pthread

SRC = main.c init.c physics.c

OBJ = $(SRC:.c=.o)

TARGET = nbody

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)