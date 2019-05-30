#ifndef _LSPCI_H
#define _LSPCI_H 1

#include <stdint.h>

struct dmi_entry;
struct nvidia_smi_entry;
struct lspci_entry;
struct portdb_entry;

struct lspci_entry
{
	struct lspci_entry *allnodes_next; /* global iterator */
	struct lspci_entry *bus_next; /* bus iterator */
	struct lspci_entry *bus_child; /* first child */

	uint16_t busaddress_domain;
	uint8_t busaddress_bus;
	uint8_t busaddress_device;
	uint8_t busaddress_function;

	uint16_t vendor;
	uint16_t device;
	char *name;

	uint16_t subsystem_vendor;
	uint16_t subsystem_device;
	/* we ignore subsystem_name, since we only need these ID for database lookups */

	struct portdb_mainboard *mainboard;
	struct portdb_port      *port;
	struct dmi_entry        *dmi;
	struct nvidia_smi_entry *nvidia_smi;

	/* for PCI-bus */
	uint8_t secondary_bus;
	uint8_t subordinate_bus;

	char *port_type;
};

extern struct lspci_entry *lspci_allnodes;
extern struct lspci_entry *lspci_rootbus;

int lspci_main(void);
void print_pcibuses (void);


#endif
