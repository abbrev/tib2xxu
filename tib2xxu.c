/*
 * tib2xxu.c, Convert tib file to xxu file
 *
 * Copyright 2014 Christopher Williams
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

static char xxuheader[] = {
	/* Signature */
	'*', '*', 'T', 'I', 'F', 'L', '*', '*',

	/* Version: major.minor (BCD) */
	0x03, 0x00,

	/* Flags */
	0x00,

	/* Object type */
	0x00,

	/* Date: mmddyyyy (BCD) */
	0x00, 0x00, 0x00, 0x00,

	/* Name length */
	0x08,

	/* Name */
	'b', 'a', 's', 'e', 'c', 'o', 'd', 'e',

	/* Filler */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,

	/* Device type */
	0x00,

	/* Data type (0x23 = OS) */
	0x23,

	/* Filler */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00,

	/* Hardware ID */
	0x00, 0x00,

	/* OS size */
	0x00, 0x00, 0x00, 0x00,
};

static const char *argv0;

static void usage()
{
	fprintf(stderr,
"Usage: %s [-t type] infile.tib outfile.??u\n"
"       %s -h\n"
"Options:\n"
"       -t type  Calculator type (89u, 9xu, v2u). Output file extension is\n"
"                used if this option is not given\n"
"       -h       Display this message and exit\n", argv0, argv0);
}

static void die(const char *what, const char *msg)
{
	fprintf(stderr, "%s: %s: %s\n", argv0, what, msg);
	exit(1);
}

static int xxutype(const char *ext)
{
	if (strcmp(ext, "89u") == 0) return 0x98;
	if (strcmp(ext, "9xu") == 0) return 0x88;
	if (strcmp(ext, "v2u") == 0) return 0x88;
	die(ext, "Unknown file extension");
	return 0;
}

static int xxufiletype(const char *filename)
{
	const char *ext = strrchr(filename, '.');
	if (!ext) {
		die(filename, "No file extension");
	}

	return xxutype(ext + 1);
}

static void pdie(const char *msg)
{
	int err = errno;
	fprintf(stderr, "%s: ", argv0);
	errno = err;
	perror(msg);
	exit(1);
}

static int int2bcd(int n)
{
	return ((n / 10) << 4) | (n % 10);
}

int main(int argc, char *argv[])
{
	const char *infilename;
	const char *outfilename;

	size_t n;

	int type = 0;
	FILE *infile;
	FILE *outfile;

	char buf[BUFSIZ];

	size_t insize = 0;

	int opt;

	time_t now;

	struct tm *tm;

	int month;
	int day;
	int year;

	argv0 = argv[0];

	while ((opt = getopt(argc, argv, "t:h")) != -1) {
		switch (opt) {
		case 't':
			type = xxutype(optarg);
			break;
		case 'h':
			usage();
			exit(0);
		default:
			usage();
			exit(1);
		}
	}

	if (optind + 2 > argc) {
		usage();
		exit(1);
	}

	infilename = argv[optind++];
	outfilename = argv[optind++];

	if (!type) {
		type = xxufiletype(outfilename);
	}

	infile = fopen(infilename, "rb");
	if (!infile) {
		pdie(infilename);
	}

	outfile = fopen(outfilename, "wb");
	if (!outfile) {
		pdie(outfilename);
	}

	// skip over header (fill it out and write it later)
	if (fseek(outfile, sizeof xxuheader, SEEK_SET)) {
		pdie(outfilename);
	}

	/* copy input file to output file */

	while ((n = fread(buf, 1, sizeof buf, infile))) {
		insize += n;
		if (fwrite(buf, 1, n, outfile) != n) {
			pdie(outfilename);
		}
	}

	/* fill out the header */

	now = time(NULL);

	tm = localtime(&now);

	month = tm->tm_mon + 1;
	day = tm->tm_mday;
	year = tm->tm_year + 1900;

	// BCD date
	xxuheader[12] = int2bcd(month);
	xxuheader[13] = int2bcd(day);
	xxuheader[14] = int2bcd(year / 100);
	xxuheader[15] = int2bcd(year % 100);

	// calculator type
	xxuheader[0x30] = type;

	// tib size
	xxuheader[0x4a] = insize >>  0;
	xxuheader[0x4b] = insize >>  8;
	xxuheader[0x4c] = insize >> 16;
	xxuheader[0x4d] = insize >> 24;

	/* write xxu header to beginning of output file */

	rewind(outfile);

	n = fwrite(xxuheader, 1, sizeof xxuheader, outfile);
	if (n != sizeof xxuheader) {
		pdie(outfilename);
	}

	fclose(outfile);
	fclose(infile);

	return 0;
}

