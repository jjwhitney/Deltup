/* Copyright (C) 2003-2008  John Whitney
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: John Whitney <jjw@deltup.org>
 */

#include <string>
#include <list>
using namespace std;
#include "file.h"
#include "system.h"
#include "tmpstore.h"
#include "archfunc.h"
#include <stdio.h>


// bzip2 section

char *bzip2_compressor_name[MAX_BZIP2_COMPRESSORS] =
	{"0.9.0c", "1.0.2", "1.0.3", "1.0.4"};
char *bzip2_name[MAX_BZIP2_COMPRESSORS] =
	{NULL, NULL, NULL, NULL};

void find_bzip2_compressors() {
	string tempfile = getTmpFilename();
	string command = "find `echo $PATH | tr \":\" \" \"` -iname \"bzip2*\" -exec sh -c 'echo {}; {} -h 2>&1 | grep \"Version\"' \\; 2> /dev/null > "
		+ tempfile;

	system(command.c_str());

	FILE * fp;
	char line[2 * CHAR_MAX];
	string fname;
	fp = fopen(tempfile.c_str(), "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);
	while (fgets(line, 2 * CHAR_MAX, fp) != NULL) {	
		// printf("Retrieved line of length %zu :\n", read);
		// printf("%s", line);
		char *v = strstr(line, "Version");
		if (v) {
			int index = -1;
			if (strncmp(v+8, "1.0.2", 5) == 0) index = 1;
			if (strncmp(v+8, "1.0.3", 5) == 0) index = 2;
			if (strncmp(v+8, "1.0.4", 5) == 0) index = 3;
			if (index!=-1) {
				bzip2_name[index] = new char[fname.length()];
				strncpy(bzip2_name[index], fname.c_str(), fname.length()-1);
				bzip2_name[index][fname.length()-1] = 0;
			}
			// printf("%s %s", v+8, fname.c_str());
		}
		fname = line;
	}
	if (verbose) {
		printf("found bzip2 compressors/decompressors:\n");
		for (int i = 0; i < MAX_BZIP2_COMPRESSORS; ++i)
			if (bzip2_name[i]!=NULL) printf("  %s\n", bzip2_name[i]);
	}
}

int find_bz2_compression_level(string file) {
	FILE *f = fopen(file.c_str(), "rb");

	char header[4];
	fread(header, 1, 4, f);
	fclose(f);
	return (header[3])-'0'; // Assumes ASCII char set.
}


// gzip section

char *gzip_name = NULL;

void find_gzip_compressor() {
	string tempfile = getTmpFilename();
	string command = "find `echo $PATH | tr ':' ' '` -iname 'gzip' -exec sh -c 'echo {};{} -V 2>&1|grep ^gzip' \\; 2> /dev/null > "
		+ tempfile;

	system(command.c_str());

	FILE * fp;
	char line[2*CHAR_MAX];
	string fname;
	fp = fopen(tempfile.c_str(), "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);
	while (fgets(line, 2*CHAR_MAX, fp)!=NULL) {
		char *v = strstr(line, "gzip");       
		if (v) {
			int index=-1;
			if (strncmp(v+5, "1.", 2) == 0) index=0;
			if (index!=-1) {
				gzip_name = new char[fname.length()];
				strncpy(gzip_name, fname.c_str(), fname.length()-1);
				gzip_name[fname.length()-1] = 0;
				break;
			}
		}
		fname = line;
	}
	if (verbose) {
		if (gzip_name!=NULL) {
			printf("found GNU gzip compressor/decompressor:\n");
			printf("  %s\n", gzip_name);
		}
		else printf("GNU gzip compressor/decompressor NOT found!\n");
	}
}
