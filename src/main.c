/* ************************************************************************** */
/*                          main.c                                            */
/*   Entry point for kuuloBT                                                  */
/* ************************************************************************** */

#include "kuulobt.h"

/* ── Find device by MAC ─────────────────────────────────────────────────── */

static t_bt_device	*find_by_mac(t_app *app, const char *mac)
{
	int	i;

	i = 0;
	while (i < app->device_count)
	{
		if (strcmp(app->devices[i].mac, mac) == 0)
			return (&app->devices[i]);
		i++;
	}
	return (NULL);
}

/* ── Connect device in ncurses (log screen) ─────────────────────────────── */

static int	nc_connect_device(t_app *app, t_bt_device *dev)
{
	char	title[256];

	nc_log_clear();
	snprintf(title, sizeof(title), "Connecting to %s", dev->name);
	nc_log_add(CP_DIM, "MAC: %s", dev->mac);
	nc_log_add(0, "");
	/* Pair */
	if (dev->paired <= 0)
	{
		nc_log_add(0, "Pairing...");
		nc_log_draw(title);
		if (bt_pair(app, dev->path) < 0)
			nc_log_add(CP_WARN, "Pairing failed (may already be paired)");
		else
			nc_log_add(CP_OK, "Paired successfully");
	}
	else
		nc_log_add(CP_OK, "Already paired");
	nc_log_draw(title);
	/* Trust */
	if (dev->trusted <= 0)
	{
		nc_log_add(0, "Trusting device...");
		nc_log_draw(title);
		bt_trust(app, dev->path);
		nc_log_add(CP_OK, "Device trusted");
	}
	else
		nc_log_add(CP_OK, "Already trusted");
	nc_log_draw(title);
	/* Connect */
	if (dev->connected <= 0)
	{
		nc_log_add(0, "Connecting...");
		nc_log_draw(title);
		if (bt_connect(app, dev->path) < 0)
		{
			nc_log_add(CP_WARN, "Retrying connection...");
			nc_log_draw(title);
			sleep(2);
			bt_connect(app, dev->path);
		}
		nc_log_add(CP_OK, "Connected");
	}
	else
		nc_log_add(CP_OK, "Already connected");
	nc_log_draw(title);
	/* Audio setup */
	nc_log_add(0, "");
	nc_log_add(0, "Waiting for audio to initialize...");
	nc_log_draw(title);
	sleep(3);
	if (pa_setup(app) == 0)
	{
		nc_log_add(0, "Setting up A2DP audio profile...");
		nc_log_draw(title);
		pa_set_profile_a2dp(app);
		sleep(1);
		pa_find_bt_sink(app);
		if (app->pa.bt_sink[0])
		{
			pa_set_default_sink(app);
			nc_log_add(0, "");
			nc_log_add(CP_OK, "Audio routed to %s", dev->name);
		}
		else
		{
			nc_log_add(0, "");
			nc_log_add(CP_WARN, "BT sink not appearing in PulseAudio");
			nc_log_add(CP_DIM, "Try 'Diagnose' or 'Fix' from the menu");
		}
		pa_teardown(app);
	}
	else
	{
		nc_log_add(CP_WARN, "Could not connect to PulseAudio");
	}
	nc_log_wait(title);
	return (0);
}

/* ── Connect device in CLI mode (printf) ────────────────────────────────── */

static int	cli_connect_device(t_app *app, t_bt_device *dev)
{
	printf("\n  %s Target: " C_CYN C_BOLD "%s" C_RST " [%s]\n\n",
		SYM_INFO, dev->name, dev->mac);
	if (dev->paired <= 0)
		bt_pair(app, dev->path);
	else
		printf("  %s Already paired\n", SYM_OK);
	if (dev->trusted <= 0)
		bt_trust(app, dev->path);
	else
		printf("  %s Already trusted\n", SYM_OK);
	if (dev->connected <= 0)
	{
		if (bt_connect(app, dev->path) < 0)
		{
			printf("  %s Retrying connection...\n", SYM_WARN);
			sleep(2);
			bt_connect(app, dev->path);
		}
	}
	else
		printf("  %s Already connected\n", SYM_OK);
	printf("  %s Waiting for audio to initialize...\n", SYM_INFO);
	sleep(3);
	if (pa_setup(app) == 0)
	{
		pa_set_profile_a2dp(app);
		sleep(1);
		pa_find_bt_sink(app);
		if (app->pa.bt_sink[0])
		{
			pa_set_default_sink(app);
			printf("\n  %s " C_GRN C_BOLD "Audio routed to %s" C_RST "\n",
				SYM_OK, dev->name);
		}
		else
		{
			printf("\n  %s BT sink not appearing in PulseAudio\n", SYM_WARN);
			printf("  %s Run kuulobt diagnose for details\n", SYM_INFO);
		}
		pa_teardown(app);
	}
	printf("\n");
	return (0);
}

/* ── Interactive action handlers (stay in ncurses) ──────────────────────── */

static int	nc_do_scan(t_app *app, int duration)
{
	int	idx;

	/* Animated progress while scanning */
	bt_start_discovery(app);
	nc_progress("Scanning", "Scanning for Bluetooth devices", duration);
	bt_stop_discovery(app);
	bt_get_devices(app);
	if (app->device_count == 0)
	{
		nc_log_clear();
		nc_log_add(CP_WARN, "No devices found");
		nc_log_wait("Scan Complete");
		return (0);
	}
	/* Show device selector */
	idx = select_device(app);
	if (idx < 0)
		return (0);
	return (nc_connect_device(app, &app->devices[idx]));
}

static int	nc_do_connect(t_app *app)
{
	int	idx;

	/* Loading screen while fetching devices */
	nc_log_clear();
	nc_log_add(0, "Fetching known devices...");
	nc_log_draw("Connect");
	bt_get_devices(app);
	if (app->device_count == 0)
	{
		/* No known devices — scan first */
		bt_start_discovery(app);
		nc_progress("Scanning", "No known devices. Scanning", 10);
		bt_stop_discovery(app);
		bt_get_devices(app);
		if (app->device_count == 0)
		{
			nc_log_clear();
			nc_log_add(CP_WARN, "No devices found");
			nc_log_wait("Connect");
			return (0);
		}
	}
	idx = select_device(app);
	if (idx < 0)
		return (0);
	return (nc_connect_device(app, &app->devices[idx]));
}

static int	nc_do_disconnect(t_app *app)
{
	int	i;
	int	idx;

	bt_get_devices(app);
	/* Auto-disconnect first connected device */
	i = 0;
	while (i < app->device_count)
	{
		if (app->devices[i].connected > 0)
		{
			nc_log_clear();
			nc_log_add(0, "Disconnecting %s...", app->devices[i].name);
			nc_log_draw("Disconnect");
			bt_disconnect(app, app->devices[i].path);
			nc_log_add(CP_OK, "Disconnected");
			nc_log_wait("Disconnect");
			return (0);
		}
		i++;
	}
	if (app->device_count == 0)
	{
		nc_log_clear();
		nc_log_add(CP_WARN, "No devices known");
		nc_log_wait("Disconnect");
		return (0);
	}
	idx = select_device(app);
	if (idx < 0)
		return (0);
	nc_log_clear();
	nc_log_add(0, "Disconnecting %s...", app->devices[idx].name);
	nc_log_draw("Disconnect");
	bt_disconnect(app, app->devices[idx].path);
	nc_log_add(CP_OK, "Disconnected");
	nc_log_wait("Disconnect");
	return (0);
}

static int	nc_do_reset(t_app *app)
{
	int			idx;
	t_bt_device	*dev;

	bt_get_devices(app);
	if (app->device_count == 0)
	{
		nc_log_clear();
		nc_log_add(CP_WARN, "No devices to reset");
		nc_log_wait("Reset");
		return (0);
	}
	idx = select_device(app);
	if (idx < 0)
		return (0);
	dev = &app->devices[idx];
	nc_log_clear();
	nc_log_add(0, "Resetting %s [%s]", dev->name, dev->mac);
	nc_log_draw("Reset");
	if (dev->connected > 0)
	{
		nc_log_add(0, "Disconnecting...");
		nc_log_draw("Reset");
		bt_disconnect(app, dev->path);
	}
	sleep(1);
	nc_log_add(0, "Removing pairing...");
	nc_log_draw("Reset");
	bt_remove(app, dev->path);
	nc_log_add(CP_OK, "Reset complete. Ready to re-pair.");
	nc_log_wait("Reset");
	return (0);
}

/* ── Dispatch interactive menu ──────────────────────────────────────────── */

static int	nc_dispatch(t_app *app, int choice)
{
	switch (choice)
	{
		case 0:
			return (nc_do_scan(app, 10));
		case 1:
			return (nc_do_connect(app));
		case 2:
			return (nc_do_disconnect(app));
		case 3:
			nc_run_captured("Status", show_status, app);
			return (0);
		case 4:
			nc_run_captured("Diagnostics", run_diagnostics, app);
			return (0);
		case 5:
			nc_run_captured_void("Fix Audio", fix_audio_config);
			return (0);
		case 6:
			return (nc_do_reset(app));
		case 7:
			return (-1);
		default:
			return (-1);
	}
}

/* ── Interactive loop ───────────────────────────────────────────────────── */

static int	interactive_loop(t_app *app)
{
	int	choice;
	int	ret;

	nc_init();
	while (1)
	{
		choice = main_menu();
		if (choice < 0 || choice == 7)
		{
			nc_end();
			return (0);
		}
		ret = nc_dispatch(app, choice);
		if (ret == -1)
		{
			nc_end();
			return (0);
		}
	}
}

/* ── CLI dispatch (printf-based, for direct command usage) ──────────────── */

static int	cli_dispatch(t_app *app, int argc, char **argv)
{
	char	*mac;
	int		duration;
	int		i;

	mac = NULL;
	duration = 10;
	i = 2;
	while (i < argc)
	{
		if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
		{
			mac = argv[i + 1];
			i += 2;
		}
		else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
		{
			duration = atoi(argv[i + 1]);
			i += 2;
		}
		else if (strcmp(argv[i], "-v") == 0)
		{
			app->verbose = 1;
			i++;
		}
		else
			i++;
	}
	if (strcmp(argv[1], "scan") == 0)
	{
		bt_scan(app, duration);
		if (app->device_count == 0)
			return (printf("  %s No devices found\n", SYM_WARN), 0);
		printf("  %s Found %d device(s)\n\n", SYM_OK, app->device_count);
		nc_init();
		i = select_device(app);
		nc_end();
		if (i >= 0)
			cli_connect_device(app, &app->devices[i]);
		return (0);
	}
	if (strcmp(argv[1], "connect") == 0)
	{
		bt_get_devices(app);
		if (mac)
		{
			i = -1;
			{
				t_bt_device	*dev;

				dev = find_by_mac(app, mac);
				if (!dev)
				{
					printf("  %s Device not found. Scanning...\n", SYM_WARN);
					bt_scan(app, 10);
					dev = find_by_mac(app, mac);
				}
				if (!dev)
					return (printf("  %s Device not found\n", SYM_FAIL), 1);
				return (cli_connect_device(app, dev));
			}
		}
		if (app->device_count == 0)
		{
			printf("  %s No devices known. Scanning...\n", SYM_INFO);
			bt_scan(app, 10);
		}
		if (app->device_count == 0)
			return (printf("  %s No devices found\n", SYM_FAIL), 1);
		nc_init();
		i = select_device(app);
		nc_end();
		if (i >= 0)
			return (cli_connect_device(app, &app->devices[i]));
		return (0);
	}
	if (strcmp(argv[1], "disconnect") == 0)
	{
		bt_get_devices(app);
		if (mac)
		{
			i = 0;
			while (i < app->device_count)
			{
				if (strcmp(app->devices[i].mac, mac) == 0)
					return (bt_disconnect(app, app->devices[i].path));
				i++;
			}
			printf("  %s Device not found\n", SYM_WARN);
			return (1);
		}
		i = 0;
		while (i < app->device_count)
		{
			if (app->devices[i].connected > 0)
				return (bt_disconnect(app, app->devices[i].path));
			i++;
		}
		printf("  %s No connected devices\n", SYM_WARN);
		return (1);
	}
	if (strcmp(argv[1], "status") == 0)
		return (show_status(app), 0);
	if (strcmp(argv[1], "diagnose") == 0)
		return (run_diagnostics(app), 0);
	if (strcmp(argv[1], "fix") == 0)
		return (fix_audio_config(), 0);
	if (strcmp(argv[1], "reset") == 0)
	{
		bt_get_devices(app);
		if (mac)
		{
			t_bt_device	*dev;

			dev = find_by_mac(app, mac);
			if (!dev)
				return (printf("  %s Device not found\n", SYM_WARN), 1);
			if (dev->connected > 0)
				bt_disconnect(app, dev->path);
			sleep(1);
			bt_remove(app, dev->path);
			printf("  %s Reset complete\n", SYM_OK);
			return (0);
		}
		if (app->device_count == 0)
			return (printf("  %s No devices to reset\n", SYM_WARN), 1);
		nc_init();
		i = select_device(app);
		nc_end();
		if (i >= 0)
		{
			if (app->devices[i].connected > 0)
				bt_disconnect(app, app->devices[i].path);
			sleep(1);
			bt_remove(app, app->devices[i].path);
			printf("  %s Reset complete\n", SYM_OK);
		}
		return (0);
	}
	fprintf(stderr, "kuulobt: unknown command '%s'\n", argv[1]);
	fprintf(stderr, "Run 'kuulobt help' for usage.\n");
	return (1);
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int	main(int argc, char **argv)
{
	t_app	app;
	int		ret;

	if (argc >= 2 && (strcmp(argv[1], "help") == 0
		|| strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
	{
		print_help();
		return (0);
	}
	memset(&app, 0, sizeof(t_app));
	if (bt_init(&app) < 0)
		return (1);
	if (argc < 2)
		ret = interactive_loop(&app);
	else
		ret = cli_dispatch(&app, argc, argv);
	bt_cleanup(&app);
	return (ret);
}
