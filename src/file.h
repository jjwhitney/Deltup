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

class IStream {
public:
	virtual unsigned read(void *data, unsigned num) = 0;
	template<class T>
	int get(T &data) {return read(&data, sizeof(data));}
};

class OStream {
public:
	virtual unsigned write(const void *data, unsigned num) = 0;
	template<class T>
	int put(const T &data) {return write(&data, sizeof(data));}
};

class IFStream : public IStream {
	FILE *file;
public:
	IFStream(string fname) {file = fopen(fname.c_str(), "rb");}
	virtual ~IFStream() {if (!bad()) fclose(file);}
	virtual unsigned read(void *data, unsigned num) {
		return fread(data, 1, num, file);
	}
	bool operator +=(long offset) {
		return fseek(file, offset, SEEK_CUR)==0;
	}
	unsigned loc() {
		return ftell(file);
	}
	bool bad() {return file==NULL;}
};

class OFStream : public OStream {
	void *file;
public:
	OFStream(string fname, bool append = false) 
		{file = fopen(fname.c_str(), append?"ab":"wb");}
	virtual ~OFStream() {fclose((FILE*)file);}
	virtual unsigned write(const void *data, unsigned num) {
		return fwrite(data, 1, num, (FILE*)file);
	}
	bool bad() {return file==NULL;}
};

class GZ_IFStream : public IStream {
	void *file;
public:
	GZ_IFStream(string fname);
	virtual ~GZ_IFStream();
	virtual unsigned read(void *data, unsigned num);
	bool bad() {return (file==NULL);}
};

class GZ_OFStream : public OStream {
	void *file;
public:
	GZ_OFStream(string fname);
	virtual ~GZ_OFStream();
	virtual unsigned write(const void *data, unsigned num);
	bool bad() {return (file==NULL);}
};

class BZ_IFStream : public IStream {
	void *file;
public:
	BZ_IFStream(string fname);
	virtual ~BZ_IFStream();
	virtual unsigned read(void *data, unsigned num);
	bool bad() {return (file==NULL);}
};

class Injectable_IStream : public IStream {
	struct Injected {
		void *buf;
		void *start;
		unsigned num;
		Injected(void *buf, unsigned num) {
			this->buf = start = new char[num];
			memcpy(start, buf, num);
			this->num = num;
		}
	};
	IStream &f;
	list<Injected> l;
public:
	Injectable_IStream(IStream &F) : f(F) {}
	virtual void inject(void *data, int num) {
		l.push_front(Injected(data, num));
	}
	virtual unsigned read(void *vdata, unsigned num) {
		unsigned numread=0;
		char *data = (char*)vdata;
		while (!l.empty() && num >= l.front().num) {
			unsigned numbuf = l.front().num;
			memcpy(data, l.front().buf, numbuf);
			data += numbuf;
			num -= numbuf;
			numread+=numbuf;
			delete (char*)l.front().start;
			l.pop_front();
		}
		if (!l.empty()) {
			memcpy(data, l.front().buf, num);
			l.front().num-=num;
			l.front().buf=(char*)l.front().buf+num;
			numread+=num;
		} else {
			numread+=f.read(data, num);
		}
		return numread;
	}
};


unsigned getLenOfFile(string fname);
bool fileExists(string fname);

unsigned read_word(IStream &f);
unsigned read_dword(IStream &f);

void write_word(OStream &f, unsigned number);
void write_dword(OStream &f, unsigned number);

//returns true if all bytes were successfully copied
bool copy_bytes_to_file(IStream &infile, OStream &outfile, unsigned numleft);

char *read_filename(IStream &f);
void write_filename(OFStream &f, string fname);
