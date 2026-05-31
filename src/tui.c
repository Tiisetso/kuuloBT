/* ************************************************************************** */
/*                          tui.c                                             */
/*   ncurses activity screens: log, progress, stdout capture                  */
/* ************************************************************************** */

#include "kuulobt.h"
#include <stdarg.h>

/* ── Log buffer ─────────────────────────────────────────────────────────── */

static char	g_log[LOG_MAX][LOG_LINE_LEN];
static int	g_log_colors[LOG_MAX];
static int	g_log_count;

void	nc_log_clear(void)
{
	g_log_count = 0;
}

void	nc_log_add(int color, const char *fmt, ...)
{
	va_list	ap;

	if (g_log_count >= LOG_MAX)
	{
		/* Shift up */
		memmove(g_log[0], g_log[1], (LOG_MAX - 1) * LOG_LINE_LEN);
		memmove(g_log_colors, g_log_colors + 1, (LOG_MAX - 1) * sizeof(int));
		g_log_count = LOG_MAX - 1;
	}
	va_start(ap, fmt);
	vsnprintf(g_log[g_log_count], LOG_LINE_LEN, fmt, ap);
	va_end(ap);
	g_log_colors[g_log_count] = color;
	g_log_count++;
}

/* ── Draw the log screen ────────────────────────────────────────────────── */

void	nc_log_draw(const char *title)
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
	attron(A_BOLD | COLOR_PAIR(CP_HEADER));
	mvprintw(1, 2, "kuuloBT");
	attroff(A_BOLD | COLOR_PAIR(CP_HEADER));
	attron(A_BOLD);
	mvprintw(3, 2, "%s", title);
	attroff(A_BOLD);
	list_h = max_y - 7;
	if (list_h < 1)
		list_h = 1;
	top = 0;
	if (g_log_count > list_h)
		top = g_log_count - list_h;
	row = 5;
	i = top;
	while (i < g_log_count && row < max_y - 2)
	{
		if (g_log_colors[i])
			attron(COLOR_PAIR(g_log_colors[i]));
		mvprintw(row, 4, "%s", g_log[i]);
		if (g_log_colors[i])
			attroff(COLOR_PAIR(g_log_colors[i]));
		row++;
		i++;
	}
	refresh();
}

/*
** Draw log screen and wait for a keypress to continue.
*/
void	nc_log_wait(const char *title)
{
	int	max_y;
	int	max_x;

	nc_log_draw(title);
	getmaxyx(stdscr, max_y, max_x);
	(void)max_x;
	attron(COLOR_PAIR(CP_DIM));
	mvprintw(max_y - 1, 2, "Press any key to continue...");
	attroff(COLOR_PAIR(CP_DIM));
	refresh();
	getch();
}

/* ── Animated progress screen ───────────────────────────────────────────── */

void	nc_progress(const char *title, const char *msg, int seconds)
{
	int		max_y;
	int		max_x;
	int		i;
	char	dots[64];

	getmaxyx(stdscr, max_y, max_x);
	(void)max_x;
	i = 0;
	while (i < seconds)
	{
		memset(dots, '.', i + 1);
		dots[i + 1] = '\0';
		clear();
		attron(A_BOLD | COLOR_PAIR(CP_HEADER));
		mvprintw(1, 2, "kuuloBT");
		attroff(A_BOLD | COLOR_PAIR(CP_HEADER));
		attron(A_BOLD);
		mvprintw(3, 2, "%s", title);
		attroff(A_BOLD);
		attron(COLOR_PAIR(CP_DIM));
		mvprintw(5, 4, "%s%s (%ds remaining)", msg, dots,
			seconds - i);
		attroff(COLOR_PAIR(CP_DIM));
		attron(COLOR_PAIR(CP_DIM));
		mvprintw(max_y - 1, 2, "Please wait...");
		attroff(COLOR_PAIR(CP_DIM));
		refresh();
		sleep(1);
		i++;
	}
}

/* ── Capture stdout from a function and show in ncurses ─────────────────── */
/*
** Temporarily redirects stdout to a pipe, runs fn(app),
** reads all output, parses lines, displays in a scrollable log view.
*/

static void	capture_to_log(const char *buf, int len)
{
	int		i;
	int		start;
	char	line[LOG_LINE_LEN];
	int		ll;

	nc_log_clear();
	start = 0;
	i = 0;
	while (i < len)
	{
		if (buf[i] == '\n' || i == len - 1)
		{
			ll = i - start;
			if (buf[i] != '\n')
				ll++;
			if (ll >= LOG_LINE_LEN)
				ll = LOG_LINE_LEN - 1;
			memcpy(line, buf + start, ll);
			line[ll] = '\0';
			/* Strip ANSI escape sequences for ncurses display */
			{
				char	clean[LOG_LINE_LEN];
				int		ci;
				int		li;

				ci = 0;
				li = 0;
				while (line[li])
				{
					if (line[li] == '\033')
					{
						while (line[li] && line[li] != 'm')
							li++;
						if (line[li])
							li++;
					}
					else
						clean[ci++] = line[li++];
				}
				clean[ci] = '\0';
				if (clean[0])
					nc_log_add(0, "%s", clean);
			}
			start = i + 1;
		}
		i++;
	}
}

void	nc_run_captured(const char *title,
	void (*fn)(t_app *), t_app *app)
{
	int		pipefd[2];
	int		saved_stdout;
	char	buf[8192];
	int		n;
	int		total;

	if (pipe(pipefd) < 0)
	{
		nc_log_clear();
		nc_log_add(CP_ERR, "Failed to create pipe");
		nc_log_wait(title);
		return ;
	}
	saved_stdout = dup(STDOUT_FILENO);
	dup2(pipefd[1], STDOUT_FILENO);
	close(pipefd[1]);
	/* Show a "working" screen while the function runs */
	fn(app);
	fflush(stdout);
	dup2(saved_stdout, STDOUT_FILENO);
	close(saved_stdout);
	/* Read captured output */
	total = 0;
	while ((n = read(pipefd[0], buf + total,
		sizeof(buf) - 1 - total)) > 0)
		total += n;
	close(pipefd[0]);
	buf[total] = '\0';
	capture_to_log(buf, total);
	nc_log_wait(title);
}

void	nc_run_captured_void(const char *title,
	void (*fn)(void))
{
	int		pipefd[2];
	int		saved_stdout;
	char	buf[8192];
	int		n;
	int		total;

	if (pipe(pipefd) < 0)
	{
		nc_log_clear();
		nc_log_add(CP_ERR, "Failed to create pipe");
		nc_log_wait(title);
		return ;
	}
	saved_stdout = dup(STDOUT_FILENO);
	dup2(pipefd[1], STDOUT_FILENO);
	close(pipefd[1]);
	fn();
	fflush(stdout);
	dup2(saved_stdout, STDOUT_FILENO);
	close(saved_stdout);
	total = 0;
	while ((n = read(pipefd[0], buf + total,
		sizeof(buf) - 1 - total)) > 0)
		total += n;
	close(pipefd[0]);
	buf[total] = '\0';
	capture_to_log(buf, total);
	nc_log_wait(title);
}
