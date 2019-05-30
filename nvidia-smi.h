#ifndef _NVIDIA_H
#define _NVIDIA_H 1

struct nvidia_smi_entry;
struct nvidia_smi_entry
{
	struct nvidia_smi_entry *allnodes_next;
	int id;
	char *name;
	char *uuid;
	uint8_t busaddress_bus;
	uint8_t busaddress_device;
	uint8_t busaddress_function;
};

extern struct nvidia_smi_entry *nvidia_smi_allnodes;

int nvidia_smi_main (void);

#endif
