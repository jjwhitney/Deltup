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
extern bool verbose;

void error(string message);
int invoke_system(string prog, string args);
void remove_file(string name);
int move_file(string from, string to);
int cat(string infile, string outfile, bool append);
int inflate(string command, string infile, string outfile, bool append);
int deflate(string command, string infile, string outfile, char lev, bool append);
int makeDelta(bool bdelta, string file1, string file2, string deltaName);

int determine_filetype(string s);
int determine_filetype(Injectable_IStream &f);
int determine_filetype_fname(string fname);
int apply_patch(string file1, string file2, string deltaName);
bool filesMatch(string file1, string file2);
bool program_exists(string prog);
