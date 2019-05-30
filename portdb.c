#include <stdio.h>
#include "lspci.h"
#include "main.h"
#include "portdb.h"

struct portdb_mainboard static_mainboards[] = {
#include "portdb-static.c"
};

static void apply_portdb_port (struct portdb_port *port)
{
	struct lspci_entry *iter;

	if (debug)
	{
		printf ("Going to search for PCI entry for DB entry %02x:%02x.%x : ", port->busaddress_bus, port->busaddress_device, port->busaddress_function);
	}
	for (iter=lspci_allnodes; iter; iter = iter->allnodes_next)
	{
		if ((iter->busaddress_bus      == port->busaddress_bus) &&
		    (iter->busaddress_device   == port->busaddress_device) &&
		    (iter->busaddress_function == port->busaddress_function))
		{
			iter->port = port;
			if (debug)
			{
				printf ("FOUND IT\n");
			}
			return;
		}
	}
	if (debug)
	{
		printf ("Failed\n");
	}

}

void apply_portdb(void)
{
	int i;

	if (!lspci_rootbus)
	{
		return;
	}

	for (i=0; i < (sizeof(static_mainboards)/sizeof(static_mainboards[0])); i++)
	{
		if ((lspci_rootbus->vendor           == static_mainboards[i].vendor) &&
		    (lspci_rootbus->device           == static_mainboards[i].device) &&
		    (lspci_rootbus->subsystem_vendor == static_mainboards[i].subsystem_vendor) &&
		    (lspci_rootbus->subsystem_device == static_mainboards[i].subsystem_device))
		{
			int j;
			lspci_rootbus->mainboard = &static_mainboards[i];
			for (j=0; j < lspci_rootbus->mainboard->ports_n; j++)
			{
				apply_portdb_port(&lspci_rootbus->mainboard->ports[j]);
			}
			return;
		}
	}
	/* no entry found */
}
