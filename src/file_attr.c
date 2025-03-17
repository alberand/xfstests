#include "global.h"
#include <sys/syscall.h>
#include <getopt.h>
#include <errno.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

#if !defined(SYS_file_getattr) && defined(__x86_64__)
#define SYS_file_getattr 468
#define SYS_file_setattr 469
#endif

#define SPECIAL_FILE(x) \
	   (S_ISCHR((x)) \
	|| S_ISBLK((x)) \
	|| S_ISFIFO((x)) \
	|| S_ISLNK((x)) \
	|| S_ISSOCK((x)))

static struct option long_options[] = {
	{"set",		no_argument,	0,	's' },
	{"get",		no_argument,	0,	'g' },
	{"no-follow",	no_argument,	0,	'n' },
	{"at-cwd",	no_argument,	0,	'a' },
	{"set-nodump",	no_argument,	0,	'd' },
	{0,		0,		0,	0 }
};

static struct xflags {
	uint	flag;
	char	*shortname;
	char	*longname;
} xflags[] = {
	{ FS_XFLAG_REALTIME,		"r", "realtime"		},
	{ FS_XFLAG_PREALLOC,		"p", "prealloc"		},
	{ FS_XFLAG_IMMUTABLE,		"i", "immutable"	},
	{ FS_XFLAG_APPEND,		"a", "append-only"	},
	{ FS_XFLAG_SYNC,		"s", "sync"		},
	{ FS_XFLAG_NOATIME,		"A", "no-atime"		},
	{ FS_XFLAG_NODUMP,		"d", "no-dump"		},
	{ FS_XFLAG_RTINHERIT,		"t", "rt-inherit"	},
	{ FS_XFLAG_PROJINHERIT,		"P", "proj-inherit"	},
	{ FS_XFLAG_NOSYMLINKS,		"n", "nosymlinks"	},
	{ FS_XFLAG_EXTSIZE,		"e", "extsize"		},
	{ FS_XFLAG_EXTSZINHERIT,	"E", "extsz-inherit"	},
	{ FS_XFLAG_NODEFRAG,		"f", "no-defrag"	},
	{ FS_XFLAG_FILESTREAM,		"S", "filestream"	},
	{ FS_XFLAG_DAX,			"x", "dax"		},
	{ FS_XFLAG_COWEXTSIZE,		"C", "cowextsize"	},
	{ FS_XFLAG_HASATTR,		"X", "has-xattr"	},
	{ 0, NULL, NULL }
};

static int
file_getattr(
		int		dfd,
		const char	*filename,
		struct fsxattr	*fsx,
		size_t		usize,
		unsigned int	at_flags)
{
	return syscall(SYS_file_getattr, dfd, filename, fsx, usize, at_flags);
}

static int
file_setattr(
		int		dfd,
		const char	*filename,
		struct fsxattr	*fsx,
		size_t		usize,
		unsigned int	at_flags)
{
	return syscall(SYS_file_setattr, dfd, filename, fsx, usize, at_flags);
}

void
printxattr(
	uint		flags,
	int		verbose,
	int		dofname,
	const char	*fname,
	int		dobraces,
	int		doeol)
{
	struct xflags	*p;
	int		first = 1;

	if (dobraces)
		fputs("[", stdout);
	for (p = xflags; p->flag; p++) {
		if (flags & p->flag) {
			if (verbose) {
				if (first)
					first = 0;
				else
					fputs(", ", stdout);
				fputs(p->longname, stdout);
			} else {
				fputs(p->shortname, stdout);
			}
		} else if (!verbose) {
			fputs("-", stdout);
		}
	}
	if (dobraces)
		fputs("]", stdout);
	if (dofname)
		printf(" %s ", fname);
	if (doeol)
		fputs("\n", stdout);
}

int main(int argc, char *argv[])
{
	int error;
	int c;
	const char *path = NULL;
	const char *path1 = NULL;
	const char *path2 = NULL;
	unsigned int at_flags = 0;
	unsigned int fsx_xflags = 0;
	int action = 0; /* 0 get; 1 set */
	struct fsxattr fsx = { 0 };
	struct stat status;
	int fd;
	int at_fdcwd = 0;

        while (1) {
            int option_index = 0;

            c = getopt_long_only(argc, argv, "", long_options, &option_index);
            if (c == -1)
                break;

            switch (c) {
	    case 's':
		action = 1;
		break;
	    case 'g':
		action = 0;
		break;
	    case 'n':
		at_flags |= AT_SYMLINK_NOFOLLOW;
		break;
	    case 'a':
		at_fdcwd = 1;
		break;
	    case 'd':
		fsx_xflags |= FS_XFLAG_NODUMP;
		break;
	    default:
		goto usage;
            }
        }

	if (!path1 && optind < argc)
		path1 = argv[optind++];
	if (!path2 && optind < argc)
		path2 = argv[optind++];

	if (at_fdcwd) {
		fd = AT_FDCWD;
		path = path1;
	} else if (!path2) {
		error = stat(path1, &status);
		if (error) {
			fprintf(stderr,
"Can not get file status of %s: %s\n", path1, strerror(errno));
			return error;
		}

		if (SPECIAL_FILE(status.st_mode)) {
			fprintf(stderr,
"Can not open special file %s without parent dir: %s\n", path1, strerror(errno));
			return errno;
		}

		fd = open(path1, O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "Can not open %s: %s\n", path1,
					strerror(errno));
			return errno;
		}
	} else {
		fd = open(path1, O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "Can not open %s: %s\n", path1,
					strerror(errno));
			return errno;
		}
		path = path2;
	}

	if (!path)
		at_flags |= AT_EMPTY_PATH;

	if (action) {
		error = file_getattr(fd, path, &fsx, sizeof(struct fsxattr),
				at_flags);
		if (error) {
			fprintf(stderr, "Can not get fsxattr on %s: %s\n", path,
					strerror(errno));
			return error;
		}

		fsx.fsx_xflags |= fsx_xflags;

		error = file_setattr(fd, path, &fsx, sizeof(struct fsxattr),
				at_flags);
		if (error) {
			fprintf(stderr, "Can not set fsxattr on %s: %s\n", path,
					strerror(errno));
			return error;
		}
	} else {
		error = file_getattr(fd, path, &fsx, sizeof(struct fsxattr),
				at_flags);
		if (error) {
			fprintf(stderr, "Can not get fsxattr on %s: %s\n", path,
					strerror(errno));
			return error;
		}

		if (path2)
			printxattr(fsx.fsx_xflags, 0, 1, path, 0, 1);
		else
			printxattr(fsx.fsx_xflags, 0, 1, path1, 0, 1);
	}

	return error;

usage:
	printf("Usage: %s [options]\n", argv[0]);
	printf("Options:\n");
	printf("\t--get\t\tget filesystem inode attributes\n");
	printf("\t--set\t\tset filesystem inode attributes\n");
	printf("\t--at-cwd\t\topen file at current working directory\n");
	printf("\t--no-follow\t\tdon't follow symlinks\n");
	printf("\t--set-nodump\t\tset FS_XFLAG_NODUMP on an inode\n");

	return 1;
}
