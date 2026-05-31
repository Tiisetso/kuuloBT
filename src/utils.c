/* ************************************************************************** */
/*                          utils.c                                           */
/*   Utility functions for kuuloBT                                            */
/* ************************************************************************** */

#include "kuulobt.h"

int	run_cmd(const char *cmd, char *out, int out_len)
{
	FILE	*fp;
	int		status;
	int		total;
	int		n;

	fp = popen(cmd, "r");
	if (!fp)
		return (-1);
	total = 0;
	if (out && out_len > 0)
	{
		out[0] = '\0';
		while (total < out_len - 1
			&& (n = fread(out + total, 1, out_len - 1 - total, fp)) > 0)
			total += n;
		out[total] = '\0';
	}
	status = pclose(fp);
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	return (-1);
}

int	run_cmd_silent(const char *cmd)
{
	char	buf[BUF_LEN];

	return (run_cmd(cmd, buf, sizeof(buf)));
}

void	print_help(void)
{
	printf(C_CYN C_BOLD "kuuloBT" C_RST
		C_DIM " — Bluetooth audio for Linux\n" C_RST "\n");
	printf(C_BOLD "Usage:" C_RST " kuulobt <command> [options]\n\n");
	printf(C_BOLD "Commands:\n" C_RST);
	printf("  " C_CYN "scan" C_RST "          Scan and select a device to connect\n");
	printf("  " C_CYN "connect" C_RST "       Connect to a device (interactive or by MAC)\n");
	printf("  " C_CYN "disconnect" C_RST "    Disconnect current device\n");
	printf("  " C_CYN "status" C_RST "        Show connection and audio status\n");
	printf("  " C_CYN "diagnose" C_RST "      Run full system diagnostics\n");
	printf("  " C_CYN "fix" C_RST "           Attempt to fix common audio issues\n");
	printf("  " C_CYN "reset" C_RST "         Remove device pairing and start fresh\n");
	printf("  " C_CYN "help" C_RST "          Show this help message\n\n");
	printf(C_BOLD "Options:\n" C_RST);
	printf("  " C_YEL "-m" C_RST " MAC        Specify device MAC address\n");
	printf("  " C_YEL "-v" C_RST "            Verbose output\n");
	printf("  " C_YEL "-t" C_RST " SEC        Scan duration in seconds (default: 10)\n\n");
	printf(C_BOLD "Examples:\n" C_RST);
	printf("  kuulobt scan\n");
	printf("  kuulobt connect\n");
	printf("  kuulobt connect -m AA:BB:CC:DD:EE:FF\n");
	printf("  kuulobt diagnose\n\n");
}

void	mac_to_path(const char *mac, char *path)
{
	char	mac_underscored[MAC_LEN];
	int		i;

	i = 0;
	while (mac[i])
	{
		if (mac[i] == ':')
			mac_underscored[i] = '_';
		else
			mac_underscored[i] = mac[i];
		i++;
	}
	mac_underscored[i] = '\0';
	snprintf(path, PATH_LEN, "/org/bluez/hci0/dev_%s", mac_underscored);
}

void	path_to_mac(const char *path, char *mac)
{
	const char	*dev;
	int			i;

	dev = strstr(path, "dev_");
	if (!dev)
	{
		mac[0] = '\0';
		return ;
	}
	dev += 4;
	i = 0;
	while (dev[i] && i < MAC_LEN - 1)
	{
		if (dev[i] == '_')
			mac[i] = ':';
		else
			mac[i] = dev[i];
		i++;
	}
	mac[i] = '\0';
}

void	progress_dots(const char *msg, int seconds)
{
	int	i;

	printf("  %s %s ", SYM_INFO, msg);
	fflush(stdout);
	i = 0;
	while (i < seconds)
	{
		printf(".");
		fflush(stdout);
		sleep(1);
		i++;
	}
	printf("\n");
}
