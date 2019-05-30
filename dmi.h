#ifndef _DMI_H
#define _DMI_H 1

#include <stdint.h>

struct dmi_entry;
struct dmi_entry
{
	struct dmi_entry *allnodes_next;
	int id;
	char *designation;
//	char *busaddress;
	uint16_t busaddress_system;
	uint8_t busaddress_bus;
	uint8_t busaddress_device;
	uint8_t busaddress_function;
	int matched;
};

extern struct dmi_entry *dmi_allnodes;

int dmidecode_main (void);

#endif
