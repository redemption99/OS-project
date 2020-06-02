#include "kernel/types.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/file.h"
#include "kernel/param.h"
#include "kernel/mmu.h"
#include "kernel/proc.h"
#include "kernel/stat.h"

void
esc(char* s)
{
	printf("%s\n", s);
	exit();
}

int
main(int argc, char* argv[])
{

	if (getginfoname(argv[argc-1]) != 0)
		esc("Naziv grupe je zauzet.");

    int gid;

    if (argc == 4 && strcmp(argv[1], "-g") == 0)
    {
		gid = atoi(argv[2]);
        if (getginfogid(gid) != 0)
            esc("GID grupe je zauzet.");
    }
	else
	{
		gid = 1000;
	    while (getginfogid(++gid) != 0);
	}


    struct groupinfo* gi = malloc(sizeof(struct groupinfo));

    gi->gname = malloc(strlen(argv[argc-1])+1);
    strcpy(gi->gname, argv[argc-1]);

    gi->gid = gid;

    gi->glava = 0;

    if (writenewgroup(gi) < 0)
    	esc("Greska prilikom pristupa /etc/group.");

    printf("Grupa je uspesno kreirana.\n");

    exit();
}

