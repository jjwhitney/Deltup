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

const int MAX_BZIP2_COMPRESSORS = 4;
extern char *bzip2_name[MAX_BZIP2_COMPRESSORS];
extern char *bzip2_compressor_name[MAX_BZIP2_COMPRESSORS];
void find_bzip2_compressors();
int find_bz2_compression_level(string file);
