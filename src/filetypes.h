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

//define recognized filetypes
const unsigned
	UNKNOWN_FMT	= 0, 
	BZIP2		= 1, 
	GZIP		= 2, 
	ZIP			= 3, 
	COMPRESS	= 4,
	TARBALL		= 5,
	DTU			= 6,

	BZIP2_OLD	= 7, // Depricated - now using sub-filetypes
	XDELTA		= 8,
	BDELTA		= 9; 

// define recognized sub-filetypes
 //0 < v. 1.0.0 - openoffice 1.0.2 & 1.0.3 use this
 //1 < v. 1.0.3
 //2 = v. 1.0.3
 //3 = v. 1.0.4
 
// the high flag bits are interleaved between 
// compressor_tested and compressor_verified
inline unsigned compressor_tested_bit(int compress_ver) {
	return ((unsigned)1 << 31 >> (compress_ver*2));
}

inline unsigned compressor_verified_bit(int compress_ver) {
	return ((unsigned)1 << 30 >> (compress_ver*2));
}
