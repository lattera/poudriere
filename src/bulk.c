#include <sys/types.h>
#include <sys/sbuf.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <err.h>

#include "commands.h"
#include "utils.h"
#include "poudriere.h"

void
usage_bulk(void)
{
	fprintf(stderr, "usage: poudriere bulk parameters [options]\n\n");
	fprintf(stderr,"Parameters:\n");
	fprintf(stderr,"\t%-15s%s\n\n", "-f", "Give the list of ports to build");
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"\t%-15s%s\n", "-k", "Keep the previous built binary packages");
	fprintf(stderr,"\t%-15s%s\n", "-t", "Add some testings to package building");
	fprintf(stderr,"\t%-15s%s\n", "-j", "Run only on the given jail");
	fprintf(stderr,"\t%-15s%s\n", "-p", "Specify on which ports tree the bulk will be done");
}

int
exec_bulk(int argc, char **argv)
{
	signed char ch;
	const char *file = NULL;
	bool keep = false;
	bool test = false;
	char *jail = NULL;
	char *porttree = "default";
	struct pjail j;
	struct pport_tree p;

	struct zfs_query qj[] = {
		{ "poudriere:version", STRING, j.version, sizeof(j.version), 0 },
		{ "poudriere:arch", STRING, j.arch, sizeof(j.arch), 0 },
		{ "poudriere:stats_built", INTEGER, NULL, 0,  j.built },
		{ "poudriere:stats_failed", INTEGER, NULL, 0,  j.failed },
		{ "poudriere:stats_ignored", INTEGER, NULL, 0,  j.ignored },
		{ "poudriere:stats_queued", INTEGER, NULL, 0, j.queued },
		{ "poudriere:status", STRING, j.status, sizeof(j.status), 0 },
		{ "mountpoint", STRING, j.mountpoint, sizeof(j.mountpoint), 0 },
		{ "name", STRING, j.fs, sizeof(j.fs), 0 },
	};

	struct zfs_query qp[] = {
		{ "poudriere:method", STRING, p.method, sizeof(p.method), 0 },
		{ "mountpoint", STRING, p.mountpoint, sizeof(p.mountpoint), 0 },
		{ "name", STRING, p.fs, sizeof(p.fs), 0 },
	};

	while ((ch = getopt(argc, argv, "f:ktj:p:")) != -1) {
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 'k':
			keep = true;
			break;
		case 't':
			test = true;
			break;
		case 'j':
			jail = optarg;
			break;
		case 'p':
			porttree = optarg;
			break;
		default:
			usage_bulk();
			return (EX_USAGE);
		}
	}

	argc -= optind;
	argv += optind;

	if (jail == NULL) {
		usage_bulk();
		return (EX_USAGE);
	}

	strlcpy(j.name, jail, sizeof(j.name));
	strlcpy(p.name, porttree, sizeof(p.name));

	if (!zfs_query("rootfs", jail, qj, sizeof(qj) / sizeof(struct zfs_query)))
		err(EX_USAGE, "No such jail %s", jail);

	if (!zfs_query("ports", porttree, qp, sizeof(qp) / sizeof(struct zfs_query)))
		err(EX_USAGE, "No such ports tree %s", porttree);

	jail_start(&j);
	mount_nullfs(&j, &p);
	sleep(60);
	jail_stop(&j);
	return (EX_OK);
}
