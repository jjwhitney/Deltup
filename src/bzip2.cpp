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
#include "bzip2.h"

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
	char * line = NULL;
	string fname;
	size_t len = 0;
	ssize_t read;
	fp = fopen(tempfile.c_str(), "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);
	while ((read = getline(&line, &len, fp)) != -1) {
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
	if (line)
		free(line);
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
