#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dmi.h"
#include "lspci.h"
#include "nvidia-smi.h"
#include "portdb.h"

int debug = 0;
int verbose = 0;

static void link_dminode_lspci(struct dmi_entry *e)
{
	struct lspci_entry *iter;

	if (debug)
	{
		printf ("Going to search for PCI entry for DMI entry %02x:%02x.%x : ", e->busaddress_bus, e->busaddress_device, e->busaddress_function);
	}
	for (iter=lspci_allnodes; iter; iter = iter->allnodes_next)
	{
		if ((iter->busaddress_bus == e->busaddress_bus) &&
		    (iter->busaddress_device == e->busaddress_device) &&
		    (iter->busaddress_function == e->busaddress_function))
		{
			iter->dmi = e;
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

static void link_dmi_lspci(void)
{
	struct dmi_entry *iter;
	for (iter=dmi_allnodes; iter; iter = iter->allnodes_next)
	{
		link_dminode_lspci (iter);
	}
}

static void link_nvidia_smi_node_lspci(struct nvidia_smi_entry *e)
{
	struct lspci_entry *iter;

	if (debug)
	{
		printf ("Going to search for PCI entry for nvidia-smi entry %02x:%02x.%x : ", e->busaddress_bus, e->busaddress_device, e->busaddress_function);
	}
	for (iter=lspci_allnodes; iter; iter = iter->allnodes_next)
	{
		if ((iter->busaddress_bus == e->busaddress_bus) &&
		    (iter->busaddress_device == e->busaddress_device) &&
		    (iter->busaddress_function == e->busaddress_function))
		{
			iter->nvidia_smi = e;
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


static void link_nvidia_smi_lspci(void)
{
	struct nvidia_smi_entry *iter;
	for (iter=nvidia_smi_allnodes; iter; iter = iter->allnodes_next)
	{
		link_nvidia_smi_node_lspci (iter);
	}
}

int main(int argc, char *argv[])
{
	int opt;
	int rundmidecode = 0;
	while  ((opt=getopt (argc, argv, "dhvD")) != -1)
	{
		switch (opt)
		{
			case 'd': debug=1; break;
			case 'v': verbose=1; break;
			case 'D': rundmidecode = 1; break;
			default:
			case 'h': fprintf (stderr, "Usage: %s [-dvh]\n -d   debug\n -v   verbose\n -h   help\n -D   run dmidecode", argv[0]); return 1;
		}
	}

	if (optind != argc)
	{
		fprintf (stderr, "Unknown trailing argument\n"); return 1;
	}

	if (lspci_main())
	{
		return 1;
	}

	if (rundmidecode)
	{
		if (dmidecode_main())
		{
			return 1;
		}
		link_dmi_lspci();
		if (verbose)
		{
			char board_vendor[128];
			char board_name[128];
			int i = 0, fd;
			struct dmi_entry *iter;

			board_vendor[0] = 0;
			board_name[0] = 0;

			fd = open ("/sys/devices/virtual/dmi/id/board_vendor", O_RDONLY);
			if (fd < 0)
			{
				perror ("open (\"/sys/devices/virtual/dmi/id/board_vendor\", O_RDONLY)");
			} else {
				int len;
				char *tmp;
				len = read (fd, board_vendor, sizeof (board_vendor) - 1);
				if (len >= 0)
				{
					board_vendor[len] = 0;
				}
				tmp = index (board_vendor, '\n');
				if (tmp)
				{
					*tmp = 0;
				}
				close (fd);
			}

			fd = open ("/sys/devices/virtual/dmi/id/board_name", O_RDONLY);
			if (fd < 0)
			{
				perror ("open (\"/sys/devices/virtual/dmi/id/board_name\", O_RDONLY)");
			} else {
				int len;
				char *tmp;
				len = read (fd, board_name, sizeof (board_name) - 1);
				if (len >= 0)
				{
					board_name[len] = 0;
				}
				tmp = index (board_name, '\n');
				if (tmp)
				{
					*tmp = 0;
				}
				close (fd);
			}

			printf ("\033[33;41m   SNIP SNIP SNIP   \033[0m\n\n");
			printf ("/* Add to portdb-static.c */\n");
			printf ("{0x%04x, 0x%04x, 0x%04x, 0x%04x, \"%s\", \"%s\",\n",
				lspci_rootbus?lspci_rootbus->vendor:0,
				lspci_rootbus?lspci_rootbus->device:0,
				lspci_rootbus?lspci_rootbus->subsystem_vendor:0,
				lspci_rootbus?lspci_rootbus->subsystem_vendor:0,
				board_vendor,
				board_name);
			printf ("\t{\n");
			for (iter=dmi_allnodes; iter; iter = iter->allnodes_next)
			{
				if (i)
				{
					printf(",\n");
				}
				i++;
				printf ("\t\t{0x%02x, 0x%02x, 0x%02x, \"%s\"}",
					iter->busaddress_bus,
					iter->busaddress_device,
					iter->busaddress_function,
					iter->designation);
			}
			if (i)
			{
				printf("\n");
			}
			printf ("\t},%d\n}\n", i);
			printf ("\n\033[33;41m   SNIP SNIP SNIP   \033[0m\n\n");
		}
	}

	if (nvidia_smi_main ())
	{
		return 1;
	}

	link_nvidia_smi_lspci();

	apply_portdb ();

	print_pcibuses ();

	return 0;
}
