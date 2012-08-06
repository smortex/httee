#include <err.h>
#include <stdio.h>
#include <stdlib.h>

int
process_file (char *file_name)
{
    FILE *f;
    char buffer[1024 * 4];

    if (!(f = fopen (file_name, "r"))) {
	perror ("fopen");
	return -1;
    }

    while (fgets (buffer, sizeof (buffer), f)) {
	char ip[BUFSIZ];
	char hostname[BUFSIZ];

	if (sscanf (buffer, "%s %s", ip, hostname) != 2) {
	    warnx ("Malformed line: %s", buffer);
	    continue;
	}

	FILE *log;

	if (!(log = fopen (hostname, "a"))) {
	    perror ("fopen");
	    return -1;
	}

	if (fprintf (log, "%s", buffer) < 0) {
	    perror ("fprintf");
	    return -1;
	}

	if (fclose (log) < 0) {
	    perror ("fclose");
	    return -1;
	}

    }

    fclose (f);

    return 0;
}

int
main (int argc, char *argv[])
{
    for (int i = 1; i <= argc; i++) {
	if (process_file (argv[i]) < 0)
	    exit (EXIT_FAILURE);
    }
    exit (EXIT_SUCCESS);
}
