#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "prun.h"
#include "main.h"

int prun(void(*e)(void), char **data)
{
	int _stdout[2];
	int pid;
	int length;
	int size;
	int retval;
	*data = 0;
	if ( pipe(_stdout) )
	{
		perror("pipe()");
		return 1;
	}
	pid = fork();
	if ( pid < 0 )
	{
		perror("fork()");
		close (_stdout[0]);
		close (_stdout[1]);
		return 1;
	}
	if (pid == 0)
	{
		close (1);
		if ( dup (_stdout[1]) < 0)
		{
			perror("dup()");
			return 1;
		}
		close (_stdout[0]);
		close (_stdout[1]);
		e();
		_exit (1);
	}
	close (_stdout[1]);
	length = 0;
	size = 0;
	while (1)
	{
		int retval;
		if ((length+2) >= size)
		{
			char *temp;
			size += 1024;
			temp = realloc (*data, size);
			if (!temp)
			{
				perror ("realloc()");
				free (*data);
				*data = 0;
				close (_stdout[0]);
				return 1;
			}
			*data = temp;
		}
		retval = read (_stdout[0], (*data) + length, size-length-1);
		if (retval < 0)
		{
			perror ("read()");
			free (*data);
			*data = 0;
			close (_stdout[0]);
			return 1;
		}
		if (retval == 0)
		{
			break;
		}
		length += retval;

	}
	close (_stdout[0]);
	(*data)[length] = 0;
	retval = 1;
	wait (&retval);
	return retval;
}
