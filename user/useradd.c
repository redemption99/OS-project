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

	if (getuinfoname(argv[argc-1]) != 0)
		esc("Username je zauzet.");

    char* dir = malloc(strlen("/home/") + strlen(argv[argc-1]) + 1);
    strcpy(dir, "/home/");
    strcpy(dir + strlen("/home/"), argv[argc-1]);

    int uid = 1000;
    while (getuinfouid(++uid) != 0);

    // pravo ime ce po defaultu biti isto kao username
    char* name = malloc(strlen(argv[argc-1]) + 1);
    strcpy(name, argv[argc-1]);

    for (int i = 1; i + 1 < argc; i += 2)
    {
    	if (strcmp(argv[i],"-d") == 0)
		{
			dir = malloc(strlen(argv[i+1]));
			strcpy(dir, argv[i+1]);
			continue;
		}
    	if (strcmp(argv[i],"-u") == 0)
		{
			uid = atoi(argv[i+1]);
			continue;
		}

    	if (strcmp(argv[i],"-c") == 0)
		{
			name = malloc(strlen(argv[i+1]));
			strcpy(name, argv[i+1]);
			continue;
		}	
    }

    int gid = uid;

    if (getginfogid(gid) != 0)
    {
    	gid = 1000;
    	while (getginfogid(++gid) != 0);
    }

    if (getuinfouid(uid) != 0)
    	esc("UID je zauzet, korisnik nije kreiran.");

    struct stat st;
    if (stat(dir, &st) == 0) {
    	if (st.type != T_DIR)
    		esc("Navedena putanja postoji, ali nije direktorijum, korisnik nije kreiran.");
    }
    else
    	if (mkdir(dir) < 0)
    		esc("Greska prilikom kreiranja direktorijuma, korisnik nije kreiran.");
    	else
    	{
    		chown(dir, uid, gid);
            printf("Direktorijum je uspesno kreiran.\n");
    	}

    struct userinfo* nu = malloc(sizeof(struct userinfo));

	nu->uname = malloc(strlen(argv[argc-1])+1);
    strcpy(nu->uname, argv[argc-1]);

    nu->passw = malloc(strlen("")+1);
    strcpy(nu->passw, "");

    nu->uid = uid;
    nu->gid = gid;

    nu->name = malloc(strlen(name)+1);
    strcpy(nu->name, name);

    nu->dir = malloc(strlen(dir)+1);
    strcpy(nu->dir, dir);

    struct groupinfo* gi = malloc(sizeof(struct groupinfo));

    gi->gname = malloc(strlen(argv[argc-1])+1);
    strcpy(gi->gname, argv[argc-1]);

    gi->gid = gid;

    gi->glava = malloc(sizeof(struct grlista));

    gi->glava->sl = 0;

    gi->glava->uname = malloc(strlen(argv[argc-1])+1);
    strcpy(gi->glava->uname, argv[argc-1]);

    if (writenewgroup(gi) < 0) 
    	esc("Greska prilikom pristupa /etc/group");

    if (writenewuser(nu) < 0)
    	esc("Greska prilikom pristupa /etc/passwd.");

    printf("Korisnik je uspesno kreiran.\n");

    exit();
}

