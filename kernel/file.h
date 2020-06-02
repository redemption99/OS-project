struct file {
	enum { FD_NONE, FD_PIPE, FD_INODE } type;
	int ref; // reference count
	char readable;
	char writable;
	struct pipe *pipe;
	struct inode *ip;
	uint off;
};


// in-memory copy of an inode
struct inode {
	uint dev;           // Device number
	uint inum;          // Inode number
	int ref;            // Reference count
	struct sleeplock lock; // protects everything below here
	int valid;          // inode has been read from disk?

	short type;         // copy of disk inode
	short major;
	short minor;
	short nlink;
	uint size;
	uint addrs[NDIRECT+1];

	int uid;
	int gid;
	int mode;
};

// table mapping major device number to
// device functions
struct devsw {
	int (*read)(struct inode*, char*, int);
	int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1


struct userinfo {
	char* uname; // username
	char* passw; // password
	int uid;	 // uid
	int gid;	 // gid
	char* name;  // real name of user
	char* dir;   // home dir
};

struct grlista
{
	struct grlista* sl;	
	char* uname;
};

struct groupinfo {
	char* gname;		   // groupname
	int gid;			   // gid
	struct grlista* glava; // list of users
};



#define MAX_LENGTH 32
#define SETUID	   1<<12
#define DEFMOD	   0644
#define DEFDIR	   0755
