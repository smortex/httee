#include <ctype.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct outlog {
    char *domain;
    FILE *log;
};

struct outlog *outlog = NULL;
int outlog_size = 0;

FILE *
get_log_file (char *domain, char *mode)
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

    if ((c = strstr (domain, ":80")))
	*c = '\0';

    if (domain == strstr (domain, "www.")) {
	strcpy (domain, domain + 4);
    }

    for (int i = 0; i < outlog_size; i++)
	if (0 == strcmp (domain, outlog[i].domain))
	    return outlog[i].log;

    FILE *new_log;
    if (!(new_log = fopen (domain, mode))) {
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
process_file (char *file_name, int skip_lines)
{
    FILE *f;
    char buffer[1024 * 4];
    char *mode = "w";

    int start = 0;
    int end = 0;

    int blank = 0;
    int corrupted = 0;
    int parsed = 0;

    fprintf (stdout, "Processing file \"%s\".\n", file_name);

    if (!(f = fopen (file_name, "r"))) {
	perror ("fopen");
	return -1;
    }

    if (skip_lines) {
	fprintf (stdout, "Direct access after line %d.\n", skip_lines);
	while (skip_lines--) {
	    if (!fgets (buffer, sizeof (buffer), f)) {
		warnx ("%s: EOF reached after skipping %d (%d lines still to be skipped).", file_name, start, skip_lines);
		return -1;
	    }
	    start++;
	}

	fprintf (stdout, "Jumped lines in file: %d\n", start);
	fprintf (stdout, " Found %d already parsed records.\n", start);

	mode = "a";
    }

    end = start;

    while (fgets (buffer, sizeof (buffer), f)) {
	char ip[BUFSIZ];
	char hostname[BUFSIZ];

	end++;

	if (buffer[0] == '\n') {
	    blank++;
	    continue;
	}

	if (sscanf (buffer, "%s %s", ip, hostname) != 2) {
	    corrupted++;
	    continue;
	}

	FILE *log;

	if ((log = get_log_file (hostname, mode))) {
	    if (fprintf (log, "%s", buffer) < 0) {
		perror ("fprintf");
		return -1;
	    }
	    parsed++;
	}

    }

    close_log_files ();
    fclose (f);

    fprintf (stdout, "Parsed lines in file: %d\n", end);
    fprintf (stdout, " Found %d blank records,\n", blank);
    fprintf (stdout, " Found %d corrupted records,\n", corrupted);
    fprintf (stdout, " Found %d old records,\n", start);
    fprintf (stdout, " Found %d new qualified records.\n", parsed);

    return 0;
}

void
usage (void)
{
    fprintf (stderr, "usage: httee [-s lines] [file...]\n");
}

int
main (int argc, char *argv[])
{
    int ch;
    int skip_lines = 0;

    while ((ch = getopt (argc, argv, "hs:")) != -1) {
	switch (ch) {
	case 's':
	    if (1 != sscanf (optarg, "%d", &skip_lines))
		errx (EXIT_FAILURE, "%s: not a number.", optarg);
	    break;
	case 'h':
	    usage ();
	    exit (EXIT_SUCCESS);
	    break;
	case '?':
	    /* FALLTHROUGH */
	default:
	    usage ();
	    exit (EXIT_FAILURE);
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if (argc > 1 && skip_lines > 0)
	errx (EXIT_FAILURE, "option \"-s\" cannot be used with multiple input files.");

    for (int i = 0; i < argc; i++) {
	if (process_file (argv[i], skip_lines) < 0)
	    exit (EXIT_FAILURE);
    }
    exit (EXIT_SUCCESS);
}
