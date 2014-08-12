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

#include <signal.h>
#include <string>
#include <vector>
#include <list>
using namespace std;
#include <cstdio>
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "system.h"
#include <openssl/md5.h>

string tmpdir;
  
list<string> tempfiles;
char tempname[8] = "/000000";
string getTmpFilename() {
	string name = tmpdir+tempname;
	tempfiles.push_back(name);
	int i;
	for (i = 6; tempname[i]=='9'; --i)
		tempname[i]='0';
	tempname[i]++;
	return name;
}

void doneTmpFile(string t) {
	remove_file(t);
	list<string>::iterator i = tempfiles.begin();
	while (i != tempfiles.end()) {
		list<string>::iterator oldi = i;
		++i;
		if (*oldi == t) tempfiles.erase(oldi);
	}
}


void cleanup() {
	if (verbose) printf("cleaning up\n");
	while (!tempfiles.empty()) {
		doneTmpFile(tempfiles.front());
	}
	rmdir(tmpdir.c_str());
}


volatile sig_atomic_t already_running = 0;
     
void term_signal(int sig) {
	if (already_running) return;
	already_running = 1;

	cleanup();

	signal (sig, SIG_DFL);
	raise (sig);
}

class init_TmpStore {
public: 
	init_TmpStore() {
		const char *tmpenv =  getenv("TMPDIR");
		if (!tmpenv) tmpenv = "/tmp";
		char *tmptemplate = new char[strlen(tmpenv)+9];
		strcpy(tmptemplate, tmpenv);
		strcat(tmptemplate, "/.XXXXXX");
		
		tmpdir = mkdtemp(tmptemplate);
		// free(tmptemplate);
		// printf("%s\n", tmpdir);
		atexit(cleanup);
		
		signal(SIGINT, term_signal);
		signal(SIGKILL, term_signal);
		signal(SIGSEGV, term_signal);
		signal(SIGTERM, term_signal);
		signal(SIGQUIT, term_signal);
		// signal(SIGCONT, term_signal);	
	}
} tmpstr;
