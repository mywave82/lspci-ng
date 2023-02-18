#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "dmi.h"
#include "lspci.h"
#include "nvidia-smi.h"
#include "main.h"
#include "portdb.h"
#include "prun.h"

struct lspci_entry *lspci_allnodes = 0;
struct lspci_entry *lspci_rootbus = 0;

static char *lspci_data = 0;

static void exec_lspci(void)
{
	execlp ("lspci", "lspci", "-vvnnD", NULL);
	perror ("execl(\"lspci\", \"-vvnnD\", 0)");
	_exit (1);
}

static void print_pcibus (const char *prefix_a, const char *prefix_b, struct lspci_entry *node)
{
	struct lspci_entry *iter;
	for (iter = node; iter; iter = iter->bus_next)
	{
		char infoprefix[128];

		printf ("%s", prefix_a);
		printf ("%c-%02x.%x-%s", iter->bus_next?'+':'\\', iter->busaddress_device, iter->busaddress_function, iter->secondary_bus?"+-":"");
		snprintf (infoprefix, sizeof (infoprefix), "%s%c      %s", prefix_b, iter->bus_next?'|':' ', iter->secondary_bus?"| ":"");

		if (iter->port)
		{
 			printf ("\033[92m");
		}
		printf (" %s", iter->name);
#if 0
		if (iter->port)
		{
 			printf ("  \033[93;44m%s \033[0m", iter->port->name);
		}
#else
		if (iter->port)
		{
 			printf (" \033[0m");
		}

#endif
		printf ("\n");
		prefix_a = prefix_b;

		if (iter->port)
		{
			printf ("%s\033[97;44m PCI port designation: %s \033[0m\n", infoprefix, iter->port->name);
		}
		if (iter->dmi)
		{
			printf ("%s\033[93;46m DMI port designation: %s \033[0m\n", infoprefix,  iter->dmi->designation);
		}

		if (iter->port_type && (iter->port || verbose))
		{
			printf ("%s\033[95m Port type: %s \033[0m\n", infoprefix, iter->port_type);
		}

		if (iter->mainboard)
		{
			printf ("%s\033[93;46m Mainboard: %s \033[0m\n", infoprefix, iter->mainboard->name_label);
		}


		if (iter->nvidia_smi)
		{
			printf ("%s\033[93m Nvidia GPU %d: %s \033[0m\n", infoprefix, iter->nvidia_smi->id, /*iter->nvidia_smi->name,*/ iter->nvidia_smi->uuid);
		}

		if (iter->secondary_bus)
		{
			if (iter->bus_child)
			{
				char newprefix_a[128];
				char newprefix_b[128];

				if (iter->secondary_bus != iter->subordinate_bus)
				{
					snprintf (newprefix_a, sizeof (newprefix_a), "%s%c      \\-[%02x-%02x]-", prefix_a, iter->bus_next?'|':' ', iter->secondary_bus, iter->subordinate_bus);
					snprintf (newprefix_b, sizeof (newprefix_b), "%s%c                ",      prefix_a, iter->bus_next?'|':' ');
				} else {
					snprintf (newprefix_a, sizeof (newprefix_a), "%s%c      \\-[%02x]-", prefix_a, iter->bus_next?'|':' ', iter->secondary_bus);
					snprintf (newprefix_b, sizeof (newprefix_b), "%s%c             ",    prefix_a, iter->bus_next?'|':' ');
				}
				print_pcibus (newprefix_a, newprefix_b, iter->bus_child);
			} else {
				if (iter->secondary_bus != iter->subordinate_bus)
				{
					printf ("%s%c      \\-[%02x-%02x]-\n", prefix_a, iter->bus_next?'|':' ', iter->secondary_bus, iter->subordinate_bus);
				} else {
					printf ("%s%c      \\-[%02x]-\n", prefix_a, iter->bus_next?'|':' ', iter->secondary_bus);
				}
			}
		}
		prefix_a = prefix_b;
	}
}

void print_pcibuses (void)
{
	print_pcibus ("", "", lspci_rootbus);
}

static void add_pcinode (struct lspci_entry *e)
{
	int gotbus = e->busaddress_bus == 0; /* we start inside bus 0 */
	int gotbranch = 0;
	struct lspci_entry **prev = &lspci_rootbus;

	if (debug)
	{
		printf ("lspci add_pcinode: Going to add %02x:%02x.%x\n", e->busaddress_bus, e->busaddress_device, e->busaddress_function);
	}

	while (!gotbus)
	{
		if (debug)
		{
			printf ("\tGoing to find bus\n");
		}
		if (!*prev)
		{
			if (debug)
			{
				printf ("\tRan out of nodes\n");
			}
			break;
		}

		if (debug)
		{
			printf ("\tTesting node %02x:%02x.%x [%02x-%02x]\n", (*prev)->busaddress_bus, (*prev)->busaddress_device, (*prev)->busaddress_function, (*prev)->secondary_bus, (*prev)->subordinate_bus);
		}

		if ((*prev)->secondary_bus == e->busaddress_bus)
		{
			if (debug)
			{
				printf ("\tFound the bus!\n");
			}
		 	prev = &((*prev)->bus_child);
			gotbus = 1;
			break;
		}
		if ( ((*prev)->secondary_bus < e->busaddress_bus) && ((*prev)->subordinate_bus >= e->busaddress_bus))
		{
			if (debug)
			{
				printf ("\tFound a branch (subordinate)\n");
			}
			prev = &((*prev)->bus_child);
			gotbranch = 1;
		} else {
			prev = &((*prev)->bus_next);
		}
	}

	while (*prev)
	{
		if (debug)
		{
			printf ("\tSearch for end\n");
		}
		prev = &((*prev)->bus_next);
	}

	if (!gotbus)
	{
		fprintf (stderr, "Didn't find PCI bus %02x", e->busaddress_bus);
		if (gotbranch)
		{
			fprintf(stderr, ", but we found a branch for it");
		}
		fprintf (stderr, "\n");
	}
	*prev = e;
}

static void build_pcibus(void)
{
	struct lspci_entry *iter;
	for (iter = lspci_allnodes; iter; iter = iter->allnodes_next)
	{
		add_pcinode (iter);
	}
}

static void append_lspci(const char *name, uint16_t busaddress_domain, uint8_t busaddress_bus, uint8_t busaddress_device, uint8_t busaddress_function, uint16_t vendor, uint16_t device, uint16_t subsystem_vendor, uint16_t subsystem_device, uint8_t secondary_bus, uint8_t subordinate_bus, const char *port_type)
{
	struct lspci_entry **prev = &lspci_allnodes;
	struct lspci_entry *entry = malloc(sizeof (*entry));

	if (debug)
	{
		printf ("append_lspci: %02x:%02x.%02x ", busaddress_bus, busaddress_device, busaddress_function);
		if (secondary_bus)
		{
			if (subordinate_bus != secondary_bus)
			{
				printf ("[%02x-%02x] ", secondary_bus, subordinate_bus);
			} else {
				printf ("[%02x] ", secondary_bus);
			}
		}
		printf ("%s\n", name);
	}

	if (!entry)
	{
		perror ("malloc()");
		return;
	}
	entry->name = strdup (name);
	if (!entry->name)
	{
		perror ("strdup()");
		free (entry);
		return;
	}

	entry->allnodes_next       = 0;
	entry->bus_next            = 0;
	entry->bus_child           = 0;
	entry->busaddress_domain   = busaddress_domain;
	entry->busaddress_bus      = busaddress_bus;
	entry->busaddress_device   = busaddress_device;
	entry->busaddress_function = busaddress_function;
	entry->vendor              = vendor;
	entry->device              = device;
	entry->subsystem_vendor    = subsystem_vendor;
	entry->subsystem_device    = subsystem_device;
	entry->mainboard           = 0;
	entry->port                = 0;
	entry->dmi                 = 0;
	entry->nvidia_smi          = 0;
	entry->secondary_bus       = secondary_bus;
	entry->subordinate_bus     = subordinate_bus;
	entry->port_type           = port_type?strdup(port_type):0;

	while (*prev)
	{
		prev = &((*prev)->allnodes_next);
	}
	*prev = entry;
}

static void push_lspci (const char *busaddress, const char *name, const uint16_t vendor, const uint16_t device, const uint16_t subsystem_vendor, const uint16_t subsystem_device, const char *bridge,
	int port_is_express, int max_width, int active_width, int max_gen, int active_gen,
	int port_is_pcix,
	int port_is_agp)
{
	uint16_t busaddress_domain;
	uint8_t  busaddress_bus;
	uint8_t  busaddress_device;
	uint8_t  busaddress_function;

	uint8_t  secondary_bus = 0;
	uint8_t  subordinate_bus = 0;

	char port_type[64];

	if (port_is_express)
	{
		snprintf (port_type, sizeof(port_type), "PCI-e %dx gen %d (%dx gen %d)", active_width, active_gen, max_width, max_gen);
	} else if (port_is_pcix)
	{
		snprintf (port_type, sizeof(port_type), "PCI-X");
	} else if (port_is_agp)
	{
		snprintf (port_type, sizeof(port_type), "AGP");
	} else {
		port_type[0] = 0;
	}

	if (!busaddress[0])
	{
		return;
	}

	//fprintf (stderr, "LSPCI busaddress:->%s<- name:->%s [%04x:%04x %04x:%04x]<- bridge:->%s<-\n", busaddress, name, vendor, device, subsystem_vendor, subsystem_device, bridge);
	busaddress_domain = strtol (busaddress, (char **)&busaddress, 16);
	if (busaddress[0] != ':')
	{
		return;
	}
	busaddress_bus = strtol (busaddress + 1, (char **)&busaddress, 16);
	if (busaddress[0] != ':')
	{
		return;
	}
	busaddress_device = strtol (busaddress + 1, (char **)&busaddress, 16);
	if (busaddress[0] != '.')
	{
		return;
	}
	busaddress_function = strtol (busaddress + 1, (char **)&busaddress, 16);
	if (busaddress[0])
	{
		return;
	}

	while (bridge[0])
	{
		if (!strncmp (bridge, "secondary=", 10))
		{
			secondary_bus = strtol (bridge + 10, (char **)&bridge, 16);
		} else if (!strncmp (bridge, "subordinate=", 12))
		{
			subordinate_bus = strtol (bridge + 12, (char **)&bridge, 16);
		} else {
			while (bridge[0] != ' ')
			{
				if (!bridge[0])
				{
					break;
				}
				bridge++;
			}
		}
		if (bridge[0] == ' ')
		{
			bridge++;
		}
	}

	append_lspci(name, busaddress_domain, busaddress_bus, busaddress_device, busaddress_function, vendor, device, subsystem_vendor, subsystem_device, secondary_bus, subordinate_bus, port_type[0]?port_type:0);
}

static void decode_lspci(void)
{
	char *temp = lspci_data;
	char name[200];
	char busaddress[32];
	char bridge[128];

	uint16_t vendor;
	uint16_t device;

	uint16_t subsystem_vendor = 0;
	uint16_t subsystem_device = 0;

	int need_root = 0;

	int cap_is_express = 0;
	int port_is_express = 0;
	int max_width = 0;
	int active_width = 0;
	int max_gen = 0;
	int active_gen = 0;

	int port_is_pcix = 0;

	int port_is_agp = 0;

	busaddress[0] = 0;
	name[0] = 0;
	bridge[0] = 0;

	while (*temp)
	{
		char *eol = index(temp, '\n');
		char *next;
		if (!eol)
		{
			eol = temp + strlen (temp);
			next = eol;
		} else {
			next = eol + 1;
		}

		if (temp[0]=='\n')
		{
			push_lspci (busaddress, name, vendor, device, subsystem_vendor, subsystem_device, bridge,
			            port_is_express, active_width, max_width, active_gen, max_gen,
			            port_is_pcix,
			            port_is_agp);
			busaddress[0] = 0;
			name[0]       = 0;
			bridge[0]     = 0;
			subsystem_vendor = 0;
			subsystem_device = 0;

			cap_is_express = 0;
			port_is_express = 0;
			max_width = 0;
			active_width = 0;
			max_gen = 0;
			active_gen = 0;
			port_is_pcix = 0;
			port_is_agp = 0;
		} else if (temp[0] != '\t')
		{
			char *needle;
			int len;
			strncpy (busaddress, temp, 12);
			busaddress[12] = 0;
			temp += 13;
			while (temp[0] != ':')
			{
				if (temp[0] == 0)
				{
					break;
				}
				temp++;
			}
			if (temp[0])
			{
				temp++;
			}
			if (temp[0] == ' ')
			{
				temp++;
			}

			len = eol - temp;
			if (len >= sizeof (name))
			{
				len = sizeof(name) - 1;
			}
			strncpy (name, temp, len);
			name[len] = 0;

			/* remove (prog-if XX foo) */
			needle = strstr(name, " (prog-if ");
			if (needle)
			{
				needle[0] = 0;
			}

			/* remove (rev XX) */
			len = strlen (name);
			if (strlen (name) >= 9)
			{
				if ((name[len-9] == ' ') &&
				    (name[len-8] == '(') &&
				    (name[len-7] == 'r') &&
				    (name[len-6] == 'e') &&
				    (name[len-5] == 'v') &&
				    (name[len-4] == ' ') &&
				    (name[len-1] == ')'))
				{
					name[len-9] = 0;
				}
			}

			/* extract the [vendor:device] information */
			vendor = 0;
			device = 0;
			len  = strlen (name);
			if (len > 11)
			{
				if ((name[len-11] == '[') && (name[len-1] == ']'))
				{
					vendor = strtol (name+len-10, 0, 16);
					device = strtol (name+len-5, 0, 16);
					name[len-12] = 0;
				}
			}

		} else if (!strncmp (temp, "\tBus: ", 6))
		{
			temp += 6;
			int len = eol - temp;
			if (len >= sizeof (bridge))
			{
				len = sizeof(bridge) - 1;
			}
			strncpy (bridge, temp, len);
			bridge[len] = 0;
		} else if (!strncmp (temp, "\tSubsystem: ", 12))
		{
			temp += 12;
			int len = eol - temp;
			if (len >= 11)
			{
				if ((temp[len-11] == '[') && (temp[len-1] == ']'))
				{
					subsystem_vendor = strtol (temp+len-10, 0, 16);
					subsystem_device = strtol (temp+len-5, 0, 16);
				}
			}
		} else if (!strncmp (temp, "\tCapabilities: [", 16))
		{
			char *next = strchr (temp, ']');
			cap_is_express = 0;
			if (next)
			{
				if (!strncmp (next, "] Express", 9))
				{
					port_is_express = cap_is_express = 1;
#if 0
					long current_cap = strtol (temp + 16, 0, 0);
					switch (current_cap)
					{
						case 0x40: cap_40 = 1; port_type = "PCIe port"; break;
						case 0x78: cap_78 = 1; port_type = "PCIe card"; break;
						case 0x80: cap_80 = 1; port_type = "PCIe card user"; break;
						case 0xa0: cap_a0 = 0; if (!cap_40) { port_type = "PCIe port 16x?"}; break;

					}
#endif
				} else if (!strncmp (next, "] PCI-X ", 8))
				{
					port_is_pcix = 1;
				} else if (!strncmp (next, "] AGP ", 6))
				{
					port_is_agp = 1;
				}

			}
		} else if (!strncmp (temp, "\tCapabilities: <access", 22))
		{
			need_root = 1;
		} else if (cap_is_express)
		{
			if (!strncmp (temp, "\t\tLnkCap:", 9))
			{
				temp += 9;
				while (temp != next)
				{

					if ((!strncmp(temp, "Width x1,", 9)) || (!strncmp(temp, "Width x1\n", 9)))
					{
						max_width = 1;
					} else if ((!strncmp(temp, "Width x2,", 9)) || (!strncmp(temp, "Width x2\n", 9)))
					{
						max_width = 2;
					} else if ((!strncmp(temp, "Width x4,", 9)) || (!strncmp(temp, "Width x4\n", 9)))
					{
						max_width = 4;
					} else if ((!strncmp(temp, "Width x8,", 9)) || (!strncmp(temp, "Width x8\n", 9)))
					{
						max_width = 8;
					} else if ((!strncmp(temp, "Width x16,", 10)) || (!strncmp(temp, "Width x16\n", 10)))
					{
						max_width = 16;
					} else if (!strncmp(temp, "Speed 2.5GT/s", 13))
					{
						max_gen = 1;
					} else if (!strncmp(temp, "Speed 5GT/s", 11))
					{
						max_gen = 2;
					} else if (!strncmp(temp, "Speed 8GT/s", 11))
					{
						max_gen = 3;
					} else if (!strncmp(temp, "Speed 16GT/s", 12))
					{
						max_gen = 4;
					}
					temp++;
				}
			} else if (!strncmp (temp, "\t\tLnkSta:", 9))
			{
				temp += 9;
				while (temp != next)
				{
					if ((!strncmp(temp, "Width x1,", 9)) || (!strncmp(temp, "Width x1\n", 9)))
					{
						active_width = 1;
					} else if ((!strncmp(temp, "Width x2,", 9)) || (!strncmp(temp, "Width x2\n", 9)))
					{
						active_width = 2;
					} else if ((!strncmp(temp, "Width x4,", 9)) || (!strncmp(temp, "Width x4\n", 9)))
					{
						active_width = 4;
					} else if ((!strncmp(temp, "Width x8,", 9)) || (!strncmp(temp, "Width x8\n", 9)))
					{
						active_width = 8;
					} else if ((!strncmp(temp, "Width x16,", 10)) || (!strncmp(temp, "Width x16\n", 10)))
					{
						active_width = 16;
					} else if (!strncmp(temp, "Speed 2.5GT/s", 13))
					{
						active_gen = 1;
					} else if (!strncmp(temp, "Speed 5GT/s", 11))
					{
						active_gen = 2;
					} else if (!strncmp(temp, "Speed 8GT/s", 11))
					{
						active_gen = 3;
					} else if (!strncmp(temp, "Speed 16GT/s", 12))
					{
						active_gen = 4;
					}
					temp++;
				}
			}
		}

		temp = next;
	}
	push_lspci (busaddress, name, vendor, device, subsystem_vendor, subsystem_device, bridge,
	            port_is_express, active_width, max_width, active_gen, max_gen,
	            port_is_pcix,
	            port_is_agp);

	if (need_root && verbose)
	{
		printf ("\n\033[33;41mNeed root to read PCI speed/statuses/etc\033[0m\n\n");
	}

}

int lspci_main(void)
{
	if (prun(exec_lspci, &lspci_data))
	{
		fprintf (stderr, "lspci failed\n");
		return 1;
	}

	decode_lspci();

//	printf ("%s\n", lspci_data);

	free (lspci_data);

	build_pcibus ();

//	print_pcibus ("", "", lspci_rootbus);

	return 0;
}
