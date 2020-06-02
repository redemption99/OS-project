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

char buf[512];

void
cat(char* path)
{
    int fd = open(path, O_RDONLY);

    int n;

    while((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(1, buf, n) != n) {
            printf("cat: write error\n");
            close(fd);
            exit();
        }
    }
    if(n < 0){
        printf("cat: read error\n");
        close(fd);
        exit();
    }

    close(fd);
}

int
main()
{
    clrscr();

    cat("/etc/issue");

    char username[MAX_LENGTH + 1] = {0};
    printf("Username: ");
    read(1, username, MAX_LENGTH+1);
    username[strlen(username) - 1] = 0; // -1 zato sto uracuna i enter u username

    char password[MAX_LENGTH + 1] = {0};
    printf("Password: ");
    hidepassw();  
    read(1, password, MAX_LENGTH+1);
    password[strlen(password) - 1] = 0;  // -1 zato sto uracuna i enter u password

    struct userinfo* tuinfo = getuinfoname(username);

    if (tuinfo == -1) {
        printf("Korisnik ne postoji.\n");
        exit();
    }

    if (strcmp(password, tuinfo->passw) != 0) {
        printf("Lozinka nije tacna.\n");
        exit();
    }

    cat("/etc/motd");
    
    char *argv[] = {"/bin/sh", 0};


    chdir(tuinfo->dir);


    setgid(tuinfo->gid); // postavljanje glavnog gid broja koji ce biti gid novokreiranih fajlova

    int n;
    int gids[MAXGROUPSNUM];

    getgroupsuname(tuinfo->uname, &n, gids);

    setgroups(n, gids);
    
    setuid(tuinfo->uid);

    exec("/bin/sh", argv);

    exit();
}
