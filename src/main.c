/* ************************************************************************** */
/*                          main.c                                            */
/*   Entry point for blueconnect                                              */
/* ************************************************************************** */

#include "blueconnect.h"

/* ── Find AirPods among known devices (or pick by MAC) ──────────────────── */

static t_bt_device	*find_target(t_app *app, const char *mac)
{
	int	i;

	i = 0;
	while (i < app->device_count)
	{
		if (mac && strcmp(app->devices[i].mac, mac) == 0)
			return (&app->devices[i]);
		if (!mac && app->devices[i].is_airpods)
			return (&app->devices[i]);
		i++;
	}
	return (NULL);
}

/* ── Connect flow: pair → trust → connect → set A2DP → set sink ─────── */

static int	do_connect(t_app *app, const char *mac)
{
	t_bt_device	*dev;

	/* Get current device list */
	bt_get_devices(app);
	dev = find_target(app, mac);
	if (!dev)
	{
		printf("  %s AirPods not found. Scanning...\n", SYM_WARN);
		bt_scan(app, 10);
		dev = find_target(app, mac);
	}
	if (!dev)
	{
		printf("  %s Could not find "
			C_MAG "AirPods" C_RST "\n", SYM_FAIL);
		if (!mac)
		{
			printf("  %s Try: open AirPods case near computer, then:\n",
				SYM_INFO);
			printf("        blueconnect scan\n");
			printf("        blueconnect connect -m XX:XX:XX:XX:XX:XX\n");
		}
		return (1);
	}
	printf("\n  %s Target: " C_MAG C_BOLD "%s" C_RST " [%s]\n\n",
		SYM_INFO, dev->name, dev->mac);
	/* Step 1: Pair if needed */
	if (dev->paired <= 0)
		bt_pair(app, dev->path);
	else
		printf("  %s Already paired\n", SYM_OK);
	/* Step 2: Trust */
	if (dev->trusted <= 0)
		bt_trust(app, dev->path);
	else
		printf("  %s Already trusted\n", SYM_OK);
	/* Step 3: Connect */
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
	/* Step 4: Wait for PulseAudio to pick up the device */
	printf("  %s Waiting for audio to initialize...\n", SYM_INFO);
	sleep(3);
	/* Step 5: Set up audio */
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
			printf("  %s Run " C_CYN "blueconnect diagnose" C_RST
				" for details\n", SYM_INFO);
			printf("  %s Run " C_CYN "blueconnect fix" C_RST
				" to attempt repairs\n", SYM_INFO);
		}
		pa_teardown(app);
	}
	printf("\n");
	return (0);
}

/* ── Disconnect flow ────────────────────────────────────────────────────── */

static int	do_disconnect(t_app *app, const char *mac)
{
	t_bt_device	*dev;

	bt_get_devices(app);
	dev = find_target(app, mac);
	if (!dev || dev->connected <= 0)
	{
		printf("  %s No connected AirPods found\n", SYM_WARN);
		return (1);
	}
	return (bt_disconnect(app, dev->path));
}

/* ── Reset: remove pairing, clean up ────────────────────────────────────── */

static int	do_reset(t_app *app, const char *mac)
{
	t_bt_device	*dev;

	bt_get_devices(app);
	dev = find_target(app, mac);
	if (!dev)
	{
		printf("  %s No AirPods found to reset\n", SYM_WARN);
		return (1);
	}
	printf("  %s Resetting " C_MAG "%s" C_RST " [%s]\n",
		SYM_INFO, dev->name, dev->mac);
	if (dev->connected > 0)
		bt_disconnect(app, dev->path);
	sleep(1);
	bt_remove(app, dev->path);
	printf("  %s Reset complete. Ready to re-pair.\n", SYM_OK);
	return (0);
}

/* ── Scan and display ───────────────────────────────────────────────────── */

static int	do_scan(t_app *app, int duration)
{
	int	i;
	int	airpods_found;

	bt_scan(app, duration);
	airpods_found = 0;
	printf("\n" C_BOLD "  ── Devices Found (%d) ──" C_RST "\n",
		app->device_count);
	i = 0;
	while (i < app->device_count)
	{
		printf("  ");
		if (app->devices[i].is_airpods)
		{
			printf(C_MAG C_BOLD "🎧 %s" C_RST, app->devices[i].name);
			airpods_found++;
		}
		else
			printf("%s %s", SYM_DOT, app->devices[i].name);
		printf(" [%s]", app->devices[i].mac);
		if (app->devices[i].connected)
			printf(" " C_GRN "(connected)" C_RST);
		if (app->devices[i].paired)
			printf(" (paired)");
		printf("\n");
		i++;
	}
	if (airpods_found)
		printf("\n  %s Found %d AirPods device(s)! "
			"Run " C_CYN "blueconnect connect" C_RST "\n", SYM_OK,
			airpods_found);
	else
		printf("\n  %s No AirPods found. Open the case and hold the "
			"button on back.\n", SYM_WARN);
	printf("\n");
	return (0);
}

/* ── Parse arguments ────────────────────────────────────────────────────── */

static void	parse_opts(int argc, char **argv, char **mac,
	int *duration, int *verbose)
{
	int	i;

	*mac = NULL;
	*duration = 10;
	*verbose = 0;
	i = 2;
	while (i < argc)
	{
		if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
		{
			*mac = argv[i + 1];
			i += 2;
		}
		else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
		{
			*duration = atoi(argv[i + 1]);
			i += 2;
		}
		else if (strcmp(argv[i], "-v") == 0)
		{
			*verbose = 1;
			i++;
		}
		else
			i++;
	}
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int	main(int argc, char **argv)
{
	t_app	app;
	char	*mac;
	int		duration;
	int		ret;

	if (argc < 2 || strcmp(argv[1], "help") == 0
		|| strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
	{
		print_help();
		return (0);
	}
	memset(&app, 0, sizeof(t_app));
	parse_opts(argc, argv, &mac, &duration, &app.verbose);
	if (bt_init(&app) < 0)
		return (1);
	print_banner();
	ret = 0;
	if (strcmp(argv[1], "scan") == 0)
		ret = do_scan(&app, duration);
	else if (strcmp(argv[1], "connect") == 0)
		ret = do_connect(&app, mac);
	else if (strcmp(argv[1], "disconnect") == 0)
		ret = do_disconnect(&app, mac);
	else if (strcmp(argv[1], "status") == 0)
		show_status(&app);
	else if (strcmp(argv[1], "diagnose") == 0)
		run_diagnostics(&app);
	else if (strcmp(argv[1], "fix") == 0)
		fix_audio_config();
	else if (strcmp(argv[1], "reset") == 0)
		ret = do_reset(&app, mac);
	else
	{
		printf("  %s Unknown command: %s\n", SYM_FAIL, argv[1]);
		print_help();
		ret = 1;
	}
	bt_cleanup(&app);
	return (ret);
}
