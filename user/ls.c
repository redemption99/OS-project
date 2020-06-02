#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/file.h"

char ret[11]; 

char*
strmode(int mode)
{
	//char ret[11] = "----------";

	ret[0] = mode & 1 << 9 ? 'd' : '-';
	ret[1] = mode & 1 << 8 ? 'r' : '-';
	ret[2] = mode & 1 << 7 ? 'w' : '-';
	ret[3] = mode & 1 << 6 ? 'x' : '-';
	ret[4] = mode & 1 << 5 ? 'r' : '-';
	ret[5] = mode & 1 << 4 ? 'w' : '-';
	ret[6] = mode & 1 << 3 ? 'x' : '-';
	ret[7] = mode & 1 << 2 ? 'r' : '-';
	ret[8] = mode & 1 << 1 ? 'w' : '-';
	ret[9] = mode & 1 << 0 ? 'x' : '-';

	ret[10] = 0;

	return ret;
}

char*
fmtname(char *path)
{
	static char buf[DIRSIZ+1];
	char *p;

	// Find first character after last slash.
	for(p=path+strlen(path); p >= path && *p != '/'; p--)
		;
	p++;

	// Return blank-padded name.
	if(strlen(p) >= DIRSIZ)
		return p;
	memmove(buf, p, strlen(p));
	memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
	return buf;
}

void
ls(char *path)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, 0)) < 0){
		fprintf(2, "ls: cannot open %s\n", path);
		return;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}

	struct userinfo* ui = getuinfouid(st.uid);
	struct groupinfo* gi = getginfogid(st.gid);

	switch(st.type){
	case T_FILE:
		printf("%s %s %s %s %d %d %d\n", strmode(st.mode), ui->uname, gi->gname, fmtname(path), st.type, st.ino, st.size);
		break;

	case T_DIR:
		if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
			printf("ls: path too long\n");
			break;
		}
		strcpy(buf, path);
		p = buf+strlen(buf);
		*p++ = '/';
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if(stat(buf, &st) < 0){
				printf("ls: cannot stat %s\n", buf);
				continue;
			}
			ui = getuinfouid(st.uid);
			gi = getginfogid(st.gid);

			printf("%s %s %s %s %d %d %d\n", strmode(st.mode | (st.type == T_DIR ? 1 : 0)<<9), ui->uname, gi->gname, fmtname(buf), st.type, st.ino, st.size);
		}
		break;
	}
	close(fd);
}

int
main(int argc, char *argv[])
{
	int i;

	if(argc < 2){
		ls(".");
		exit();
	}
	for(i=1; i<argc; i++)
		ls(argv[i]);

	exit();
}
