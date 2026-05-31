# kuuloBT - Bluetooth audio connector for Linux
# No sudo required

NAME     = kuulobt
CC       = cc
CFLAGS   = -Wall -Wextra -Werror -g
INCDIR   = include
OBJDIR   = obj
DBUS_CFLAGS  = $(shell pkg-config --cflags dbus-1)
DBUS_LIBS    = $(shell pkg-config --libs dbus-1)
PULSE_CFLAGS = $(shell pkg-config --cflags libpulse)
PULSE_LIBS   = $(shell pkg-config --libs libpulse)
NC_CFLAGS    = $(shell pkg-config --cflags ncursesw)
NC_LIBS      = $(shell pkg-config --libs ncursesw)

ALL_CFLAGS = $(CFLAGS) -I$(INCDIR) $(DBUS_CFLAGS) $(PULSE_CFLAGS) $(NC_CFLAGS)
ALL_LIBS   = $(DBUS_LIBS) $(PULSE_LIBS) $(NC_LIBS)

SRCS = src/main.c src/bluetooth.c src/audio.c src/diagnose.c src/utils.c src/selector.c src/tui.c
OBJS = $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRCS))

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LIBS)

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
