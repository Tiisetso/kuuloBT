/* ************************************************************************** */
/*                          diagnose.c                                        */
/*   System diagnostics and fix routines                                      */
/* ************************************************************************** */

#include "kuulobt.h"

static void	check_bluetooth_adapter(t_app *app)
{
	char	buf[BUF_LEN];
	int		powered;

	printf("\n" C_BOLD "Bluetooth Adapter" C_RST "\n");
	powered = 0;
	if (run_cmd("bluetoothctl show 2>/dev/null | grep 'Powered:'", buf,
			sizeof(buf)) == 0)
	{
		if (strstr(buf, "yes"))
		{
			printf("  %s Adapter powered on\n", SYM_OK);
			powered = 1;
		}
		else
			printf("  %s Adapter powered " C_RED "OFF" C_RST "\n", SYM_FAIL);
	}
	else
		printf("  %s Cannot query adapter\n", SYM_FAIL);
	if (!powered)
	{
		printf("  %s Attempting to power on...\n", SYM_INFO);
		(void)app;
		run_cmd_silent("bluetoothctl power on 2>/dev/null");
	}
	if (run_cmd("bluetoothctl show 2>/dev/null | grep 'Name:'", buf,
			sizeof(buf)) == 0)
		printf("  %s %s", SYM_INFO, buf);
}

static void	check_audio_server(void)
{
	char	buf[BUF_LEN];

	printf("\n" C_BOLD "Audio Server" C_RST "\n");
	if (run_cmd("pactl info 2>/dev/null | grep 'Server Name:'", buf,
			sizeof(buf)) == 0)
		printf("  %s %s", SYM_OK, buf);
	else
		printf("  %s PulseAudio not running\n", SYM_FAIL);
	if (run_cmd("pactl info 2>/dev/null | grep 'Default Sink:'", buf,
			sizeof(buf)) == 0)
	{
		if (strstr(buf, "auto_null") || strstr(buf, "null"))
			printf("  %s Default sink: " C_RED "Dummy Output (no real audio)"
				C_RST "\n", SYM_WARN);
		else
			printf("  %s %s", SYM_OK, buf);
	}
}

static void	check_bt_modules(void)
{
	char	buf[BUF_LEN];

	printf("\n" C_BOLD "Bluetooth Audio Modules" C_RST "\n");
	if (run_cmd("pactl list modules short 2>/dev/null "
			"| grep bluetooth", buf, sizeof(buf)) == 0 && buf[0])
		printf("  %s module-bluetooth-discover loaded\n", SYM_OK);
	else
		printf("  %s module-bluetooth-discover " C_RED "NOT loaded" C_RST "\n",
			SYM_FAIL);
	if (run_cmd("pactl list modules short 2>/dev/null "
			"| grep bluez5-discover", buf, sizeof(buf)) == 0 && buf[0])
		printf("  %s module-bluez5-discover loaded\n", SYM_OK);
	else
	{
		printf("  %s module-bluez5-discover " C_RED "NOT loaded" C_RST "\n",
			SYM_FAIL);
		printf("  %s Attempting to load...\n", SYM_INFO);
		run_cmd_silent("pactl load-module module-bluez5-discover 2>/dev/null");
	}
	if (run_cmd("ls /usr/lib/x86_64-linux-gnu/pulse-*/modules/"
			"module-bluez5-device.so 2>/dev/null", buf, sizeof(buf)) == 0
		&& buf[0])
		printf("  %s module-bluez5-device.so present on disk\n", SYM_OK);
	else
		printf("  %s module-bluez5-device.so " C_RED "MISSING" C_RST
			" - BT audio cannot work!\n", SYM_FAIL);
}

static void	check_alsa_cards(void)
{
	char	buf[BUF_LEN];

	printf("\n" C_BOLD "ALSA Sound Cards" C_RST "\n");
	if (run_cmd("cat /proc/asound/cards 2>/dev/null", buf,
			sizeof(buf)) == 0 && buf[0])
		printf("%s", buf);
	else
		printf("  %s No ALSA cards found\n", SYM_FAIL);
}

static void	check_pa_sinks(void)
{
	char	buf[BUF_LEN];

	printf("\n" C_BOLD "PulseAudio Sinks" C_RST "\n");
	if (run_cmd("pactl list sinks short 2>/dev/null", buf,
			sizeof(buf)) == 0 && buf[0])
		printf("%s", buf);
	else
		printf("  %s No sinks found\n", SYM_WARN);
}

static void	check_pa_cards(void)
{
	char	buf[BUF_LEN];

	printf("\n" C_BOLD "PulseAudio Cards" C_RST "\n");
	if (run_cmd("pactl list cards short 2>/dev/null", buf,
			sizeof(buf)) == 0 && buf[0])
		printf("%s", buf);
	else
		printf("  %s No PA cards found\n", SYM_WARN);
}

static void	check_connected_bt(t_app *app)
{
	int	i;
	int	found;

	printf("\n" C_BOLD "Known Bluetooth Devices" C_RST "\n");
	bt_get_devices(app);
	found = 0;
	i = 0;
	while (i < app->device_count)
	{
		printf("  %s %s [%s]", app->devices[i].connected ? SYM_OK : SYM_DOT,
			app->devices[i].name, app->devices[i].mac);
		if (app->devices[i].connected)
			printf(" " C_GRN "(connected)" C_RST);
		if (app->devices[i].paired)
			printf(" (paired)");
		printf("\n");
		found++;
		i++;
	}
	if (!found)
		printf("  %s No Bluetooth devices known\n", SYM_WARN);
}

static void	check_user_config(void)
{
	char	buf[BUF_LEN];

	printf("\n" C_BOLD "User Audio Config" C_RST "\n");
	if (run_cmd("cat ~/.config/pulse/daemon.conf 2>/dev/null", buf,
			sizeof(buf)) == 0 && buf[0])
		printf("  %s ~/.config/pulse/daemon.conf:\n%s\n", SYM_INFO, buf);
	else
		printf("  %s No custom daemon.conf\n", SYM_DOT);
	if (run_cmd("cat ~/.config/pulse/default.pa 2>/dev/null", buf,
			sizeof(buf)) == 0 && buf[0])
		printf("  %s ~/.config/pulse/default.pa exists\n", SYM_INFO);
	else
		printf("  %s No custom default.pa\n", SYM_DOT);
}

void	run_diagnostics(t_app *app)
{
	printf(C_CYN C_BOLD "kuuloBT" C_RST " diagnostics\n");
	check_bluetooth_adapter(app);
	check_audio_server();
	check_bt_modules();
	check_alsa_cards();
	check_pa_sinks();
	check_pa_cards();
	check_connected_bt(app);
	check_user_config();
	printf("\n" C_BOLD "Recommendations" C_RST "\n");
	printf("  %s Run " C_CYN "kuulobt fix" C_RST
		" to attempt automatic fixes\n", SYM_INFO);
	printf("  %s Run " C_CYN "kuulobt connect" C_RST
		" to connect a device\n", SYM_INFO);
	printf("\n");
}

/*
** Attempt to fix common PulseAudio / Bluetooth audio issues
** without requiring sudo.
*/
void	fix_audio_config(void)
{
	char	path[PATH_LEN];
	FILE	*fp;

	printf(C_CYN C_BOLD "kuuloBT" C_RST " applying audio fixes\n\n");
	/* 1. Ensure bluetooth modules are loaded */
	printf("  %s Loading Bluetooth PulseAudio modules...\n", SYM_INFO);
	run_cmd_silent("pactl load-module module-bluetooth-policy 2>/dev/null");
	run_cmd_silent("pactl load-module module-bluetooth-discover 2>/dev/null");
	run_cmd_silent("pactl load-module module-bluez5-discover 2>/dev/null");
	printf("  %s Bluetooth modules loaded\n", SYM_OK);
	/* 2. Create/update user PulseAudio config for better BT support */
	snprintf(path, sizeof(path), "%s/.config/pulse", getenv("HOME"));
	run_cmd_silent("mkdir -p ~/.config/pulse 2>/dev/null");
	/* daemon.conf */
	snprintf(path, sizeof(path), "%s/.config/pulse/daemon.conf",
		getenv("HOME"));
	fp = fopen(path, "w");
	if (fp)
	{
		fprintf(fp, "# Generated by kuuloBT\n");
		fprintf(fp, "avoid-resampling = true\n");
		fprintf(fp, "default-sample-rate = 48000\n");
		fprintf(fp, "alternate-sample-rate = 44100\n");
		fprintf(fp, "default-fragments = 5\n");
		fprintf(fp, "default-fragment-size-msec = 25\n");
		fprintf(fp, "resample-method = speex-float-3\n");
		fclose(fp);
		printf("  %s Updated daemon.conf\n", SYM_OK);
	}
	/* 3. Restart PulseAudio (user-level, no sudo needed) */
	printf("  %s Restarting PulseAudio...\n", SYM_INFO);
	run_cmd_silent("pulseaudio -k 2>/dev/null");
	sleep(1);
	run_cmd_silent("pulseaudio --start --log-target=syslog 2>/dev/null");
	sleep(2);
	printf("  %s PulseAudio restarted\n", SYM_OK);
	/* 4. Re-load bluetooth modules after restart */
	run_cmd_silent("pactl load-module module-bluetooth-policy 2>/dev/null");
	run_cmd_silent("pactl load-module module-bluetooth-discover 2>/dev/null");
	run_cmd_silent("pactl load-module module-bluez5-discover 2>/dev/null");
	printf("  %s Bluetooth modules re-loaded\n", SYM_OK);
	printf("\n  %s Fixes applied. Run " C_CYN "kuulobt connect"
		C_RST " next.\n\n", SYM_OK);
}

void	show_status(t_app *app)
{
	char	buf[BUF_LEN];
	int		i;
	int		found;

	printf(C_CYN C_BOLD "kuuloBT" C_RST " status\n\n");
	/* Audio */
	if (run_cmd("pactl info 2>/dev/null | grep 'Default Sink:'", buf,
			sizeof(buf)) == 0)
	{
		if (strstr(buf, "bluez"))
			printf("  %s Audio: " C_GRN "Bluetooth sink active" C_RST "\n",
				SYM_OK);
		else if (strstr(buf, "auto_null") || strstr(buf, "null"))
			printf("  %s Audio: " C_RED "Dummy output (no audio)"
				C_RST "\n", SYM_WARN);
		else
			printf("  %s Audio: %s", SYM_INFO, buf);
	}
	/* BT devices */
	bt_get_devices(app);
	found = 0;
	i = 0;
	while (i < app->device_count)
	{
		if (app->devices[i].connected)
		{
			printf("  %s Connected: " C_GRN "%s" C_RST " [%s]\n",
				SYM_OK, app->devices[i].name, app->devices[i].mac);
			found++;
		}
		i++;
	}
	if (!found)
		printf("  %s No Bluetooth devices connected\n", SYM_WARN);
	printf("\n");
}
