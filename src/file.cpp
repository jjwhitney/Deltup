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

#include <zlib.h>
#include <bzlib.h>
#include <string>
using namespace std;
#include <list>
#include <cstdlib>
#include <string.h>
#include "file.h"

GZ_IFStream::GZ_IFStream(string fname) {file = gzopen(fname.c_str(), "rb");}
GZ_IFStream::~GZ_IFStream() {if (!bad()) gzclose(file);}
unsigned GZ_IFStream::read(void *data, unsigned num) {return gzread(file, data, num);}

GZ_OFStream::GZ_OFStream(string fname) {file = gzopen(fname.c_str(), "wb");}
GZ_OFStream::~GZ_OFStream() {gzclose(file);}
unsigned GZ_OFStream::write(const void *data, unsigned num) {return gzwrite(file, (voidp)data, num);}

BZ_IFStream::BZ_IFStream(string fname) {file = BZ2_bzopen(fname.c_str(), "rb");}
BZ_IFStream::~BZ_IFStream() {if (!bad()) BZ2_bzclose(file);}
unsigned BZ_IFStream::read(void *data, unsigned num) {return BZ2_bzread(file, data, num);}

unsigned getLenOfFile(string fname) {
	FILE *f = fopen(fname.c_str(), "rb");
	fseek(f, 0, SEEK_END);
	unsigned len = ftell(f);
	fclose(f);
	return len;
}

bool fileExists(string fname) {
	FILE *f = fopen(fname.c_str(), "rb");
	bool exists = (f!=NULL);
	if (exists) fclose(f);
	return exists;
}

unsigned read_word(IStream &f) {
	unsigned char b, b2;
	f.get(b);
	f.get(b2);
	return (b2<<8)+b;
}
unsigned read_dword(IStream &f) {
	unsigned low = read_word(f);
	return (read_word(f)<<16)+low;
}

void write_word(OStream &f, unsigned number) {
	unsigned char b = number&255;
	f.put(b);
	b = number>>8;
	f.put(b);
}

void write_dword(OStream &f, unsigned number) {
	write_word(f, number&65535);
	write_word(f, number>>16);
}

//returns true if all bytes were successfully copied
bool copy_bytes_to_file(IStream &infile, OStream &outfile, unsigned numleft) {
	unsigned numread;
	do {
		char buf[1024];
		numread = infile.read(buf, numleft>1024?1024:numleft);
		if (outfile.write(buf, numread) != numread) 
			return false;
		// error("Could not write temporary data.  Possibly out of space");
		numleft-=numread;
	} while (numleft && !(numread < 1024 && numleft));
	return (numleft==0);
}

char *read_filename(IStream &f) {
	unsigned fnamelen = read_word(f);
	char *f1 = (char*)malloc(fnamelen+1);
	f.read(f1, fnamelen);
	f1[fnamelen] = 0;
	return f1;
}

void write_filename(OFStream &f, string fname) {
	unsigned lenName = fname.size();
	write_word(f, lenName);
	f.write(fname.c_str(), lenName);
}
