/* ************************************************************************** */
/*                          bluetooth.c                                       */
/*   BlueZ D-Bus interface for Bluetooth management                           */
/* ************************************************************************** */

#include "kuulobt.h"

int	bt_init(t_app *app)
{
	DBusError	err;

	dbus_error_init(&err);
	app->dbus = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err))
	{
		fprintf(stderr, "  %s D-Bus error: %s\n", SYM_FAIL, err.message);
		dbus_error_free(&err);
		return (-1);
	}
	if (!app->dbus)
	{
		fprintf(stderr, "  %s Failed to connect to D-Bus\n", SYM_FAIL);
		return (-1);
	}
	return (0);
}

void	bt_cleanup(t_app *app)
{
	if (app->dbus)
	{
		dbus_connection_unref(app->dbus);
		app->dbus = NULL;
	}
}

/*
** Call a simple method on a BlueZ device object (Connect, Disconnect, etc.)
** Returns 0 on success, -1 on failure.
*/
static int	bt_call_method(t_app *app, const char *path,
	const char *iface, const char *method)
{
	DBusMessage		*msg;
	DBusMessage		*reply;
	DBusError		err;

	dbus_error_init(&err);
	msg = dbus_message_new_method_call("org.bluez", path, iface, method);
	if (!msg)
		return (-1);
	reply = dbus_connection_send_with_reply_and_block(
			app->dbus, msg, 30000, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err))
	{
		if (app->verbose)
			fprintf(stderr, "  %s %s failed: %s\n",
				SYM_FAIL, method, err.message);
		dbus_error_free(&err);
		return (-1);
	}
	if (reply)
		dbus_message_unref(reply);
	return (0);
}

/*
** Read a string property from a BlueZ device via D-Bus.
*/
static int	bt_get_str_prop(t_app *app, const char *path,
	const char *prop, char *out, int out_len)
{
	DBusMessage			*msg;
	DBusMessage			*reply;
	DBusError			err;
	DBusMessageIter		args;
	DBusMessageIter		variant;
	const char			*iface;
	const char			*val;

	iface = "org.bluez.Device1";
	dbus_error_init(&err);
	msg = dbus_message_new_method_call("org.bluez", path,
			"org.freedesktop.DBus.Properties", "Get");
	if (!msg)
		return (-1);
	dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface,
		DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);
	reply = dbus_connection_send_with_reply_and_block(
			app->dbus, msg, 5000, &err);
	dbus_message_unref(msg);
	if (!reply || dbus_error_is_set(&err))
	{
		if (dbus_error_is_set(&err))
			dbus_error_free(&err);
		return (-1);
	}
	if (!dbus_message_iter_init(reply, &args))
	{
		dbus_message_unref(reply);
		return (-1);
	}
	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_VARIANT)
	{
		dbus_message_unref(reply);
		return (-1);
	}
	dbus_message_iter_recurse(&args, &variant);
	if (dbus_message_iter_get_arg_type(&variant) != DBUS_TYPE_STRING)
	{
		dbus_message_unref(reply);
		return (-1);
	}
	dbus_message_iter_get_basic(&variant, &val);
	snprintf(out, out_len, "%s", val);
	dbus_message_unref(reply);
	return (0);
}

/*
** Read a boolean property from a BlueZ device via D-Bus.
*/
static int	bt_get_bool_prop(t_app *app, const char *path,
	const char *prop)
{
	DBusMessage			*msg;
	DBusMessage			*reply;
	DBusError			err;
	DBusMessageIter		args;
	DBusMessageIter		variant;
	const char			*iface;
	dbus_bool_t			val;

	iface = "org.bluez.Device1";
	dbus_error_init(&err);
	msg = dbus_message_new_method_call("org.bluez", path,
			"org.freedesktop.DBus.Properties", "Get");
	if (!msg)
		return (-1);
	dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface,
		DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);
	reply = dbus_connection_send_with_reply_and_block(
			app->dbus, msg, 5000, &err);
	dbus_message_unref(msg);
	if (!reply || dbus_error_is_set(&err))
	{
		if (dbus_error_is_set(&err))
			dbus_error_free(&err);
		return (-1);
	}
	if (!dbus_message_iter_init(reply, &args))
	{
		dbus_message_unref(reply);
		return (-1);
	}
	dbus_message_iter_recurse(&args, &variant);
	if (dbus_message_iter_get_arg_type(&variant) != DBUS_TYPE_BOOLEAN)
	{
		dbus_message_unref(reply);
		return (-1);
	}
	dbus_message_iter_get_basic(&variant, &val);
	dbus_message_unref(reply);
	return (val ? 1 : 0);
}

int	bt_get_device_info(t_app *app, t_bt_device *dev)
{
	bt_get_str_prop(app, dev->path, "Name", dev->name, NAME_LEN);
	bt_get_str_prop(app, dev->path, "Alias", dev->name, NAME_LEN);
	dev->paired = bt_get_bool_prop(app, dev->path, "Paired");
	dev->trusted = bt_get_bool_prop(app, dev->path, "Trusted");
	dev->connected = bt_get_bool_prop(app, dev->path, "Connected");
	return (0);
}

int	bt_start_discovery(t_app *app)
{
	return (bt_call_method(app, "/org/bluez/hci0",
		"org.bluez.Adapter1", "StartDiscovery"));
}

int	bt_stop_discovery(t_app *app)
{
	return (bt_call_method(app, "/org/bluez/hci0",
		"org.bluez.Adapter1", "StopDiscovery"));
}

int	bt_scan(t_app *app, int duration_sec)
{
	if (duration_sec <= 0)
		duration_sec = 10;
	printf("  %s Starting Bluetooth scan (%ds)...\n", SYM_INFO, duration_sec);
	bt_start_discovery(app);
	progress_dots("Scanning", duration_sec);
	bt_stop_discovery(app);
	printf("  %s Scan complete\n", SYM_OK);
	return (bt_get_devices(app));
}

/*
** Use GetManagedObjects to enumerate all BlueZ device paths, then
** read properties for each to populate app->devices[].
*/
int	bt_get_devices(t_app *app)
{
	DBusMessage			*msg;
	DBusMessage			*reply;
	DBusError			err;
	DBusMessageIter		root;
	DBusMessageIter		dict;
	DBusMessageIter		entry;
	const char			*path;

	app->device_count = 0;
	dbus_error_init(&err);
	msg = dbus_message_new_method_call("org.bluez", "/",
			"org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
	if (!msg)
		return (-1);
	reply = dbus_connection_send_with_reply_and_block(
			app->dbus, msg, 10000, &err);
	dbus_message_unref(msg);
	if (!reply || dbus_error_is_set(&err))
	{
		if (dbus_error_is_set(&err))
			dbus_error_free(&err);
		return (-1);
	}
	if (!dbus_message_iter_init(reply, &root))
	{
		dbus_message_unref(reply);
		return (-1);
	}
	dbus_message_iter_recurse(&root, &dict);
	while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY)
	{
		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &path);
		if (strstr(path, "/org/bluez/hci0/dev_")
			&& app->device_count < MAX_DEVICES)
		{
			memset(&app->devices[app->device_count], 0, sizeof(t_bt_device));
			snprintf(app->devices[app->device_count].path, PATH_LEN,
				"%s", path);
			path_to_mac(path, app->devices[app->device_count].mac);
			bt_get_device_info(app, &app->devices[app->device_count]);
			app->device_count++;
		}
		dbus_message_iter_next(&dict);
	}
	dbus_message_unref(reply);
	return (app->device_count);
}

int	bt_pair(t_app *app, const char *path)
{
	printf("  %s Pairing with device...\n", SYM_INFO);
	if (bt_call_method(app, path, "org.bluez.Device1", "Pair") < 0)
	{
		printf("  %s Pairing failed (may already be paired)\n", SYM_WARN);
		return (-1);
	}
	printf("  %s Paired successfully\n", SYM_OK);
	return (0);
}

int	bt_trust(t_app *app, const char *path)
{
	DBusMessage		*msg;
	DBusMessage		*reply;
	DBusError		err;
	const char		*iface;
	const char		*prop;
	dbus_bool_t		val;
	DBusMessageIter	args;
	DBusMessageIter	variant;

	iface = "org.bluez.Device1";
	prop = "Trusted";
	val = TRUE;
	dbus_error_init(&err);
	msg = dbus_message_new_method_call("org.bluez", path,
			"org.freedesktop.DBus.Properties", "Set");
	if (!msg)
		return (-1);
	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop);
	dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT,
		DBUS_TYPE_BOOLEAN_AS_STRING, &variant);
	dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &val);
	dbus_message_iter_close_container(&args, &variant);
	reply = dbus_connection_send_with_reply_and_block(
			app->dbus, msg, 5000, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err))
	{
		dbus_error_free(&err);
		return (-1);
	}
	if (reply)
		dbus_message_unref(reply);
	printf("  %s Device trusted\n", SYM_OK);
	return (0);
}

int	bt_connect(t_app *app, const char *path)
{
	printf("  %s Connecting...\n", SYM_INFO);
	if (bt_call_method(app, path, "org.bluez.Device1", "Connect") < 0)
	{
		printf("  %s Connection failed\n", SYM_FAIL);
		return (-1);
	}
	printf("  %s Connected successfully\n", SYM_OK);
	return (0);
}

int	bt_disconnect(t_app *app, const char *path)
{
	printf("  %s Disconnecting...\n", SYM_INFO);
	if (bt_call_method(app, path, "org.bluez.Device1", "Disconnect") < 0)
	{
		printf("  %s Disconnect failed\n", SYM_FAIL);
		return (-1);
	}
	printf("  %s Disconnected\n", SYM_OK);
	return (0);
}

int	bt_remove(t_app *app, const char *path)
{
	DBusMessage		*msg;
	DBusMessage		*reply;
	DBusError		err;

	printf("  %s Removing device pairing...\n", SYM_INFO);
	dbus_error_init(&err);
	msg = dbus_message_new_method_call("org.bluez", "/org/bluez/hci0",
			"org.bluez.Adapter1", "RemoveDevice");
	if (!msg)
		return (-1);
	dbus_message_append_args(msg, DBUS_TYPE_OBJECT_PATH, &path,
		DBUS_TYPE_INVALID);
	reply = dbus_connection_send_with_reply_and_block(
			app->dbus, msg, 10000, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err))
	{
		printf("  %s Remove failed: %s\n", SYM_FAIL, err.message);
		dbus_error_free(&err);
		return (-1);
	}
	if (reply)
		dbus_message_unref(reply);
	printf("  %s Device removed\n", SYM_OK);
	return (0);
}
