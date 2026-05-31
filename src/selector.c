/* ************************************************************************** */
/*                          selector.c                                        */
/*   ncurses interactive menus and device selector                            */
/* ************************************************************************** */

#include "kuulobt.h"

/* ── ncurses setup / teardown ───────────────────────────────────────────── */

void	nc_init(void)
{
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	nc_init_colors();
}

void	nc_end(void)
{
	endwin();
}

void	nc_init_colors(void)
{
	start_color();
	use_default_colors();
	init_pair(CP_NORMAL, -1, -1);
	init_pair(CP_SELECTED, COLOR_BLACK, COLOR_CYAN);
	init_pair(CP_HEADER, COLOR_CYAN, -1);
	init_pair(CP_DIM, COLOR_WHITE, -1);
	init_pair(CP_OK, COLOR_GREEN, -1);
	init_pair(CP_WARN, COLOR_YELLOW, -1);
	init_pair(CP_ERR, COLOR_RED, -1);
}

/* ── Generic list selector ──────────────────────────────────────────────── */
/*
** Draws a scrollable list of items. Wraps around at top/bottom.
** title:    Header text
** items:    Array of display strings
** count:    Number of items
** Returns:  Selected index or -1 if cancelled
*/

static void	draw_list(const char *title, const char **items,
	int count, int selected)
{
	int	max_y;
	int	max_x;
	int	list_h;
	int	top;
	int	row;
	int	i;

	getmaxyx(stdscr, max_y, max_x);
	(void)max_x;
	clear();
	/* Header */
	attron(A_BOLD | COLOR_PAIR(CP_HEADER));
	mvprintw(1, 2, "kuuloBT");
	attroff(A_BOLD | COLOR_PAIR(CP_HEADER));
	/* Title */
	attron(A_BOLD);
	mvprintw(3, 2, "%s", title);
	attroff(A_BOLD);
	/* Viewport: scroll if list exceeds available height */
	list_h = max_y - 7;
	if (list_h < 1)
		list_h = 1;
	top = 0;
	if (count > list_h)
	{
		top = selected - list_h / 2;
		if (top < 0)
			top = 0;
		if (top > count - list_h)
			top = count - list_h;
	}
	row = 5;
	i = top;
	while (i < count && i < top + list_h)
	{
		if (i == selected)
		{
			attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
			mvprintw(row, 2, " > %-*.*s",
				max_x - 6, max_x - 6, items[i]);
			attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
		}
		else
		{
			attron(COLOR_PAIR(CP_NORMAL));
			mvprintw(row, 2, "   %s", items[i]);
			attroff(COLOR_PAIR(CP_NORMAL));
		}
		row++;
		i++;
	}
	/* Scroll indicators */
	if (top > 0)
	{
		attron(COLOR_PAIR(CP_DIM));
		mvprintw(4, 2, "   ... (%d more above)", top);
		attroff(COLOR_PAIR(CP_DIM));
	}
	if (top + list_h < count)
	{
		attron(COLOR_PAIR(CP_DIM));
		mvprintw(row, 2, "   ... (%d more below)", count - top - list_h);
		attroff(COLOR_PAIR(CP_DIM));
	}
	/* Footer */
	attron(COLOR_PAIR(CP_DIM));
	mvprintw(max_y - 1, 2, "Up/Down: navigate   Enter: select   q: back");
	attroff(COLOR_PAIR(CP_DIM));
	refresh();
}

static int	run_selector(const char *title, const char **items, int count)
{
	int	selected;
	int	ch;

	selected = 0;
	draw_list(title, items, count, selected);
	while (1)
	{
		ch = getch();
		if (ch == KEY_UP || ch == 'k')
		{
			if (selected > 0)
				selected--;
			else
				selected = count - 1;
			draw_list(title, items, count, selected);
		}
		else if (ch == KEY_DOWN || ch == 'j')
		{
			if (selected < count - 1)
				selected++;
			else
				selected = 0;
			draw_list(title, items, count, selected);
		}
		else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER)
			return (selected);
		else if (ch == 'q' || ch == 27)
			return (-1);
	}
}

/* ── Main Menu ──────────────────────────────────────────────────────────── */

int	main_menu(void)
{
	static const char	*items[] = {
		"Scan          Scan for devices and connect",
		"Connect       Connect to a known device",
		"Disconnect    Disconnect current device",
		"Status        Show connection and audio status",
		"Diagnose      Run full system diagnostics",
		"Fix           Attempt to fix audio issues",
		"Reset         Remove a device pairing",
		"Quit          Exit kuuloBT"
	};
	int	count;

	count = 8;
	return (run_selector("What would you like to do?", items, count));
}

/* ── Device Selector ────────────────────────────────────────────────────── */

int	select_device(t_app *app)
{
	const char	*items[MAX_DEVICES];
	static char	labels[MAX_DEVICES][256];
	int			i;
	char		status[64];

	if (app->device_count == 0)
	{
		nc_end();
		printf("  %s No devices found\n", SYM_WARN);
		return (-1);
	}
	i = 0;
	while (i < app->device_count)
	{
		status[0] = '\0';
		if (app->devices[i].connected)
			snprintf(status, sizeof(status), " [connected]");
		else if (app->devices[i].paired)
			snprintf(status, sizeof(status), " [paired]");
		snprintf(labels[i], sizeof(labels[i]), "%-28s %s%s",
			app->devices[i].name, app->devices[i].mac, status);
		items[i] = labels[i];
		i++;
	}
	return (run_selector("Select a device", items, app->device_count));
}
