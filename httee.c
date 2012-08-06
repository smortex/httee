#include <ctype.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct outlog {
    char *domain;
    FILE *log;
};

struct outlog *outlog = NULL;
int outlog_size = 0;

FILE *
get_log_file (char *domain)
{
    char *c = domain;
    while (*c) {
	if (!isprint (*c)) {
	    warnx ("invalid hostname");
	    return NULL;
	}
	*c = tolower (*c);
	c++;
    }

    for (int i = 0; i < outlog_size; i++)
	if (0 == strcmp (domain, outlog[i].domain))
	    return outlog[i].log;

    FILE *new_log;
    if (!(new_log = fopen (domain, "a"))) {
	warnx ("cannot open log file \"%s\" for writing.", domain);
	return NULL;
    }

    struct outlog *new_outlog = realloc (outlog, (outlog_size + 1) * sizeof (struct outlog));
    if (new_outlog) {
	outlog = new_outlog;
	outlog[outlog_size].domain = strdup (domain);
	outlog[outlog_size].log = new_log;
	outlog_size++;
	return outlog[outlog_size - 1].log;
    }

    err (EXIT_FAILURE, "malloc");
}

int
close_log_files (void)
{
    for (int i = 0; i < outlog_size; i++) {
	if (fclose (outlog[i].log) < 0)
	    return -1;
    }
    return 0;
}

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

	if ((log = get_log_file (hostname))) {
	    if (fprintf (log, "%s", buffer) < 0) {
		perror ("fprintf");
		return -1;
	    }
	}

    }

    close_log_files ();
    fclose (f);

    return 0;
}

int
main (int argc, char *argv[])
{
    for (int i = 1; i <= argc - 1; i++) {
	if (process_file (argv[i]) < 0)
	    exit (EXIT_FAILURE);
    }
    exit (EXIT_SUCCESS);
}
