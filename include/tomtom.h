#ifndef __TOMTOM_H__
#define __TOMTOM_H__

/* is memset to 0 in start_armboot */
struct tomtom_global_data {
	unsigned long sysboot_mode;
	unsigned long bootcount;
};

#define SYSBOOT_MODE_UNKNOWN	0
#define SYSBOOT_MODE_COLD	1
#define SYSBOOT_MODE_WARM	2
#define SYSBOOT_MODE_WATCHDOG	3

#define SYSBOOT_MODE_STR_COLD		"cold"
#define SYSBOOT_MODE_STR_WARM		"warm"
#define SYSBOOT_MODE_STR_WATCHDOG	"watchdog"

#endif
