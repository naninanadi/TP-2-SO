CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGET = fs

SRCS = main.c \
       sistemaDeArquivos.c \
       inode.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q *.o *.exe 2>nul || rm -f *.o *.exe

ARQ ?=

run: $(TARGET)
	./$(TARGET) $(ARQ)