/* ************************************************************************** */
/*                          audio.c                                           */
/*   PulseAudio integration for Bluetooth audio routing                       */
/* ************************************************************************** */

#include "blueconnect.h"

/* ── PulseAudio callbacks ───────────────────────────────────────────────── */

static void	pa_state_cb(pa_context *ctx, void *userdata)
{
	t_pa_state	*pa;

	pa = (t_pa_state *)userdata;
	switch (pa_context_get_state(ctx))
	{
		case PA_CONTEXT_READY:
			pa->ready = 1;
			break ;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			pa->ready = -1;
			break ;
		default:
			break ;
	}
}

static void	pa_sink_info_cb(pa_context *ctx, const pa_sink_info *info,
	int eol, void *userdata)
{
	t_pa_state	*pa;

	(void)ctx;
	pa = (t_pa_state *)userdata;
	if (eol > 0 || !info)
	{
		pa->done = 1;
		return ;
	}
	if (strstr(info->name, "bluez") || strstr(info->name, "bluetooth"))
	{
		snprintf(pa->bt_sink, NAME_LEN, "%s", info->name);
		printf("  %s Found BT audio sink: " C_CYN "%s" C_RST "\n",
			SYM_OK, info->description);
	}
}

static void	pa_card_info_cb(pa_context *ctx, const pa_card_info *info,
	int eol, void *userdata)
{
	t_pa_state			*pa;
	pa_card_profile_info	*profile;
	uint32_t			j;

	(void)ctx;
	pa = (t_pa_state *)userdata;
	if (eol > 0 || !info)
	{
		pa->done = 1;
		return ;
	}
	if (strstr(info->name, "bluez"))
	{
		snprintf(pa->bt_card, NAME_LEN, "%s", info->name);
		pa->bt_card_index = info->index;
		printf("  %s Found BT audio card: " C_CYN "%s" C_RST "\n",
			SYM_OK, info->name);
		printf("    Active profile: %s\n",
			info->active_profile ? info->active_profile->name : "none");
		j = 0;
		while (j < info->n_profiles)
		{
			profile = &info->profiles[j];
			printf("    %s Available: %s (%s)\n", SYM_DOT,
				profile->name, profile->description);
			j++;
		}
	}
}

static void	pa_success_cb(pa_context *ctx, int success, void *userdata)
{
	t_pa_state	*pa;

	(void)ctx;
	pa = (t_pa_state *)userdata;
	pa->done = success ? 1 : -1;
}

/* ── Wait for PulseAudio operation to complete ──────────────────────────── */

static void	pa_wait_op(t_pa_state *pa)
{
	int	count;

	count = 0;
	while (!pa->done && count < 100)
	{
		pa_mainloop_iterate(pa->ml, 0, NULL);
		usleep(50000);
		count++;
	}
}

/* ── Public functions ───────────────────────────────────────────────────── */

int	pa_setup(t_app *app)
{
	int	count;

	memset(&app->pa, 0, sizeof(t_pa_state));
	app->pa.bt_card_index = -1;
	app->pa.ml = pa_mainloop_new();
	if (!app->pa.ml)
		return (-1);
	app->pa.api = pa_mainloop_get_api(app->pa.ml);
	app->pa.ctx = pa_context_new(app->pa.api, "blueconnect");
	if (!app->pa.ctx)
	{
		pa_mainloop_free(app->pa.ml);
		return (-1);
	}
	pa_context_set_state_callback(app->pa.ctx, pa_state_cb, &app->pa);
	if (pa_context_connect(app->pa.ctx, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0)
	{
		fprintf(stderr, "  %s Cannot connect to PulseAudio\n", SYM_FAIL);
		pa_context_unref(app->pa.ctx);
		pa_mainloop_free(app->pa.ml);
		return (-1);
	}
	count = 0;
	while (app->pa.ready == 0 && count < 100)
	{
		pa_mainloop_iterate(app->pa.ml, 0, NULL);
		usleep(50000);
		count++;
	}
	if (app->pa.ready != 1)
	{
		fprintf(stderr, "  %s PulseAudio not ready\n", SYM_FAIL);
		pa_context_unref(app->pa.ctx);
		pa_mainloop_free(app->pa.ml);
		return (-1);
	}
	return (0);
}

void	pa_teardown(t_app *app)
{
	if (app->pa.ctx)
	{
		pa_context_disconnect(app->pa.ctx);
		pa_context_unref(app->pa.ctx);
	}
	if (app->pa.ml)
		pa_mainloop_free(app->pa.ml);
	memset(&app->pa, 0, sizeof(t_pa_state));
}

int	pa_find_bt_sink(t_app *app)
{
	pa_operation	*op;

	app->pa.done = 0;
	app->pa.bt_sink[0] = '\0';
	op = pa_context_get_sink_info_list(app->pa.ctx,
			pa_sink_info_cb, &app->pa);
	if (op)
	{
		pa_wait_op(&app->pa);
		pa_operation_unref(op);
	}
	if (app->pa.bt_sink[0] == '\0')
	{
		printf("  %s No Bluetooth audio sink found\n", SYM_WARN);
		return (-1);
	}
	return (0);
}

int	pa_set_profile_a2dp(t_app *app)
{
	pa_operation	*op;
	const char		*profiles[] = {
		"a2dp-sink-aac",
		"a2dp-sink-sbc",
		"a2dp-sink",
		"a2dp_sink",
		NULL
	};
	int				i;

	/* First discover the card */
	app->pa.done = 0;
	app->pa.bt_card[0] = '\0';
	op = pa_context_get_card_info_list(app->pa.ctx,
			pa_card_info_cb, &app->pa);
	if (op)
	{
		pa_wait_op(&app->pa);
		pa_operation_unref(op);
	}
	if (app->pa.bt_card[0] == '\0')
	{
		printf("  %s No Bluetooth audio card found in PulseAudio\n", SYM_WARN);
		return (-1);
	}
	/* Try each A2DP profile name variant */
	i = 0;
	while (profiles[i])
	{
		printf("  %s Trying profile: %s\n", SYM_INFO, profiles[i]);
		app->pa.done = 0;
		op = pa_context_set_card_profile_by_name(app->pa.ctx,
				app->pa.bt_card, profiles[i], pa_success_cb, &app->pa);
		if (op)
		{
			pa_wait_op(&app->pa);
			pa_operation_unref(op);
			if (app->pa.done == 1)
			{
				printf("  %s Set profile to " C_GRN "%s" C_RST "\n",
					SYM_OK, profiles[i]);
				return (0);
			}
		}
		i++;
	}
	printf("  %s Could not set A2DP profile\n", SYM_WARN);
	return (-1);
}

int	pa_set_default_sink(t_app *app)
{
	pa_operation	*op;

	if (app->pa.bt_sink[0] == '\0')
	{
		if (pa_find_bt_sink(app) < 0)
			return (-1);
	}
	printf("  %s Setting default sink to BT device...\n", SYM_INFO);
	app->pa.done = 0;
	op = pa_context_set_default_sink(app->pa.ctx, app->pa.bt_sink,
			pa_success_cb, &app->pa);
	if (op)
	{
		pa_wait_op(&app->pa);
		pa_operation_unref(op);
		if (app->pa.done == 1)
		{
			printf("  %s Default sink set to " C_GRN "%s" C_RST "\n",
				SYM_OK, app->pa.bt_sink);
			return (0);
		}
	}
	printf("  %s Failed to set default sink\n", SYM_FAIL);
	return (-1);
}

int	pa_test_audio(t_app *app)
{
	char	cmd[BUF_LEN];

	(void)app;
	printf("  %s Playing test tone (2 seconds)...\n", SYM_INFO);
	snprintf(cmd, sizeof(cmd),
		"paplay --channels=2 --rate=44100 --format=s16le /dev/urandom "
		"2>/dev/null &"
		" PID=$!; sleep 2; kill $PID 2>/dev/null; wait $PID 2>/dev/null");
	if (run_cmd_silent(cmd) == 0)
	{
		printf("  %s Test tone sent\n", SYM_OK);
		return (0);
	}
	printf("  %s Test tone failed\n", SYM_WARN);
	return (-1);
}
