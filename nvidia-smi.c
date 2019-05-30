#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "main.h"
#include "nvidia-smi.h"
#include "prun.h"

struct nvidia_smi_entry *nvidia_smi_allnodes = 0;

static char *nvidia_smi_data = 0;

static void exec_nvidia_smi(void)
{
	execlp ("nvidia-smi", "nvidia-smi", "--query-gpu=index,name,uuid,pci.bus_id", "--format=csv,noheader", NULL);
	perror ("execl(\"nvidia-smi\", \"--query-gpu=index,name,uuid,pci.bus_id\", \"--format=csv,noheader\")");
	_exit (1);
}

static void append_nvidia_smi(int id, const char *name, const char *uuid, /*uint16_t busaddress_system,*/ uint8_t busaddress_bus, uint8_t busaddress_device, uint8_t busaddress_function)
{
	struct nvidia_smi_entry **prev = &nvidia_smi_allnodes;
	struct nvidia_smi_entry *entry = malloc(sizeof (*entry));

	if (debug)
	{
		printf ("append_nvidia_smi: %d: %02x:%02x.%02x %s %s\n", id, busaddress_bus, busaddress_device, busaddress_function, uuid, name);
	}

	if (!entry)
	{
		perror ("malloc()");
		return;
	}
	entry->id = id;
	entry->name = strdup (name);
	if (!entry->name)
	{
		perror ("strdup()");
		free (entry);
		return;
	}
	entry->uuid = strdup (uuid);
	if (!entry->uuid)
	{
		perror ("strdup()");
		free (entry->name);
		free (entry);
		return;
	}
//	entry->busaddress_system   = busaddress_system;
	entry->busaddress_bus      = busaddress_bus;
	entry->busaddress_device   = busaddress_device;
	entry->busaddress_function = busaddress_function;
	entry->allnodes_next = 0;

	while (*prev)
	{
		prev = &((*prev)->allnodes_next);
	}
	*prev = entry;
}

static void push_nvidia_smi (const char *id, const char *name, const char *uuid, const char *busaddress)
{
	/* uint32_t busaddress_system; */
	uint8_t busaddress_bus;
	uint8_t busaddress_device;
	uint8_t busaddress_function;

	if (!busaddress[0])
	{
		return;
	}

	if (debug)
	{
		printf ("NVIDIA-SMI ID:->%s<- name:->%s<- uuid:->%s<- busaddress:->%s<-\n", id, name, uuid, busaddress);
	}

	/* busaddress_system = */ strtol (busaddress, (char **)&busaddress, 16);
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

	append_nvidia_smi(atoi(id), name, uuid, /*busaddress_system,*/ busaddress_bus, busaddress_device, busaddress_function);
}

static void decode_nvidia_smi(void)
{
	char *temp = nvidia_smi_data;

	while (*temp)
	{
		char *eol = index(temp, '\n');
		char *next;
		char *needle;
		int len;

		char id[8];
		char name[64];
		char uuid[128];
		char busaddress[32];

		/*
		id[0] = 0;
		busaddress[0] = 0;
		name[0] = 0;
		busaddress[0] = 0;
		*/

		if (!eol)
		{
			eol = temp + strlen (temp);
			next = eol;
		} else {
			next = eol + 1;
		}


		/* ID */
		needle = index (temp, ',');
		if (!needle)
		{
			continue;
		}

		len = needle - temp;
		if (len >= sizeof (id))
		{
			len = sizeof(id) - 1;
		}
		strncpy (id, temp, len);
		id[len] = 0;

		needle++;
		if (needle[0] != ' ')
		{
			continue;
		}
		temp = needle + 1;

		/* NAME */
		needle = index (temp, ',');
		if (!needle)
		{
			continue;
		}

		len = needle - temp;
		if (len >= sizeof (name))
		{
			len = sizeof(name) - 1;
		}
		strncpy (name, temp, len);
		name[len] = 0;

		needle++;
		if (needle[0] != ' ')
		{
			continue;
		}
		temp = needle + 1;

		/* UUID */
		needle = index (temp, ',');
		if (!needle)
		{
			continue;
		}

		len = needle - temp;
		if (len < 1)
		{
			continue;
		}
		strncpy (uuid, temp, len);
		uuid[len] = 0;

		needle++;
		if (needle[0] != ' ')
		{
			continue;
		}
		temp = needle + 1;

		/* BUSADDRESS */
		len = eol - temp;
		if (len >= sizeof (busaddress))
		{
			len = sizeof(busaddress) - 1;
		}
		strncpy (busaddress, temp, len);
		busaddress[len] = 0;

		push_nvidia_smi (id, name, uuid, busaddress);

		temp = next;
	}
}

int nvidia_smi_main (void)
{
	if (prun(exec_nvidia_smi, &nvidia_smi_data))
	{
		fprintf (stderr, "nvidia-smi failed\n");
		return 0;
//		return 1;
	}

	decode_nvidia_smi();

	free (nvidia_smi_data);

	return 0;
}
