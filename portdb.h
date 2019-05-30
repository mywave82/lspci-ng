#ifndef PORTDB_H
#define PORTDB_H 1

struct portdb_port
{
	uint8_t busaddress_bus;
	uint8_t busaddress_device;
	uint8_t busaddress_function;

	const char *name;
};

struct portdb_mainboard
{	
	uint16_t vendor;
	uint16_t device;

	uint16_t subsystem_vendor;
	uint16_t subsystem_device;

	const char *vendor_label;
	const char *name_label;

	/* these might be allocated dynamically for special mainboards */
	struct portdb_port ports[32]; /* this number we can increase if needed */
	int                ports_n;
};

void apply_portdb(void);

#endif
