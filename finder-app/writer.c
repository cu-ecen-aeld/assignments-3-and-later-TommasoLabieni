#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
    openlog(NULL, 0, LOG_USER);

    if (argc != 3)
    {
        syslog(LOG_ERR, "Wrong number of params. Usage is: %s <outfile> <str>\n", argv[0]);

        closelog();
        exit(1);
    }

    FILE *f = fopen(argv[1], "w");

    if (f == NULL)
    {
        syslog(LOG_ERR, "ERROR While opening file %s\n", argv[1]);

        closelog();
        exit(2);
    }

    if (fputs(argv[2], f) == EOF)
    {
        syslog(LOG_ERR, "ERROR While writing string %s to file %s\n", argv[2], argv[1]);

        closelog();
        fclose(f);
        exit(3);
    }

    syslog(LOG_DEBUG, "Writing %s to %s\n", argv[2], argv[1]);
    closelog();
    fclose(f);

    return 0;
}