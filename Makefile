CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wno-unused-parameter -std=c99 \
         -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
         -g -Iinclude

TARGET = cshell

SRCS = src/cshell.c src/shellPrompt.c src/execute.c src/lexer.c \
       src/parser.c src/hop.c src/reveal.c src/log.c \
       src/background.c src/redirection.c src/activities.c \
       src/ping.c src/signals.c

OBJS = $(SRCS:.c=.o)

HDRS = include/shellPrompt.h include/token.h include/hop.h \
       include/reveal.h include/log.h include/redirection.h \
       include/activities.h include/ping.h include/signals.h \
       include/background.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

rebuild: clean all

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)

.PHONY: all clean rebuild install
