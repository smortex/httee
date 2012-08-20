#include <sys/stat.h>

#include <basedir.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct outlog {
    char *domain;
    FILE *log;
};

struct outlog *outlog = NULL;
int outlog_size = 0;

FILE *
get_log_file (char *domain, const char *mode)
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
	char *src = domain + 4;
	char *dst = domain;
	while (*src)
	    *dst++ = *src++;
	*dst++ = *src++;
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
	fclose (outlog[i].log);
	free (outlog[i].domain);
    }
    free (outlog);
    outlog = NULL;
    outlog_size = 0;
    return 0;
}

int
process_file (const char const *file_name, int skip_lines)
{
    FILE *f;
    char buffer[1024 * 4];
    char *mode = "w";

    int start = 0;
    int end = 0;

    int blank = 0;
    int corrupted = 0;
    int parsed = 0;

    const char *stdin_name = "<stdin>";

    if (!file_name) {
	fprintf (stdout, "Processing <stdin>.\n");
	file_name = stdin_name;
	f = fdopen (dup (0), "r");
    } else {
	fprintf (stdout, "Processing file \"%s\".\n", file_name);

	if (!(f = fopen (file_name, "r"))) {
	    perror ("fopen");
	    return -1;
	}
    }

    if (skip_lines) {
	fprintf (stdout, "Direct access after line %d.\n", skip_lines);
	while (skip_lines--) {
	    if (!fgets (buffer, sizeof (buffer), f)) {
		warnx ("%s: EOF reached after skipping %d lines (%d lines still to be skipped).", file_name, start, skip_lines + 1);
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
	} else {
	    corrupted++;
	}

    }

    close_log_files ();
    fclose (f);

    fprintf (stdout, "Parsed lines in file: %d\n", end);
    fprintf (stdout, " Found %d blank records,\n", blank);
    fprintf (stdout, " Found %d corrupted records,\n", corrupted);
    fprintf (stdout, " Found %d old records,\n", start);
    fprintf (stdout, " Found %d new qualified records.\n", parsed);

    return end;
}

int
config_read_file_skip_lines (const char *config_file, const char *file_name)
{
    FILE *config;
    int skip_lines = 0;

    char config_file_name[BUFSIZ];
    char config_skip_line[BUFSIZ];

    if ((config = fopen (config_file, "r"))) {
	while (fgets (config_file_name, sizeof (config_file_name), config)) {
	    if (fgets (config_skip_line, sizeof (config_skip_line), config)) {
		config_file_name[strlen (config_file_name) -1] = '\0';
		if (0 == strcmp (config_file_name, file_name)) {
		    config_skip_line[strlen (config_skip_line) -1] = '\0';
		    if (1 != sscanf (config_skip_line, "%d", &skip_lines))
			warnx ("Invalid line number \"%s\" for file \"%s\" in configuration file \"%s\".", config_skip_line, config_file_name, config_file);
		    break;
		}
	    } else {
		warnx ("Malformed configuration file \"%s\".", config_file);
	    }
	}
	fclose (config);
    }
    return skip_lines;
}

int
config_write_file_skip_lines (const char *config_file, const char *file_name, int skip_lines)
{
    int ret = 0;
    int wrote = 0;

    char tmp[BUFSIZ];
    snprintf (tmp, sizeof (tmp), "%s~", config_file);

    FILE *src, *dst;
    char config_file_name[BUFSIZ];
    char config_skip_line[BUFSIZ];

    if ((dst = fopen (tmp, "w"))) {
	if ((src = fopen (config_file, "r"))) {
	    while (fgets (config_file_name, sizeof (config_file_name), src)) {
		if (fgets (config_skip_line, sizeof (config_skip_line), src)) {
		    config_file_name[strlen (config_file_name) -1] = '\0';
		    if (0 == strcmp (config_file_name, file_name)) {
			snprintf (config_skip_line, sizeof (config_skip_line), "%d\n", skip_lines);
			wrote = 1;
		    }
		    if (fprintf (dst, "%s\n%s", config_file_name, config_skip_line) < 0) {
			warn ("Error writing to \"%s\".", config_file);
			ret = -1;
			break;
		    }
		} else {
		    warnx ("Malformed configuration file \"%s\".", config_file);
		}
	    }

	    fclose (src);
	}
	if (!wrote) {
	    fprintf (dst, "%s\n%d\n", file_name, skip_lines);
	}
	fclose (dst);
    }

    if (ret >= 0)
	if (rename (tmp, config_file) < 0)
	    perror ("rename");

    return ret;
}

void
usage (void)
{
    fprintf (stderr, "usage: httee  [-RW] < -s number file | file ... >\n");
}

int
main (int argc, char *argv[])
{
    int ch;
    int skip_lines = 0;
    int read_config = 1, write_config = 1;

    while ((ch = getopt (argc, argv, "hRs:W")) != -1) {
	switch (ch) {
	case 's':
	    if (1 != sscanf (optarg, "%d", &skip_lines))
		errx (EXIT_FAILURE, "%s: not a number.", optarg);
	    break;
	case 'h':
	    usage ();
	    exit (EXIT_SUCCESS);
	    break;
	case 'R':
	    read_config = 0;
	    break;
	case 'W':
	    write_config = 0;
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

    xdgHandle xdg_handle;
    xdgInitHandle (&xdg_handle);
    const char *xdg_data_home = xdgDataHome (&xdg_handle);

    char config[BUFSIZ];
    snprintf (config, sizeof (config), "%s/httee", xdg_data_home);
    mkdir (config, 0777);

    snprintf (config, sizeof (config), "%s/httee/positions", xdg_data_home);

    for (int i = 0; i < argc; i++) {
	char *path = realpath (argv[i], NULL);
	if (!path) {
	    if (0 != strcmp ("-", argv[i])) {
		err (EXIT_FAILURE, "realpath(\"%s\")", argv[i]);
	    }
	    read_config = write_config = 0;
	}
	if (read_config && !skip_lines)
	    skip_lines = config_read_file_skip_lines (config, path);
	if ((skip_lines = process_file (path, skip_lines)) < 0)
	    exit (EXIT_FAILURE);
	if (write_config)
	    config_write_file_skip_lines (config, path, skip_lines);
	free (path);
	skip_lines = 0;
    }
    xdgWipeHandle (&xdg_handle);
    exit (EXIT_SUCCESS);
}
