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
		return -1;
	}
	fscanf(file, "%d", &ret);
	fclose(file);
	return ret;
}

static int get_battery_percent(int battery_id) {
	int energy_now, energy_full;
	char path[256];

	snprintf(path, sizeof path, "/sys/class/power_supply/BAT%i/energy_now", battery_id);
	energy_now = freadint(path);

	snprintf(path, sizeof path, "/sys/class/power_supply/BAT%i/energy_full", battery_id);
	energy_full = freadint(path);

	return lrint(energy_now * 100.0 / energy_full);
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
		len += snprintf(status + len, sizeof status, " %d%% %d%% | ",
				get_battery_percent(0),
				get_battery_percent(1));

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
