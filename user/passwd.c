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

int
main(int argc, char *argv[])
{
	int uid = getuid();

	struct userinfo* tuinfo = getuinfouid(uid);

	if (argc == 2)
	{
		char* name = argv[1];

		if ((tuinfo = getuinfoname(name)) == 0)
		{
			printf("Navedeni korisnik ne postoji.\n");
			exit();
		}
	}

	if (uid != tuinfo->uid && uid != 0)
	{
		printf("Ulogovani korisnik ne moze menjati sifru navedenom korisniku, sifra se ne menja.\n");
		exit();
	}

	if (uid != 0)
	{
		char password[MAX_LENGTH + 1] = {0};
		printf("Unesi svoju sifru: ");
		hidepassw();
	    read(1, password, MAX_LENGTH+1);
	    password[strlen(password) - 1] = 0;  // -1 zato sto uracuna i enter u password

	    if (strcmp(password, tuinfo->passw) != 0)
	    {
	    	printf("Sifra nije dobra, sifra se ne menja.\n");
	    	exit();
	    }
	}

	char password2[MAX_LENGTH + 1] = {0};
	printf("Unesi novu sifru: ");
	hidepassw();
    read(1, password2, MAX_LENGTH+1);
    password2[strlen(password2) - 1] = 0;  // -1 zato sto uracuna i enter u password

    char password3[MAX_LENGTH + 1] = {0};
	printf("Potvrdi novu sifru: ");
	hidepassw();
    read(1, password3, MAX_LENGTH+1);
    password3[strlen(password3) - 1] = 0;  // -1 zato sto uracuna i enter u password

    if (strcmp(password2, password3) != 0)
    {
    	printf("Sifre se ne poklapaju, sifra se ne menja.\n");
    	exit();
    }

    if (strlen(password2) <= 6)
    {
    	printf("Sifra mora biti duza od 6 karaktera, sifra se ne menja.\n");
    	exit();
    }

    tuinfo->passw = password2;

    rewriteuseruid(tuinfo);

	printf("Sifra je promenjena.\n");

    exit();
}
