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

static struct userinfo* rootinfo;
static struct userinfo uinfo;
static struct groupinfo* grootinfo;
static struct groupinfo ginfo;
static char line[512];


int
getnextline(int fd, char* dest)
{
    char c;

    int n = 0;

    while (read(fd, &c, 1) && c != '\n')
    {
        *dest++ = c;
        n++;
    }

    *dest++ = 0;

    return n;
}

int
linetouser()
{
    int n = strlen(line);

    line[n] = ':';
    line[n+1] = 0;

    int t = 0;

    int pos = 0;

    for (int i = 0; line[i] != 0; i++) 
        if (line[i] == ':') {

            line[i] = 0;

            t++;

            if (t == 1) uinfo.uname = line + pos;
            if (t == 2) uinfo.passw = line + pos;
            if (t == 3) uinfo.uid = atoi(line + pos);
            if (t == 4) uinfo.gid = atoi(line + pos);
            if (t == 5) uinfo.name = line + pos;
            if (t == 6) {
                uinfo.dir = line + pos;
                break;
            }

            pos = i + 1;
        }

    return 0;    
}

int
linetogroup()
{
    int n = strlen(line);

    line[n] = ':';
    line[n+1] = 0;

    int t = 0;

    int pos = 0;

    ginfo.glava = 0;

    for (int i = 0; line[i] != 0; i++) 
        if (line[i] == ':' || line[i] == ',') {

            if (line[i] == ',')
                t = 3;

            if (line[i] == ':')
                t++;

            line[i] = 0;

            if (t == 1) ginfo.gname = line + pos;
            if (t == 2) ginfo.gid = atoi(line + pos);
            if (t >= 3 && strlen(line + pos) > 0) {
                struct grlista* novi = malloc(sizeof(struct grlista));
                novi->sl = ginfo.glava;
                ginfo.glava = novi;
                novi->uname = line + pos;
            }
            if (t > 3)
                break;

            pos = i + 1;
        }

    return 0;    

}

struct userinfo*
userinfocpy(struct userinfo* tuinfo)
{
    struct userinfo* ui = malloc(sizeof(struct userinfo));

    ui->uname = malloc(strlen(tuinfo->uname)+1);
    strcpy(ui->uname, tuinfo->uname);

    ui->passw = malloc(strlen(tuinfo->passw)+1);
    strcpy(ui->passw, tuinfo->passw);

    ui->uid = tuinfo->uid;
    ui->gid = tuinfo->gid;

    ui->name = malloc(strlen(tuinfo->name)+1);
    strcpy(ui->name, tuinfo->name);

    ui->dir = malloc(strlen(tuinfo->dir)+1);
    strcpy(ui->dir, tuinfo->dir);

    return ui;
}

struct groupinfo*
groupinfocpy(struct groupinfo* tginfo)
{
    struct groupinfo* gi = malloc(sizeof(struct groupinfo));

    gi->gname = malloc(strlen(tginfo->gname)+1);
    strcpy(gi->gname, tginfo->gname);

    gi->gid = tginfo->gid;

    gi->glava = 0; 

    struct grlista* i = tginfo->glava;

    while (i != 0)
    {
        struct grlista* pom = malloc(sizeof(struct grlista));

        pom->uname = malloc(strlen(i->uname)+1);
        strcpy(pom->uname, i->uname);

        pom->sl = gi->glava;
        gi->glava = pom;

        i = i->sl;
    }

    return gi;
}


struct userinfo*
getrootinfo()
{
    if (rootinfo == 0)
    {
        rootinfo = malloc(sizeof(struct userinfo));

        rootinfo->uname = malloc(strlen("root")+1);
        strcpy(rootinfo->uname, "root");

        rootinfo->passw = malloc(strlen("")+1);
        strcpy(rootinfo->passw, "");

        rootinfo->uid = 0;
        rootinfo->gid = 0;

        rootinfo->name = malloc(strlen("Superuser")+1);
        strcpy(rootinfo->name, "Superuser");

        rootinfo->dir = malloc(strlen("/home/root")+1);
        strcpy(rootinfo->dir, "/home/root");
    }

    return rootinfo;
}

struct groupinfo*
getgrootinfo()
{
    if (grootinfo == 0)
    {
        grootinfo = malloc(sizeof(struct userinfo));

        grootinfo->gname = malloc(strlen("root")+1);
        strcpy(grootinfo->gname, "root");

        grootinfo->gid = 0;

        grootinfo->glava = malloc(sizeof(struct grlista));

        grootinfo->glava->sl = 0;
        grootinfo->glava->uname = malloc(strlen("root")+1);
        strcpy(grootinfo->glava->uname, "root");
    }

    return grootinfo;
}

struct userinfo*
getuinfoname(const char* username)
{
    if (strcmp(username, "root") == 0)
        return getrootinfo();

    int fd = open("/etc/passwd", O_RDONLY);

    if (fd < 0)
    {
        printf("Greska prilikom otvaranja /etc/passwd.\n");
        return;
    }

    while (getnextline(fd, line) > 0) {
        linetouser();

        if (strcmp(username, uinfo.uname) == 0)
        {
            close(fd);
            return userinfocpy(&uinfo);
        }
    }
    
    close(fd);

    return 0;
}

struct groupinfo*
getginfoname(const char* groupname)
{
    if (strcmp(groupname, "root") == 0)
        return getgrootinfo();

    int fd = open("/etc/group", O_RDONLY);

    if (fd < 0)
    {
        printf("Greska prilikom otvaranja /etc/group.\n");
        return;
    }

    while (getnextline(fd, line) > 0) {
        linetogroup();

        if (strcmp(groupname, ginfo.gname) == 0)
        {
            close(fd);
            return groupinfocpy(&ginfo);
        }
    }
    
    close(fd);

    return 0;
}

struct userinfo*
getuinfouid(int uid)
{
    if (uid == 0)
        return getrootinfo();

    int fd = open("/etc/passwd", O_RDONLY);

    if (fd < 0)
    {
        printf("Greska prilikom otvaranja /etc/passwd.\n");
        return;
    }

    while (getnextline(fd, line) > 0) {
        linetouser();

        if (uid == uinfo.uid)
        {
            close(fd);
            return userinfocpy(&uinfo);
        }
    }
    
    close(fd);

    return 0;
}

struct groupinfo*
getginfogid(int gid)
{
    if (gid == 0)
        return getgrootinfo();

    int fd = open("/etc/group", O_RDONLY);

    if (fd < 0)
    {
        printf("Greska prilikom otvaranja /etc/group.\n");
        return;
    }

    while (getnextline(fd, line) > 0) {
        linetogroup();

        if (gid == ginfo.gid)
        {
            close(fd);
            return groupinfocpy(&ginfo);
        }
    }
    
    close(fd);

    return 0;
}

struct lista
{
    struct lista* sl;
    struct userinfo* ui;
};

struct listag
{
    struct listag* sl;
    struct groupinfo* gi;
};

// menjaju se sve informacije o useru osim uid
int
rewriteuseruid(struct userinfo* tuinfo)
{
    struct lista* glava = 0;

    struct lista* nu = malloc(sizeof(struct lista));

    nu->sl = 0;
    nu->ui = userinfocpy(tuinfo);

    int fd = open("/etc/passwd", O_RDONLY);

    if (fd < 0)
        return -1;

    while (getnextline(fd, line) > 0) {

        linetouser();

        if (nu->ui->uid == uinfo.uid)
        {
            nu->sl = glava;
            glava = nu;
        }
        else
        {
            struct lista* novi = malloc(sizeof(struct lista));

            novi->ui = userinfocpy(&uinfo);

            novi->sl = glava;
            glava = novi;
        }
    }
    close(fd);

    fd = open("/etc/passwd", O_WRONLY);

    if (fd < 0)
        return -1;

    while (glava != 0)
    {
        fprintf(fd, "%s:%s:%d:%d:%s:%s\n", glava->ui->uname, glava->ui->passw, glava->ui->uid, glava->ui->gid, glava->ui->name, glava->ui->dir);
        glava = glava->sl;
    }

    close(fd);

    return 0;
}


// menjaju se sve informacije o useru osim uname
int
rewriteuseruname(struct userinfo* tuinfo)
{
    struct lista* glava = 0;

    struct lista* nu = malloc(sizeof(struct lista));

    nu->sl = 0;
    nu->ui = userinfocpy(tuinfo);

    int fd = open("/etc/passwd", O_RDONLY);

    if (fd < 0)
        return -1;

    while (getnextline(fd, line) > 0) {

        linetouser();

        if (strcmp(nu->ui->uname, uinfo.uname) == 0)
        {
            nu->sl = glava;
            glava = nu;
        }
        else
        {
            struct lista* novi = malloc(sizeof(struct lista));

            novi->ui = userinfocpy(&uinfo);

            novi->sl = glava;
            glava = novi;
        }
    }
    close(fd);

    fd = open("/etc/passwd", O_WRONLY);

    if (fd < 0)
        return -1;

    while (glava != 0)
    {
        fprintf(fd, "%s:%s:%d:%d:%s:%s\n", glava->ui->uname, glava->ui->passw, glava->ui->uid, glava->ui->gid, glava->ui->name, glava->ui->dir);
        glava = glava->sl;
    }

    close(fd);

    return 0;
}


int
writenewuser(struct userinfo* tuinfo)
{
    struct lista* glava = 0;

    struct lista* nu = malloc(sizeof(struct lista));

    nu->sl = 0;
    nu->ui = userinfocpy(tuinfo);

    int fd = open("/etc/passwd", O_RDONLY);

    if (fd < 0)
        return -1;

    nu->sl = glava;
    glava = nu;

    while (getnextline(fd, line) > 0) {

        linetouser();

        struct lista* novi = malloc(sizeof(struct lista));

        novi->ui = userinfocpy(&uinfo);

        novi->sl = glava;
        glava = novi;
    }

    close(fd);

    fd = open("/etc/passwd", O_WRONLY);

    if (fd < 0)
        return -1;

    while (glava != 0)
    {
        fprintf(fd, "%s:%s:%d:%d:%s:%s\n", glava->ui->uname, glava->ui->passw, glava->ui->uid, glava->ui->gid, glava->ui->name, glava->ui->dir);
        glava = glava->sl;
    }

    close(fd);

    return 0;
}


int
writenewgroup(struct groupinfo* tginfo)
{
    // listag je lista grupa koje se iz /etc/group ucitavaju u memoriju da bi se opet upisale u fajl
    struct listag* glava = 0;

    // jedan cvor koji sadrzi podatke o novoj grupi
    struct listag* ng = malloc(sizeof(struct listag));
    
    ng->gi = malloc(sizeof(struct groupinfo));

    ng->sl = 0;
    ng->gi = groupinfocpy(tginfo);

    int fd = open("/etc/group", O_RDONLY);

    if (fd < 0)
        return -1;

    ng->sl = glava;
    glava = ng;

    while (getnextline(fd, line) > 0) {

        linetogroup();

        struct listag* novi = malloc(sizeof(struct listag));
        
        novi->gi = groupinfocpy(&ginfo);
        novi->sl = glava;

        glava = novi;
    }

    close(fd);

    fd = open("/etc/group", O_WRONLY);

    if (fd < 0)
        return -1;

    while (glava != 0)
    {
        fprintf(fd, "%s:%d:", glava->gi->gname, glava->gi->gid);

        int first = 0;

        struct grlista* glava2 = glava->gi->glava;

        while (glava2 != 0)
        {
            if (first > 0)
                fprintf(fd, ",");
            first++;

            fprintf(fd, "%s", glava2->uname);

            glava2 = glava2->sl;
        }

        fprintf(fd, "\n");

        glava = glava->sl;
    }

    close(fd);

    return 0;
}

int
getgroupsuname(const char* uname, int* n, int* gids)
{
    *n = 0;

    int fd = open("/etc/group", O_RDONLY);

    if (fd < 0)
        return -1;

    if (strcmp(uname, "root") == 0)
    {
        gids[*n] = 0;
        (*n)++;
    }

    while (getnextline(fd, line) > 0) {

        linetogroup();

        struct grlista* glava = ginfo.glava;

        while (glava != 0)
        {
            if (strcmp(glava->uname, uname) == 0)
            {
                gids[*n] = ginfo.gid;
                (*n)++;
                break;
            }

            glava = glava->sl;
        }
    }

    close(fd);

    return 0;
}

int
addutogname(const char* uname, const char* gname, int finda)
{
    // listag je lista grupa koje se iz /etc/group ucitavaju u memoriju da bi se opet upisale u fajl
    struct listag* glava = 0;

    int fd = open("/etc/group", O_RDONLY);

    if (fd < 0)
        return -1;

    while (getnextline(fd, line) > 0) {

        linetogroup();

        struct listag* novi = malloc(sizeof(struct listag));
        
        novi->gi = groupinfocpy(&ginfo);
        novi->sl = glava;

        glava = novi;
    }

    close(fd);

    fd = open("/etc/group", O_WRONLY);

    if (fd < 0)
        return -1;

    int nasao = 0;

    while (glava != 0)
    {
        fprintf(fd, "%s:%d:", glava->gi->gname, glava->gi->gid);

        int first = 0;

        struct grlista* glava2 = glava->gi->glava;

        while (glava2 != 0)
        {
            if (first > 0)
                fprintf(fd, ",");

            if (strcmp(gname, glava->gi->gname) == 0 && strcmp(glava2->uname, uname) == 0)
                nasao = 1;

            // ne mozemo izbaciti korisnika iz njegove trivijalne grupe
            if (strcmp(glava->gi->gname, uname) == 0 || strcmp(gname, glava->gi->gname) || (strcmp(gname, glava->gi->gname) == 0 && (strcmp(glava2->uname, uname) || finda)))
            {
                fprintf(fd, "%s", glava2->uname);
                first++;
            }

            glava2 = glava2->sl;
        }

        if (!nasao && strcmp(gname, glava->gi->gname) == 0)
        {
            if (first > 0)
                fprintf(fd, ",");
            fprintf(fd, "%s", uname);
        }

        fprintf(fd, "\n");

        glava = glava->sl;
    }

    close(fd);

    return 0;
}


int
rewritegroupsuname(const char* oldname, const char* newname)
{
    // listag je lista grupa koje se iz /etc/group ucitavaju u memoriju da bi se opet upisale u fajl
    struct listag* glava = 0;

    int fd = open("/etc/group", O_RDONLY);

    if (fd < 0)
        return -1;

    while (getnextline(fd, line) > 0) {

        linetogroup();

        struct listag* novi = malloc(sizeof(struct listag));
        
        novi->gi = groupinfocpy(&ginfo);
        novi->sl = glava;

        glava = novi;
    }

    close(fd);

    fd = open("/etc/group", O_WRONLY);

    if (fd < 0)
        return -1;

    while (glava != 0)
    {
        if (strcmp(oldname, glava->gi->gname) == 0)
                fprintf(fd, "%s:%d:", newname, glava->gi->gid);
            else
                fprintf(fd, "%s:%d:", glava->gi->gname, glava->gi->gid);

        int first = 0;

        struct grlista* glava2 = glava->gi->glava;

        while (glava2 != 0)
        {
            if (first > 0)
                fprintf(fd, ",");
            first++;

            if (strcmp(oldname, glava2->uname) == 0)
                fprintf(fd, "%s", newname);
            else
                fprintf(fd, "%s", glava2->uname);

            glava2 = glava2->sl;
        }

        fprintf(fd, "\n");

        glava = glava->sl;
    }

    close(fd);

    return 0;
}
