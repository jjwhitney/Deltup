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

//#include <sys/types.h> 
//#include <sys/wait.h>
//#include <unistd.h> 
//#include <sys/signal.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include <stdarg.h>
//#include <ctype.h>
#include <string>
#include <vector>
#include <list>
using namespace std;
#include "file.h"
#include <openssl/md5.h>
#include "bpatch.h"
#include "tmpstore.h"
#include "filetypes.h"
#include "system.h"
#include "bzip2.h"

bool force_overwrite = false, remove_intermediate = false,
	info_mode = false, ensure_md5sum = false, use_bdelta = false;
     
const char magicStr[3] = {'D','T','U'};
unsigned patch_compression_type = 0;
char patch_compression = '0';

vector<string> inputfname;
vector<string> oldrepository;
string newrepository;

const unsigned
	MAKEPATCH	= 1,
	PATCHFILE	= 2;
  
  
void showHelp() {
	printf("deltup version 0.4.4\n");
	printf("Bad usage.  Try one of:\n");
	printf("deltup -m[fvej] [bg #] [-[dD] <repository>] package1 package2 patchfile #make patch\n");
	printf("deltup -p[fvir] [-[dD] <repository>] <patchfiles> #apply patches\n");
	printf("\n");
	printf("Flags: f = force overwrite output files\n");
	printf("       v = verbose\n");
	printf("       e = ensure md5sum is correct (otherwise checks on patch)\n");
	printf("       j = use new bdelta algorithm\n");
	printf("     b,g = compress patch with bzip2 or gzip\n");
	printf("       i = don't actually do anything.  Just show info\n");
	printf("       r = if several patches to one source are given, merge\n");
	printf("           patches so they will apply in one step and don't\n"); 
	printf("           create intermediate files\n"); 
	printf("       d = new tarball belongs in <repository>\n");
	printf("       D = old tarball(s) can be found in <repository>\n");
  
	printf("See README in /usr/share/doc/deltup<ver> for examples and info\n");
	exit(0);
}

bool endsWith(string f, string end) {
	return 
		(f.size() >= end.size()) &&
		f.substr(f.size()-end.size(), end.size()) == end;
}

bool compressionTypeSupported(unsigned c) {
	return (c==UNKNOWN_FMT||c==BZIP2||c==GZIP||c==BZIP2_OLD);
}

unsigned read_ascii_octal(char *s) {
	int num = 0;
	for (char *i = s; i < s+12; ++i) 
		if (*i>='0'&&*i<='7') {
			num<<=3;
			num+=*i-'0';
		}
	return num;
}

bool unzipFileAs(string f, unsigned packageformat, string unzippedfname) {
	switch (packageformat) {
		case UNKNOWN_FMT: return !cat(f, unzippedfname, false);
		case BZIP2: return !inflate("bunzip2", f, unzippedfname, false);
		case GZIP: return !inflate("gunzip", f, unzippedfname, false);
	}
	return false;
}

bool unzipFile(string f, string unzippedfname, unsigned &packageformat) {
	if (endsWith(f, "bz2")) packageformat = BZIP2; else 
	if (endsWith(f, "gz")) packageformat = GZIP; else
	if (endsWith(f, "zip")) error("zip files are unzipported (pardon the pun!)"); else
	if (endsWith(f, "lzw")) error("lzw files are not supported"); else
	if (endsWith(f, ".Z")) error(".Z compression not supported"); else
		packageformat = UNKNOWN_FMT; // assume no compression
	return unzipFileAs(f, packageformat, unzippedfname);
}

void read_md5sum(unsigned char *md, string fname) {
	FILE *f = fopen(fname.c_str(), "rb");
  
	unsigned char buf[4096];
	MD5_CTX c;
	MD5_Init(&c);
	unsigned numread;
	do {
		numread = fread(buf, 1, 4096, f);
		if (numread) MD5_Update(&c, buf, numread);
	} while (numread==4096);
	fclose(f);
	MD5_Final(md, &c);
}

bool md5_equal(unsigned char *md1, string fname) {
	unsigned char md2[16];
	read_md5sum(md2, fname);
	for (int i = 0; i < 16; ++i)
		if (md1[i]!=md2[i]) return false;
	return true;
}

void write_header(OFStream &f) {
	f.write(magicStr, 3);
	write_word(f, 4); //FormatVersion
}

void read_header(IStream &f, unsigned &version, unsigned &numpatches) {
	char magic[3];
	f.read(magic, 3);
	if (strncmp(magic, magicStr, 3) != 0) error("Invalid patch file");
	version = read_word(f);
	if (version > 4) error("Cannot read package format.  Upgrade deltup");
	if (version==2) numpatches = read_word(f); else numpatches = 1;
}

void gzip_without_header(string in, string out, char compression) {
	string tempfile = getTmpFilename();
  
	deflate("gzip", in, tempfile, compression, false);
	// printf("here2 %s %s %c\n", in.c_str(), tempfile.c_str(), compression);
	unsigned filesize = getLenOfFile(tempfile);
	char inbuf[12];
	IFStream *f = new IFStream(tempfile);
	f->read(inbuf, 10);
	char flags = inbuf[3];
  
	if (flags & 2) f->read(inbuf, 2);
	if (flags & 4) {
		unsigned extrafieldsize = read_word(*f);
		while (extrafieldsize) 
			extrafieldsize -= f->read(inbuf, extrafieldsize<10?extrafieldsize:10);
	}
	if (flags & 8) do f->read(inbuf, 1); while (*inbuf);
	if (flags & 16) do f->read(inbuf, 1); while (*inbuf);
	if (flags & 32) f->read(inbuf, 2);
  
	OFStream o(out);
	copy_bytes_to_file(*f, o, filesize-f->loc());
	delete f;
	doneTmpFile(tempfile);
}

//stores known info about file, fields might be unset
struct FileInfo {
	string dir;   //directory where file is located
	string name; //Filename
	string uname; //Inflated filename
	unsigned type;
	unsigned compression; // last four bits are level, rest are version
	string pristineName; //Name of pristine copatch
	bool has_md5;
	unsigned char md5[16];
  
	FileInfo(string name) {this->name = name;}
	string fullname() {return dir+name;}
	bool exists() {return fileExists(fullname());}
	void calc_md5(bool u) {
		if (u) read_md5sum(md5, uname);
		else read_md5sum(md5, fullname());
	}
	bool verify_md5(bool u) {
		return (!has_md5 || md5_equal(md5, u?uname:fullname()));
	}
	bool unpack() {
		uname = getTmpFilename();
		return unzipFile(fullname(), uname, type); 
	}
};

void createDelta(string oldfile, string newfile, string patchfname) {
	if (oldrepository.size()!=1) 
		error("multiple dirs not supported with the -m option");
	if (oldfile.find('/') != string::npos || 
			newfile.find('/') != string::npos)
		printf("Warning: don't prefix packages with a path.  Use the -d and -D options\n");
  
	FileInfo file1(oldfile),
		file2(newfile);
	file1.dir = oldrepository[0];
	file2.dir = newrepository;
  
	if (!force_overwrite && fileExists(patchfname)) error("output file already exists");
	FILE *testaccess = fopen(patchfname.c_str(), "wb");
	if (!testaccess) error("Access denied to output file");
	fclose(testaccess);
	remove_file(patchfname);

	unsigned compression_level, flags = 0;
	if (verbose) printf("Looking at package contents\n");
	string deltaName = getTmpFilename(), pristineName = getTmpFilename();

	if (!file1.exists()) error("cannot access first input file"); 
	if (!file2.exists()) error("cannot access second input file");
	if (!file1.unpack()) error("Cannot read first input file.  Error encountered during decompression");
	if (!file2.unpack()) error("Cannot read second input file.  Error encountered during decompression");

	file1.calc_md5(true);
	file2.calc_md5(false);

	if (verbose) printf("Making package delta\n");

	makeDelta(use_bdelta, file1.uname, file2.uname, deltaName);

	doneTmpFile(file1.uname);
	if (verbose) printf("Ensuring MD5sum will be correct\n");
	if (file2.type==GZIP) {
		flags |= 1;
		string gzip_temp = getTmpFilename();
		const char *lev = "968712534";
		do {
			printf("here %c\n", *lev);
			gzip_without_header(file2.uname, gzip_temp, *lev);
			makeDelta(use_bdelta, gzip_temp, file2.fullname(), pristineName);
			++lev;
		} while (*lev && getLenOfFile(pristineName)>1024);
		if (!*lev) error("Unknown gzip compression format");
		compression_level=*(lev-1)-'0';
		doneTmpFile(gzip_temp);
	} else if (file2.type==BZIP2) {
		compression_level=find_bz2_compression_level(file2.fullname());
		if (ensure_md5sum) {
			find_bzip2_compressors();
			string bzip_temp = getTmpFilename(); // temporary space to compress to
			for (int i = 0; i < MAX_BZIP2_COMPRESSORS; ++i) 
				if (bzip2_name[i] != NULL) { // if there is a compressor to verify

					// printf("compression_level = %x, %x\n", compression_level,
					// compressor_tested_bit(i));
					compression_level |= compressor_tested_bit(i);
					// printf("compression_level = %x\n", compression_level);
					deflate(bzip2_name[i], file2.uname, bzip_temp, compression_level, false);
					if (filesMatch(bzip_temp, file2.fullname())) {
						// printf("%x\n", compression_level);
						compression_level |= compressor_verified_bit(i);
						// printf("%x\n", compression_level);
						if (verbose) 
							printf("found working bzip2 compression format: %s\n",
								bzip2_compressor_name[i]);
					} else if (verbose)
						printf("bzip2 compression format: %s does not work\n",
					bzip2_compressor_name[i]);
				}
			doneTmpFile(bzip_temp);
		}
	} else if (file2.type==UNKNOWN_FMT) {}
	doneTmpFile(file2.uname);

	if (verbose) printf("Output package to: %s\n", patchfname.c_str());
  
	string finalName = getTmpFilename();

	{
		OFStream f(finalName);
		write_header(f);
		write_filename(f, oldfile);
		f.write(file1.md5, 16);
		write_filename(f, newfile);
		f.write(file2.md5, 16);
		write_word(f, file1.type);
		write_word(f, file2.type);
		write_dword(f, compression_level);
		flags|=2; //md5 is after decompression
		write_dword(f, flags);

		unsigned len = getLenOfFile(deltaName);
		write_dword(f, len);
		{
			IFStream deltafile(deltaName);
			copy_bytes_to_file(deltafile, f, len);
		}
		doneTmpFile(deltaName);
		if (flags&1) {
			unsigned len = getLenOfFile(pristineName);
			write_dword(f, len);
			{
				IFStream pristineFile(pristineName);
				copy_bytes_to_file(pristineFile, f, len);
			}
			doneTmpFile(pristineName);
		}
	}
	switch (patch_compression_type) {
		case GZIP: deflate("gzip", finalName, patchfname, patch_compression, false); break;
		case BZIP2: deflate("bzip2", finalName, patchfname, patch_compression, false); break;
		case UNKNOWN_FMT: cat(finalName, patchfname, false); break;
	}
	doneTmpFile(finalName);
	if (fileExists(patchfname)) {
		if (verbose) printf("All done\n");
		printf("Patch is %f times smaller.\n", 
			(double)(getLenOfFile(file2.fullname()))/getLenOfFile(patchfname));
	} else
		fprintf(stderr, "Couldn't output patch\n");
}

void finalize_package(FileInfo &f) {
	string finalName = getTmpFilename();
	char c = '0'+(f.compression&15);
	char *command;
	int i;
	int found_compressor = 0;
	switch (f.type) {
		case BZIP2_OLD: deflate("bzip2_old", f.uname, finalName, c, false); break;
		case BZIP2: 
			find_bzip2_compressors();
			for (i = MAX_BZIP2_COMPRESSORS-1; i >= 0; --i) {
				if ((f.compression & compressor_verified_bit(i)) && 
						(bzip2_name[i]!=NULL)) {
					found_compressor = i;
					if (verbose)
						printf("using working compression format: %s\n",
					bzip2_compressor_name[i]);
					deflate(bzip2_name[i], f.uname, finalName, c, false); 
					break;
				}
			}
			if (!found_compressor) for (i = MAX_BZIP2_COMPRESSORS-1; i >= 0; --i) {
				if ((!(f.compression & compressor_tested_bit(i))) &&
						(bzip2_name[i]!=NULL)) {
					if (verbose) printf("trying to use %s for recompression\n", bzip2_name[i]);
					deflate(bzip2_name[i], f.uname, finalName, c, false); 
					if (!f.has_md5 || md5_equal(f.md5, finalName)) {
						found_compressor = i;
						break;
					}
				}
			}
			if (found_compressor) {
				if (verbose) printf("found compatible bzip2 compressor: %s\n",
					bzip2_name[i]);
			} else fprintf(stderr, "Error: Deltup cannot find the proper bzip2 to rebuild the package\n");
			break;
		case GZIP: gzip_without_header(f.uname, finalName, c); break;
		case UNKNOWN_FMT: cat(f.uname, finalName, false);
	};
	if (f.pristineName!="") {
		int status = apply_patch(finalName,
			f.fullname(),
			f.pristineName);
		switch (status) {
			case 2: fprintf(stderr, "Unknown delta format.  Try upgrading deltup\n"); break;
			case 1: fprintf(stderr, "permission denied on file: %s\n", f.fullname().c_str()); break;
			default: if (!f.verify_md5(false)) fprintf(stderr, "MD5 check failed!!!\n");
		}
	} else {
		move_file(finalName, f.fullname());
		if (fileExists(finalName)) {
			fprintf(stderr, "Access denied\n");
			doneTmpFile(finalName); //rm PKGfinal only
		}
	}
	if (f.pristineName!="") {
		doneTmpFile(finalName);
		// f.pristineName="";
	}
	doneTmpFile(f.uname);
}
      
list<FileInfo*> unfinished;
void patchPackage(IStream &f) {
	unsigned version, numpatches;
	read_header(f, version, numpatches);

	if (version==1) error("Old patch.  Can be read by deltup <= 0.2.1.  Support removed before popuralized.");
	bool check_md5sums = version==4;
	// if (version<3) error("Old patch.  Cannot apply.  Try 0.2 series of deltup");
	for (unsigned patch = 0; patch < numpatches; ++patch) {
		FileInfo *oldpackage,
			*newpackage;
		oldpackage = new FileInfo(read_filename(f));
		oldpackage->has_md5=check_md5sums;
		if (check_md5sums) f.read(oldpackage->md5, 16);
		newpackage = new FileInfo(read_filename(f));
		newpackage->has_md5=check_md5sums;
		if (check_md5sums) f.read(newpackage->md5, 16);

		oldpackage->uname = getTmpFilename();
		newpackage->uname = getTmpFilename();
		oldpackage->type=read_word(f);
		newpackage->type=read_word(f);
		oldpackage->compression=9;
		newpackage->compression=read_dword(f);
		unsigned flags = read_dword(f);
		oldpackage->pristineName="";
		newpackage->pristineName=(flags&1)?getTmpFilename():"";
		string deltaName = getTmpFilename();
		bool success;
		{
			OFStream o(deltaName);
			unsigned sizeofpatch = read_dword(f);
			success = sizeofpatch && copy_bytes_to_file(f, o, sizeofpatch); //read numleft and copy bytes
		}
		if (flags&1) {
			OFStream o(newpackage->pristineName);
			unsigned sizeofpatch = read_dword(f);
			success = success && sizeofpatch && copy_bytes_to_file(f, o, sizeofpatch); //same as above
		}
		if (version<4 && oldpackage->type==GZIP) //3 had a bug with gzip
			newpackage->pristineName="";
    
		list<FileInfo*>::iterator s;
		for (s = unfinished.begin(); 
			s != unfinished.end() && (oldpackage->name == (*s)->name); 
			++s) ;
		bool isContinue = s != unfinished.end();

		printf("%s -> %s: ", oldpackage->name.c_str(), newpackage->name.c_str());
		fflush(stdout);
		if (!success) error("patch is truncated\n");
    
		oldpackage->dir = " ";
		for (unsigned i = 0; i < oldrepository.size(); ++i) {
			if (fileExists(oldrepository[i]+oldpackage->name)) {
				oldpackage->dir = oldrepository[i];
			}
		}

		success=false;
		if (info_mode) 
			printf("\n");
		else if (oldpackage->dir==" " && !isContinue) 
			printf("couldn't find source to patch.\n");
		else if (!force_overwrite && fileExists(newpackage->fullname())) // && !remove_intermediate) {
			printf("patch already applied.\n");
		else if (!compressionTypeSupported(oldpackage->type))
			printf("Can't uncompress old package.  Upgrade deltup.\n");
		else if (!compressionTypeSupported(newpackage->type))
			printf("Can't re-compress package.  Upgrade deltup.\n");
		else success=true;
		if (!success) continue;
    
		if (isContinue) {
			delete oldpackage;
			oldpackage = *s;
		} else {
			if ((!(flags&2) && !oldpackage->verify_md5(false)) ||
					!oldpackage->unpack()) {
				printf("previous package is corrupt\n");
				continue;
			}
			if ((flags&2) && !oldpackage->verify_md5(true)) {
				printf("previous package is corrupt\n");
				continue;
			}
		}
		int status = apply_patch(oldpackage->uname,
			newpackage->uname,
			deltaName);
		switch (status) {
			case 2: fprintf(stderr, "Unknown delta format used.  Try upgrading deltup\n"); continue;
			case 1: printf("Error applying patch\n"); continue;
		}
		doneTmpFile(deltaName);
		doneTmpFile(oldpackage->uname);
		// if (oldpackage->pristineName) doneTmpFile(oldpackage->pristineName);

		if (remove_intermediate) {
			// invoke_system(4, "mv", f2Name, " ", f1Name, " -f");
			FileInfo *f = newpackage;
			if (isContinue) {
				delete *s;
				unfinished.erase(s);
			}
			unfinished.push_back(f);
		} else finalize_package(*newpackage);

		printf("OK\n");
	}
}

void applyPatchfile(string fname);
void applyPatchfile(IStream &f) {
	string fileName = getTmpFilename();
	{
		OFStream o(fileName);
		copy_bytes_to_file(f, o, unsigned(-1));
	}
	applyPatchfile(fileName);
	doneTmpFile(fileName);  //UNTESTED 07/30
}

void applyPatchfile(string fname) {
	IStream *f = new IFStream(fname);
	Injectable_IStream f2(*f);
	if (((IFStream*)f)->bad()) {
		fprintf(stderr, "file is missing: %s\n", fname.c_str()); return;}
	unsigned type = determine_filetype(f2);
	delete f;
	switch (type) {
		case GZIP: f = new GZ_IFStream(fname); break;
		case BZIP2: f = new BZ_IFStream(fname); break;
		case DTU: f = new IFStream(fname); break;
		case UNKNOWN_FMT: fprintf(stderr, "cannot read file %s\n", fname.c_str()); return;
		case TARBALL :
			f = new IFStream(fname);
			unsigned zero_count;
			zero_count = 0;
			char tarheader[512];
			do {
				f->read(tarheader, 512);
				if (tarheader[0]==0) ++zero_count;
				int size = read_ascii_octal(tarheader+124);
				if (size) {
					string fileName = getTmpFilename();
					{
						OFStream o(fileName);
						copy_bytes_to_file(*f, o, size);
					} 
					applyPatchfile(fileName);
					doneTmpFile(fileName);

					unsigned lastblocksize = size % 512;
					if (lastblocksize==0) lastblocksize=512;
					f->read(tarheader, 512 - lastblocksize);
				}
			} while (zero_count < 2);
			return;
	}
	Injectable_IStream infile(*f);
	type = determine_filetype(infile);
	if (type==DTU) patchPackage(infile); else 
		applyPatchfile(infile);
}

int parse_args(int argc, char **argv) {
	unsigned commandtype = 0;
	bool nextNewRepository=false;
	bool nextOldRepositories=false;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0]=='-') {
			for (unsigned j = 1; j < strlen(argv[i]); ++j) {
				unsigned oldcommandtype = commandtype; 
				switch (argv[i][j]) {
					case 'm' : commandtype = MAKEPATCH; break;
					case 'p' : commandtype = PATCHFILE; break;
					case 'i' : info_mode = true; break;
					case 'f' : force_overwrite = true; break;
					case 'v' : verbose = true; break;
					case 'r' : remove_intermediate = true; break;
					case 'd' : nextNewRepository=true; break;
					case 'D' : nextOldRepositories=true; break;
					case 'g' : patch_compression_type=GZIP; break;
					case 'b' : patch_compression_type=BZIP2; break;
					case 'e' : ensure_md5sum=true; break;
					case 'j' : use_bdelta=true; break;
					default : printf("unknown option: %c\n", argv[i][j]); exit(1);
				}
				if (oldcommandtype&&commandtype!=oldcommandtype) 
					error("cannot specify more than one of -mpc options");
			}
		} else if (patch_compression_type && patch_compression == '0') {
			if (strlen(argv[i]) > 1 || argv[i][0] > '9' || argv[i][0] < '1')
				error("compression option must precede a digit from 1-9");
			patch_compression = argv[i][0];
		} else if (nextNewRepository) {
			newrepository = string(argv[i])+"/";
			nextNewRepository=false;
		} else if (nextOldRepositories) {
			char *list = argv[i];
			while (*list) {
				while (isspace(*list)) ++list;
				char *p;
				for (p = list; *p && !isspace(*p); ++p) ;
				int num = p-list;
				if (num) {
					string r(list, num);
					r += "/";
					oldrepository.push_back(r);
					list = p;
				}
				nextOldRepositories=false;
			}
		} else 
			inputfname.push_back(argv[i]); //assume filename
	}
	if (oldrepository.empty()) oldrepository.push_back(newrepository);

	if (nextNewRepository) error("must supply directory after -d option");
	if (nextOldRepositories) error("must supply directory after -D option");
	return commandtype;
}

int main(int argc, char *argv[]) {
	unsigned commandtype = parse_args(argc, argv);
	if (commandtype == MAKEPATCH) {
		if (inputfname.size()!=3) 
			showHelp();
		else
			createDelta(inputfname[0], inputfname[1], inputfname[2]);
	} else if (commandtype == PATCHFILE) {
		for (unsigned filenum = 0; filenum < inputfname.size(); ++filenum)
			applyPatchfile(inputfname[filenum]);
		while (!unfinished.empty()) {
			finalize_package(*unfinished.front());
			unfinished.pop_front();
		}
	} else showHelp();
}
