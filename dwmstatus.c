#define _DEFAULT_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef enum {
	NONE,
	UNKNOWN,
	DISCHARGING,
	CHARGING
} battery_status_t;

/* Open a file and read an integer */
static battery_status_t fread_battery_status(const char *filename){
	char buf[16];
	FILE *file = fopen(filename, "r");
	if(!file) {
		fprintf(stderr, "Error opening %s.\n", filename);
		return NONE;
	}
	fgets(buf, 16, file);
	fclose(file);
	if(!strcmp(buf, "Charging\n")) {
		return CHARGING;
	} else if(!strcmp(buf, "Discharging\n")) {
		return DISCHARGING;
	} else {
		return UNKNOWN;
	}
}

typedef struct {
	int energy_now;
	int energy_full;
	int power_now;
	battery_status_t status;
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

	snprintf(path, sizeof path, "/sys/class/power_supply/BAT%i/status", battery_id);
	info.status = fread_battery_status(path);

	return info;
}

static int battery_get_percent(battery_info_t bat) {
	if(bat.energy_full) {
		return lrint(bat.energy_now * 100.0 / bat.energy_full);
	} else {
		return 0;
	}
}

static int battery_get_time_remaining(battery_info_t bat, int power_now) {
	return lrint(bat.energy_now * 60.0 / power_now);
}

static int status_battery(char *status, int max) {
	int len = 0;
	battery_info_t bat0 = battery_get_info(0);
	battery_info_t bat1 = battery_get_info(1);

	int power_now = bat0.power_now ? bat0.power_now : bat1.power_now;

	len += snprintf(status + len, max - len, "%d%% %d%% ",
			battery_get_percent(bat0),
			battery_get_percent(bat1));

	int charging = bat0.status == CHARGING || bat1.status == CHARGING;
	int minutes_remaining = battery_get_time_remaining(bat0, power_now) + battery_get_time_remaining(bat1, power_now);

	if(charging) {
		len += snprintf(status + len, max - len, "Charging");
	} else {
		len += snprintf(status + len, max - len, "%d:%.2d",
				minutes_remaining / 60, minutes_remaining % 60);
	}

	return len;
}

static int status_separator(char *status, int max) {
	return snprintf(status, max, " | ");
}

static int status_time(char *status, int max) {
	time_t current_time = time(NULL);
	struct tm *current_tm = localtime(&current_time);

	return strftime(status, max, "%F %H:%M", current_tm);
}

static int status_load(char *status, int max) {
	double loadavg[3];
	getloadavg(loadavg, 3);

	return snprintf(status, max, "%.2f %.2f %.2f", loadavg[0], loadavg[1], loadavg[2]);
}

typedef int (*status_func_t)(char *, int);

static status_func_t status_segments[] = {
	status_load,
	status_separator,
	status_battery,
	status_separator,
	status_time,
	NULL
};

int main(void) {
	static Display *dpy;
	char status[256];

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Cannot open display.\n");
		return 1;
	}

	for(;;) {
		int len = 0;
		int max = sizeof status;

		len += snprintf(status, max, " ");

		for(int i = 0; status_segments[i]; i++) {
			len += status_segments[i](status + len, max - len);
		}

		XStoreName(dpy, DefaultRootWindow(dpy), status);
		XFlush(dpy);

		sleep(5);
	}

	/* FIXME: unreachable */
	XCloseDisplay(dpy);

	return 0;
}
