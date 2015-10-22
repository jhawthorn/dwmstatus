#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>

/* Open a file and read an integer */
static int freadint(const char *filename){
	int ret;
	FILE *file = fopen(filename, "r");
	if(!file) {
		fprintf(stderr, "Error opening %s.\n", filename);
		return 0;
	}
	fscanf(file, "%d", &ret);
	fclose(file);
	return ret;
}

typedef struct {
	int energy_now;
	int energy_full;
	int power_now;
} battery_info_t;

static battery_info_t battery_get_info(int battery_id) {
	char path[256];

	battery_info_t info;

	snprintf(path, sizeof path, "/sys/class/power_supply/BAT%i/energy_now", battery_id);
	info.energy_now = freadint(path);

	snprintf(path, sizeof path, "/sys/class/power_supply/BAT%i/energy_full", battery_id);
	info.energy_full = freadint(path);

	snprintf(path, sizeof path, "/sys/class/power_supply/BAT%i/power_now", battery_id);
	info.power_now = freadint(path);

	return info;
}

static int battery_get_percent(battery_info_t bat) {
	if(bat.energy_full) {
		return lrint(bat.energy_now * 100.0 / bat.energy_full);
	} else {
		return 0;
	}
}

int main(void) {
	static Display *dpy;
	char status[256];

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Cannot open display.\n");
		return 1;
	}

	int i;
	for (i = 0;; i++) {
		int len = 0;

		battery_info_t bat0 = battery_get_info(0);
		battery_info_t bat1 = battery_get_info(1);

		len += snprintf(status + len, sizeof status, " %d%% %d%% | ",
				battery_get_percent(bat0),
				battery_get_percent(bat1));

		time_t current_time = time(NULL);
		struct tm *current_tm = localtime(&current_time);

		strftime(status + len, sizeof status - len, "%F %H:%M", current_tm);

		XStoreName(dpy, DefaultRootWindow(dpy), status);
		XFlush(dpy);

		sleep(60 - current_tm->tm_sec);
	}

	XCloseDisplay(dpy);

	return 0;
}
