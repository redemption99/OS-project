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


int
main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Nema dovoljno argumenata.\n");
		exit();
	}

	if (strcmp(argv[argc-1], "root") == 0)
	{
		printf("Nije moguce modifikovati root korisnika.\n");
		exit();
	}
    
	struct userinfo* tuinfo = getuinfoname(argv[argc-1]);

	if (tuinfo == 0)
	{
		printf("Navedeni korisnik ne postoji.\n");
		exit();
	}

	int findm = 0, finda = 0;

	for (int i = 1; i + 1 < argc; i++)
	{
		if (strcmp(argv[i],"-m") == 0)
			findm = 1;
		if (strcmp(argv[i],"-a") == 0)
			finda = 1;
	}

	for (int i = 1; i + 1 < argc; i++)
	{
		if (strcmp(argv[i],"-l") == 0)
		{
			struct userinfo* pomuinfo = getuinfoname(argv[i+1]);
			if (pomuinfo != 0)
			{
				printf("Username je zauzet.");
				continue;
			}

			char* oldname = malloc(strlen(tuinfo->uname) + 1);
			strcpy(oldname, tuinfo->uname);

			tuinfo->uname = malloc(strlen(argv[i+1])+1);
			strcpy(tuinfo->uname, argv[i+1]);
			

			if (rewriteuseruid(tuinfo) < 0)
			{
				printf("Greska prilikom pristupa /etc/passwd.\n");
				continue;
			}

			if (rewritegroupsuname(oldname, tuinfo->uname) < 0)
			{
				printf("Greska prilikom pristupa /etc/group.\n");
				continue;
			}

			printf("Promenjen je username korisnika.\n");

			continue;
		}
		if (strcmp(argv[i],"-u") == 0)
		{
			struct userinfo* pomuinfo = getuinfouid(atoi(argv[i+1]));
			if (pomuinfo != 0)
			{
				printf("UID je zauzet.\n");
				continue;
			}

			int olduid = tuinfo->uid;

			tuinfo->uid = atoi(argv[i+1]);

			if (rewriteuseruname(tuinfo) < 0)
			{
				printf("Greska prilikom pristupa /etc/passwd.\n");
				exit();
			}
			else
				printf("Promenjen je UID korisnika.\n");

			struct stat st;
			int fd;

			if((fd = open(tuinfo->dir, 0)) < 0){
				printf("Greska prikikom otvaranja %s\n", tuinfo->dir);
				continue;
			}

			if(fstat(fd, &st) < 0){
				printf("Greska prilikom pristupa %s\n", tuinfo->dir);
				close(fd);
				continue;
			}

			struct dirent de;
			char buf[512];
			char* p;
			strcpy(buf, tuinfo->dir);
			p = buf+strlen(buf);
			*p++ = '/';
			while(read(fd, &de, sizeof(de)) == sizeof(de)){
				if(de.inum == 0)
					continue;
				memmove(p, de.name, DIRSIZ);
				p[DIRSIZ] = 0;
				if(stat(buf, &st) < 0){
					printf("Greska prilikom pristupa %s\n", buf);
					continue;
				}

				if (st.uid == olduid)
					chown(buf, tuinfo->uid, st.gid);
			}

			continue;
		}
		if (strcmp(argv[i],"-c") == 0)
		{
			tuinfo->name = malloc(strlen(argv[i+1])+1);
			strcpy(tuinfo->name, argv[i+1]);

			if (rewriteuseruname(tuinfo) < 0)
				printf("Greska prilikom pristupa /etc/passwd.\n");
			else
				printf("Promenjeno je ime korisnika.\n");

			continue;
		}
		if (strcmp(argv[i],"-d") == 0)
		{
			char* olddir;

			strcpy(olddir, tuinfo->dir);

			struct stat st;
			if (stat(argv[i+1], &st) == 0) {
				if (st.type != T_DIR)
				{
					printf("Navedena putanja postoji, ali nije direktorijum, direktorijum korisnika nije promenjen.\n");
					continue;
				}
				else

				{
					printf("Navedeni direktorijum vec postoji.\n");
				}
			}
			else
				if (mkdir(argv[i+1]) < 0)
				{
					printf("Greska prilikom kreiranja direktorijuma, direktorijum korisnika nije promenjen.\n");
					continue;
				}
				else
				{
					if (chmod(argv[i+1], DEFMOD) < 0 || chown(argv[i+1], tuinfo->uid, tuinfo->gid) < 0)
					{
						printf("Greska prilikom postavljanja informacija za novi direktorijum, direktorijum korisnika nije promenjen.\n");
						continue;
					}

					printf("Kreiran je novi direktorijum.\n");
				}

			strcpy(tuinfo->dir, argv[i+1]);

			if (rewriteuseruname(tuinfo) < 0)
				printf("Greska prilikom pristupa /etc/passwd.\n");
			else
				printf("Promenjen je direktorijum korisnika.\n");

			if (!findm)
				continue;

			char oldpathbuf[512];
			char newpathbuf[512];

			strcpy(oldpathbuf, olddir);
			strcpy(newpathbuf, tuinfo->dir);

			char* p1 = oldpathbuf + strlen(oldpathbuf);
			char* p2 = newpathbuf + strlen(newpathbuf);
			*p1++ = '/';
			*p2++ = '/';

			struct dirent de;
			int fd = open(olddir, 0);

			if (fd < 0)
			{
				printf("Greska prilikom otvaranja %s, fajlovi nisu premesteni.\n", olddir);
				continue;
			}

			while(read(fd, &de, sizeof(de)) == sizeof(de)){
				if(de.inum == 0)
					continue;
				memmove(p1, de.name, DIRSIZ);
				memmove(p2, de.name, DIRSIZ);
				p1[DIRSIZ] = 0;
				p2[DIRSIZ] = 0;

				if(stat(oldpathbuf, &st) < 0){
					printf("Greska prilikom pristupa %s\n", oldpathbuf);
					continue;
				}

				if (st.type != T_FILE || st.uid != tuinfo->uid)
					continue;

				if(stat(newpathbuf, &st) >= 0){
					printf("Fajl sa imenom %s vec postoji u novom direktorijumu, nije premesten. \n", de.name);
					continue;
				}


				int oldfd = open(oldpathbuf, O_RDONLY);
				int newfd = open(newpathbuf, O_WRONLY | O_CREATE);

				char buf[512];

				int r, w;

				while ((r = read(oldfd, buf, sizeof(buf))) > 0) {
					w = write(newfd, buf, r);
					if (w != r || w < 0)
						break;
				}

				if (r< 0 || w < 0)
					printf("Doslo je do greske prilikom prebacivanja fajla %s\n", de.name);

				close(oldfd);
				close(newfd);

				unlink(oldpathbuf);				
			}


			printf("Zavrseno je premestanje svih fajlova.\n");

			continue;
		}
		if (strcmp(argv[i],"-G") == 0)
		{
			char* gname;

			int pos = 0;

			int n = strlen(strlen(argv[i+1]));

			for (int j = 0; j < n; j++)
				if (argv[i+1][j] == ',')
				{
					argv[i+1][j] = 0;
					gname = malloc(strlen(argv[i+1] + pos) + 1);
					strcpy(gname, argv[i+1] + pos);

					pos = j + 1;

					struct groupinfo* gi = 0;

					if (gname[0] >= '0' && gname[0] <= '9')
						gi = getginfogid(atoi(gname));
					else
						gi = getginfoname(gname);

					if (gi == 0)
					{
						printf("Grupa %s ne postoji.\n", gname);
						continue;
					}

					if (gi->gid == 0)
					{
						printf("Nije moguce dodati korisnika u root grupu.\n", gname);
						continue;	
					}

					if (addutogname(tuinfo->uname, gi->gname, finda) < 0) 
						printf("Greska prilikom pristupa /etc/grooup za %s.\n", gname);
					else
						printf("Uspesno izvrsen upit za grupu %s\n", gname);
				}

			gname = malloc(strlen(argv[i+1] + pos) + 1);
			strcpy(gname, argv[i+1] + pos);

			struct groupinfo* gi = 0;

			if (gname[0] >= '0' && gname[0] <= '9')
				gi = getginfogid(atoi(gname));
			else
				gi = getginfoname(gname);

			if (gi == 0)
			{
				printf("Grupa %s ne postoji.\n", gname);
				continue;
			}

			if (gi->gid == 0)
			{
				printf("Nije moguce dodati korisnika u root grupu.\n", gname);
				continue;	
			}

			if (addutogname(tuinfo->uname, gi->gname, finda) < 0) 
				printf("Greska prilikom pristupa /etc/grooup za %s.\n", gname);
			else
				printf("Uspesno izvrsen upit za grupu %s\n", gname);


			if (tuinfo->uid == getuid())
			{
				int n2;
			    int gids[MAXGROUPSNUM];

			    getgroupsuname(tuinfo->uname, &n2, gids);

			    if (setgroups(n2, gids) < 0)
			    	printf("Nije moguce azurirati podatke o grupama u procesu.\n");
			    else
			    	printf("Podaci o grupama u procesu su azurirani.\n");
			}

			continue;
		}
	}


    exit();
}
