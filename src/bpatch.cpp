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

int do_bdelta_patch(const char *reffname,
		const char *newfname, const char *patchfname) {
	IFStream patchfile(patchfname);
	if (patchfile.bad()) {
		printf("cannot open patchfile (%s)\n", patchfname);
		return 1;
	}
	IFStream ref(reffname);
	if (ref.bad()) {
		printf("cannot open reference file (%s)\n", reffname);
		return 1;
	}

	char magic[3];
	patchfile.read(magic, 3);
	if (strncmp(magic, "BDT", 3)) {
		printf("Given file is not a recognized patchfile\n");
		return 1;
	}
	unsigned short version = read_word(patchfile);
	if (version!=1) {
		printf("unsupported patch version\n");
		return 1;
	}
	char intsize;
	patchfile.read(&intsize, 1);
	if (intsize!=4) {
		printf("unsupported file pointer size\n");
		return 1;
	}
	unsigned
		size1 = read_dword(patchfile), 
		size2 = read_dword(patchfile);

	unsigned nummatches = read_dword(patchfile);

	unsigned
		*copyloc1 = new unsigned[nummatches+1],
		*copyloc2 = new unsigned[nummatches+1],
		*copynum  = new unsigned[nummatches+1];

	for (unsigned i = 0; i < nummatches; ++i) {
		copyloc1[i] = read_dword(patchfile);
		copyloc2[i] = read_dword(patchfile);
		copynum[i] = read_dword(patchfile);
		size2-=copyloc2[i]+copynum[i];
	}
	if (size2) {
		copyloc1[nummatches]=0; copynum[nummatches]=0; 
		copyloc2[nummatches]=size2;
		++nummatches;
	}
  
	OFStream outfile(newfname);
	if (outfile.bad()) {
		printf("cannot open output file (%s)\n", newfname);
		return 1;
	}
  
	for (unsigned i = 0; i < nummatches; ++i) {
		if (!copy_bytes_to_file(patchfile, outfile, copyloc2[i])) {
			printf("Error.  patchfile is truncated\n");
			return -1;
		}

		int copyloc = copyloc1[i];
		ref+=copyloc;

		if (!copy_bytes_to_file(ref, outfile, copynum[i])) {
			printf("Error while copying from reference file\n");
			return -1;
		}
	}
	return 0;  
}
