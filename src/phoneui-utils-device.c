
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

static const char *device_vibrator = NULL;

struct VibrationData {
	int fd;
	int duration;
	int repeat;
	int pause;
	char systring[5];
};


static int
_vibration_on(gpointer data);
static int
_vibration_off(gpointer data);

int
phoneui_utils_device_init(GKeyFile *keyfile)
{
	const char *vibrator =
		g_key_file_get_string(keyfile, "device", "vibrator", NULL);
	if (vibrator) {
		g_debug("using %s for vibration", vibrator);
		device_vibrator = strdup(vibrator);
	}
	else {
		g_message("no vibrator configured - turning vibration off");
	}
	return 0;
}

static int
_vibration_on(gpointer data)
{
	struct VibrationData *vdata = (struct VibrationData *)data;
	ssize_t len = write(vdata->fd, vdata->systring, strlen(vdata->systring));
	g_assert(len != -1);
	g_timeout_add(vdata->duration, _vibration_off, vdata);
	return 0;
}

static int
_vibration_off(gpointer data)
{
	struct VibrationData *vdata = (struct VibrationData *)data;
	ssize_t len = write(vdata->fd, "0\n", 2);
	g_assert(len != -1);
	if (vdata->repeat > 0) {
		vdata->repeat--;
		g_timeout_add(vdata->pause, _vibration_on, vdata);
	}
	else {
		close(vdata->fd);
		free(vdata);
	}
	return 0;
}


void
phoneui_utils_device_vibrate(int duration, int intensity, int repeat, int pause)
{
	int fd;

	if (!device_vibrator)
		return;

	fd = open(device_vibrator, O_WRONLY);
	if (fd == -1) {
		g_warning("vibrate error %d: %s", errno, strerror(errno));
		return;
	}
	struct VibrationData *vdata = malloc(sizeof(struct VibrationData));
	vdata->fd = fd;
	vdata->duration = duration;
	vdata->repeat = repeat;
	vdata->pause = pause;
	snprintf(vdata->systring, 4, "%d\n", intensity);
	ssize_t len = write(fd, vdata->systring, strlen(vdata->systring));
	g_assert(len != -1);
	g_timeout_add(duration, _vibration_off, vdata);
}



void
phoneui_utils_device_flash(int duration, int intensity, int repeat, int pause)
{
	(void)duration;
	(void)intensity;
	(void)repeat;
	(void)pause;
	// TODO
}


void
phoneui_utils_device_sound(const char *sound)
{
	(void)sound;
	// TODO
}


void
phoneui_utils_device_activate_screensaver(void)
{
	// FIXME: do this in a sane way!!!
	int rc1 = system("xset -display localhost:0 s blank");
	int rc2 = system("xset -display localhost:0 s activate");
	g_assert(rc1 == 0 && rc2 ==0);
}

void
phoneui_utils_device_deactivate_screensaver(void)
{
	// FIXME: do this in a sane way!!!
	int rc = system("xset -display localhost:0 s reset");
	g_assert(rc == 0);
}

