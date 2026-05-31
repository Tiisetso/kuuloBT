# blueconnect - AirPods Pro connector for Linux
# No sudo required

NAME     = blueconnect
CC       = cc
CFLAGS   = -Wall -Wextra -Werror -g
DBUS_CFLAGS  = $(shell pkg-config --cflags dbus-1)
DBUS_LIBS    = $(shell pkg-config --libs dbus-1)
PULSE_CFLAGS = $(shell pkg-config --cflags libpulse)
PULSE_LIBS   = $(shell pkg-config --libs libpulse)

ALL_CFLAGS = $(CFLAGS) $(DBUS_CFLAGS) $(PULSE_CFLAGS)
ALL_LIBS   = $(DBUS_LIBS) $(PULSE_LIBS)

SRCS = src/main.c src/bluetooth.c src/audio.c src/diagnose.c src/utils.c
OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LIBS)

%.o: %.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
