#include "kernel/types.h"
#include "user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/file.h"
#include "kernel/param.h"
#include "kernel/mmu.h"
#include "kernel/proc.h"

int
main(int argc, char* argv[])
{
    struct groupinfo* gi = 0;

	if (argv[1][0] >= '0' && argv[1][0] <= '9')
		gi = getginfogid(atoi(argv[1]));
	else
		gi = getginfoname(argv[1]);

	if (gi == 0)
	{
		printf("Navedena grupa ne postoji.\n");
		exit();
	}

	int gid = gi->gid;

    for (int i = 2; i < argc; i++)
    {
    	struct stat st;
    	stat(argv[i], &st);
        if (chown(argv[i], st.uid, gid) < 0) 
        {
            printf("Greska, grupa za %s nije promenjena.\n", argv[i]);
        }
        else
            printf("Grupa za %s je promenjena.\n", argv[i]);
    }
    

    exit();
}
