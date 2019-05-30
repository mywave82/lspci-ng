#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "dmi.h"
#include "main.h"
#include "prun.h"

static char *dmidecode_data = 0;

struct dmi_entry *dmi_allnodes = 0;

static void exec_dmidecode(void)
{
	execl ("./dmidecode-3-2/dmidecode", "dmidecode", "-t", "9", NULL);
	execl ("./dmidecode", "dmidecode", "-t", "9", NULL);
	execlp ("dmidecode", "dmidecode", "-t", "9", NULL);
	perror ("execl(\"./dmidecode-3-2/dmidecode\", \"dmidecode\", 0)");
	_exit (1);
}

static void add_dmi(const char *id, const char *designation, uint16_t busaddress_system, uint8_t busaddress_bus, uint8_t busaddress_device, uint8_t busaddress_function)
{
	struct dmi_entry *entry = malloc(sizeof (*entry));
	struct dmi_entry **prev = &dmi_allnodes;
	if (!entry)
	{
		perror ("malloc()");
		return;
	}
	entry->id = atoi(id);
	entry->designation = strdup (designation);
	if (!entry->designation)
	{
		perror ("strdup()");
		free (entry);
		return;
	}
	entry->busaddress_system = busaddress_system;
	entry->busaddress_bus = busaddress_bus;
	entry->busaddress_device = busaddress_device;
	entry->busaddress_function = busaddress_function;
	entry->matched = 0;
	entry->allnodes_next = 0;

	while (*prev)
	{
		prev = &(*prev)->allnodes_next;
	}
	*prev = entry;
}

static void push_dmidecode (const char *id, const char *designation, const char *busaddress)
{
	uint16_t busaddress_system;
	uint8_t busaddress_bus;
	uint8_t busaddress_device;
	uint8_t busaddress_function;

	if ((!id[0]) && (!designation[0]) && (!busaddress[0]))
	{
		return;
	}
	if (debug)
	{
		printf ("DMI id:->%s<- designation:->%s<- busaddress:->%s<-\n", id, designation, busaddress);
	}
	if (!busaddress[0])
	{
		/* we only store entries with PCI-express address */
		return;
	}
	busaddress_system = strtol (busaddress, (char **)&busaddress, 16);
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
	add_dmi (id, designation, busaddress_system, busaddress_bus, busaddress_device, busaddress_function);
}

static void decode_dmidecode(void)
{
	char *temp = dmidecode_data;
	char designation[32];
	char busaddress[32];
	char id[16];

	designation[0] = 0;
	busaddress[0] = 0;
	id[0] = 0;

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

		if (!strncmp (temp, "Handle ", 7))
		{
			push_dmidecode (id, designation, busaddress);
			designation[0] = 0;
			busaddress[0] = 0;
			id[0] = 0;
		} else if (!strncmp (temp, "\tDesignation: ", 14))
		{
			int len = eol - (temp + 14);
			if (len >= 32)
			{
				len = 31;
			}
			strncpy (designation, temp + 14, len);
			designation[len] = 0;
		} else if (!strncmp (temp, "\tBus Address: ", 14))
		{
			int len = eol - (temp + 14);
			if (len >= 32)
			{
				len = 31;
			}
			strncpy (busaddress, temp + 14, len);
			busaddress[len] = 0;
		} else if (!strncmp (temp, "\tID: ", 5))
		{
			int len = eol - (temp + 5);
			if (len >= 16)
			{
				len = 15;
			}
			strncpy (id, temp + 5, len);
			id[len] = 0;
		}
		temp = next;
	}
	push_dmidecode (id, designation, busaddress);
}

int dmidecode_main (void)
{
	if (getuid() != 0)
	{
		fprintf (stderr, "Need ROOT to run dmidecode\n");
		return 1;
	}

	if (prun(exec_dmidecode, &dmidecode_data))
	{
		fprintf (stderr, "DMIDECODE failed\n");
		return 1;
	}

	decode_dmidecode();

//	printf ("%s\n", dmidecode_data);

	free (dmidecode_data);

	return 0;
}
