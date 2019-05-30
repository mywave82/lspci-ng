#ifndef _PRUN_H
#define _PRUN_H

/* pipe, callback for exec, returns copy of stdout in *data */
int prun(void(*e)(void), char **data);

#endif
