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
 
#include <sys/signal.h>
#include <string>
#include <vector>
#include <list>
using namespace std;
#include "file.h"
#include "bpatch.h"
#include <sys/wait.h>
#include "filetypes.h"

bool verbose = false;

void error(string message) {
	fprintf(stderr, "error: %s\n", message.c_str());
	exit(1);
}

int invoke_system(string prog, string args) {
	// printf("%s\n", ret);
	int ret = system((prog + " " + args).c_str());
	if (WIFSIGNALED(ret) && 
			(WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
		error("Caught signal");
	if (ret==0x7F00) error(string("A required executable, ") + prog + ", was not found");
	return ret;
}

void remove_file(string name) {
	invoke_system("rm", string("-f ") + name);
}

int move_file(string from, string to) {
	return invoke_system("mv", from+" "+to);
}

int cat(string infile, string outfile, bool append) {
	return invoke_system("cat", infile+ (append?" >> ":" > ")+outfile+" 2> /dev/null");
}

int inflate(string command, string infile, string outfile, bool append) {
	return invoke_system(command, string("-c ") +
		infile + (append?" >> ":" > ")+outfile+" 2> /dev/null");
}

int deflate(string command, string infile, string outfile, char lev, bool append) {
	return invoke_system("cat", infile + " | " + command + " -" + lev +
		(append?" >> ":" > ") + outfile);
}

int makeDelta(bool bdelta, string file1, string file2, string deltaName) {
	if (bdelta)
		return invoke_system("bdelta", file1+" " +file2+" " + deltaName);
	else
		return invoke_system("xdelta", string("delta -0 --pristine ") +
			file1 + " " +
			file2 + " " +
			deltaName + " 2> /dev/null");
};

int determine_filetype(string s) {
	if (s.substr(0,3)=="DTU") return DTU;
	if (s.substr(0,2)=="\037\213" ||
		s.substr(0,2)=="\037\236") return GZIP;
	if (s.substr(0,2)=="BZ") return BZIP2;
	if (s.substr(0,4)=="PK\003\004") return ZIP;
	if (s.substr(0,4)=="%XDZ") return XDELTA;
	if (s.substr(0,3)=="BDT") return BDELTA;
	return UNKNOWN_FMT;
}

int determine_filetype(Injectable_IStream &f) {
	char c[4];
	f.read(c, 4);
	f.inject(c, 4);
	unsigned fmt = determine_filetype(c);
	if (fmt!=UNKNOWN_FMT) return fmt;
    
	char tarheader[512];
	f.read(tarheader, 512);
	f.inject(tarheader, 512);
	if (strncmp(tarheader+257, "ustar", 5)==0) return TARBALL;
	return UNKNOWN_FMT;
}

int determine_filetype_fname(string fname) {
	FILE *f = fopen(fname.c_str(), "rb");
	char header[4];
	fread(header, 1, 4, f);
	fclose(f);
	return determine_filetype(header);
}

int apply_patch(string file1, string file2, string deltaName) {
	unsigned patchfiletype = determine_filetype_fname(deltaName);
	bool failure;
	if (patchfiletype == BDELTA)
		failure = do_bdelta_patch(
			file1.c_str(),
			file2.c_str(),
			deltaName.c_str()
		);
	else if (patchfiletype == XDELTA) 
		failure = invoke_system("xdelta",
			string("patch --pristine ")+
			deltaName+" "+
			file1+" "+
			file2
		);
	else return 2;
	if (failure) return 1;
	return 0;
}
bool filesMatch(string file1, string file2) {
	return !invoke_system("cmp", file1+" "+file2+ " > /dev/null");
}

bool program_exists(string prog) {
	prog += " 2> /dev/null";
	return !(system(prog.c_str())==0x7F00);
}


