#include "kernel/types.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/file.h"
#include "kernel/param.h"
#include "kernel/mmu.h"
#include "kernel/proc.h"

int
oct(char* s)
{
    int ret = 0;
    while (*s)
    {
        ret = ret * 8 + *s - '0';
        *s++;
    }

    return ret;
}


int
main(int argc, char *argv[])
{
    
    if (argv[1][0] >= '0' && argv[1][0] <= '9')
    {
        int mode = oct(argv[1]);

        for (int i = 2; i < argc; i++)
            if (chmod(argv[i], mode) < 0) 
            {
                printf("Greska, mod za %s nije promenjen.\n", argv[i]);
            }
            else
                printf("Mod za %s je promenjen.\n", argv[i]);
    }
    else
    {
        int nmode = 0111;

        if (argv[1][0] == 'u') nmode = 0100;
        if (argv[1][0] == 'g') nmode = 0010;
        if (argv[1][0] == 'o') nmode = 0001;

        if (argv[1][2] == 'r') nmode <<= 2;
        if (argv[1][2] == 'w') nmode <<= 1;

        if (argv[1][1] == '-') nmode = 0777 - nmode;

        for (int i = 2; i < argc; i++)
        {
            struct stat st;

            stat(argv[i], &st);

            int tmode = st.mode;

            int mode;

            if (argv[1][1] == '-') mode = tmode & nmode;
            if (argv[1][1] == '+') mode = tmode | nmode;

            // ukoliko je setuid bit bio setovan treba da ostane setovan
            if (tmode & SETUID)
                mode |= SETUID;

            if (chmod(argv[i], mode) < 0) 
            {
                printf("Greska, mod za %s nije promenjen.\n", argv[i]);
            }
            else
                printf("Mod za %s je promenjen.\n", argv[i]);
        }
    }

    exit();
}
