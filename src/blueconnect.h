/* ************************************************************************** */
/*                          blueconnect.h                                     */
/*   AirPods Pro connector for Linux (no sudo)                                */
/* ************************************************************************** */

#ifndef BLUECONNECT_H
# define BLUECONNECT_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <signal.h>
# include <time.h>
# include <sys/wait.h>
# include <dbus/dbus.h>
# include <pulse/pulseaudio.h>

/* ── Color codes ────────────────────────────────────────────────────────── */
# define C_RST   "\033[0m"
# define C_BOLD  "\033[1m"
# define C_DIM   "\033[2m"
# define C_RED   "\033[31m"
# define C_GRN   "\033[32m"
# define C_YEL   "\033[33m"
# define C_BLU   "\033[34m"
# define C_MAG   "\033[35m"
# define C_CYN   "\033[36m"
# define C_WHT   "\033[37m"

/* ── Status symbols ─────────────────────────────────────────────────────── */
# define SYM_OK   C_GRN "✓" C_RST
# define SYM_FAIL C_RED "✗" C_RST
# define SYM_WARN C_YEL "!" C_RST
# define SYM_INFO C_BLU "●" C_RST
# define SYM_DOT  C_DIM "·" C_RST

/* ── Limits ─────────────────────────────────────────────────────────────── */
# define MAX_DEVICES   64
# define MAC_LEN       18
# define NAME_LEN      128
# define PATH_LEN      256
# define BUF_LEN       1024

/* ── Structures ─────────────────────────────────────────────────────────── */
typedef struct s_bt_device
{
	char	mac[MAC_LEN];
	char	name[NAME_LEN];
	char	path[PATH_LEN];
	int		paired;
	int		trusted;
	int		connected;
	int		is_airpods;
}	t_bt_device;

typedef struct s_pa_state
{
	pa_mainloop		*ml;
	pa_mainloop_api	*api;
	pa_context		*ctx;
	int				ready;
	int				done;
	char			default_sink[NAME_LEN];
	char			bt_sink[NAME_LEN];
	char			bt_card[NAME_LEN];
	int				bt_card_index;
}	t_pa_state;

typedef struct s_app
{
	DBusConnection	*dbus;
	t_pa_state		pa;
	t_bt_device		devices[MAX_DEVICES];
	int				device_count;
	int				verbose;
}	t_app;

/* ── bluetooth.c ────────────────────────────────────────────────────────── */
int		bt_init(t_app *app);
void	bt_cleanup(t_app *app);
int		bt_scan(t_app *app, int duration_sec);
int		bt_get_devices(t_app *app);
int		bt_pair(t_app *app, const char *path);
int		bt_trust(t_app *app, const char *path);
int		bt_connect(t_app *app, const char *path);
int		bt_disconnect(t_app *app, const char *path);
int		bt_remove(t_app *app, const char *path);
int		bt_get_device_info(t_app *app, t_bt_device *dev);

/* ── audio.c ────────────────────────────────────────────────────────────── */
int		pa_setup(t_app *app);
void	pa_teardown(t_app *app);
int		pa_find_bt_sink(t_app *app);
int		pa_set_profile_a2dp(t_app *app);
int		pa_set_default_sink(t_app *app);
int		pa_test_audio(t_app *app);

/* ── diagnose.c ─────────────────────────────────────────────────────────── */
void	run_diagnostics(t_app *app);
void	fix_audio_config(void);
void	show_status(t_app *app);

/* ── utils.c ────────────────────────────────────────────────────────────── */
int		run_cmd(const char *cmd, char *out, int out_len);
int		run_cmd_silent(const char *cmd);
void	print_banner(void);
void	print_help(void);
int		is_airpods_name(const char *name);
void	mac_to_path(const char *mac, char *path);
void	path_to_mac(const char *path, char *mac);
void	progress_dots(const char *msg, int seconds);

#endif
