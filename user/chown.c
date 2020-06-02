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

	int pos = -1;

	for (int i = 0; i < strlen(argv[1]); i++)
		if (argv[1][i] == ':')
		{
			if (pos != -1)
			{
				printf("Format nije validan.\n");
				exit();
			}
			pos = i;
		}

	struct userinfo* ui = 0;
	struct groupinfo* gi = 0;
	
    if (strcmp(argv[1], ":") == 0)
    {
    	printf("Nista se ne menja.\n");
    	exit();
    }

	if (pos == -1)
	{
		if (argv[1][0] >= '0' && argv[1][0] <= '9')
			ui = getuinfouid(atoi(argv[1]));
		else
			ui = getuinfoname(argv[1]);

		if (ui == 0)
		{
			printf("Navedeni korisnik ne postoji.\n");
			exit();
		}
	}
	else
	if (pos == 0)
	{
		argv[1]++;

		if (argv[1][0] >= '0' && argv[1][0] <= '9')
			gi = getginfogid(atoi(argv[1]));
		else
			gi = getginfoname(argv[1]);

		if (gi == 0)
		{
			printf("Navedena grupa ne postoji.\n");
			exit();
		}
	}
	else
	if (pos == strlen(argv[1]) - 1)
	{
		argv[1][strlen(argv[1]) - 1] = 0;

		if (argv[1][0] >= '0' && argv[1][0] <= '9')
			ui = getuinfouid(atoi(argv[1]));
		else
			ui = getuinfoname(argv[1]);

		if (ui == 0)
		{
			printf("Navedeni korisnik ne postoji.\n");
			exit();
		}

		gi = getginfogid(ui -> gid);
	}
	else
	if (pos > 0 && pos < strlen(argv[1]) - 1)
	{
		argv[1][pos] = 0;

		if (argv[1][0] >= '0' && argv[1][0] <= '9')
			ui = getuinfouid(atoi(argv[1]));
		else
			ui = getuinfoname(argv[1]);

		if (ui == 0)
		{
			printf("Navedeni korisnik ne postoji.\n");
			exit();
		}

		if (argv[1][pos + 1] >= '0' && argv[1][pos + 1] <= '9')
			gi = getginfogid(atoi(argv[1] + pos + 1));
		else
			gi = getginfoname(argv[1] + pos + 1);

		if (gi == 0)
		{
			printf("Navedena grupa ne postoji.\n");
			exit();
		}
	}


	for (int i = 2; i < argc; i++)
    {
    	struct stat st;
    	if (stat(argv[i], &st))
    	{
    		printf("%s ne postoji\n", argv[i]);
    		continue;
    	}

    	int uid;
    	int gid;

    	if (ui == 0)
    		uid = st.uid;
    	else
    		uid = ui->uid;

    	if (gi == 0)
    		gid = st.gid;
    	else
    		gid = gi->gid;


        if (chown(argv[i], uid, gid) < 0) 
        {
            printf("Greska, podaci za %s nisu promenjeni.\n", argv[i]);
        }
        else
            printf("Podaci za %s su promenjeni.\n", argv[i]);
    }


    exit();
}
