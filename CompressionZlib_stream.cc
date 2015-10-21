/*
 * Ceph - scalable distributed file system
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 */

// -----------------------------------------------------------------------------
#include "common/debug.h"
#include "CompressionZlib.h"
#include "osd/osd_types.h"
// -----------------------------------------------------------------------------

#include <zlib.h>

// -----------------------------------------------------------------------------
#define dout_subsys ceph_subsys_osd
#undef dout_prefix
#define dout_prefix _prefix(_dout)
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

static ostream&
_prefix(std::ostream* _dout)
{
  return *_dout << "CompressionZlib: ";
}
// -----------------------------------------------------------------------------

const char* CompressionZlib::get_method_name()
{
	return "zlib";
}

int CompressionZlib::compress(bufferlist &in, bufferlist &out)
{
	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char* c_in, *c_out;
	int level = 5;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, level);
	if (ret != Z_OK) {
		dout(10) << "Compression init error: init return "
				 << ret << " instead of Z_OK" << dendl;
		return ret;
	}

	long unsigned int max_len = 0;
	for (std::list<buffer::ptr>::const_iterator i = in.buffers().begin();
				i != in.buffers().end(); ++i)
		max_len = (*i).length() > max_len ? (*i).length() : max_len;
	c_out = new unsigned char[max_len];

	for (std::list<buffer::ptr>::const_iterator i = in.buffers().begin();
			i != in.buffers().end();) {

		c_in = (unsigned char*) (*i).c_str();
		long unsigned int len = (*i).length();
		++i;

		strm.avail_in = len;
		flush = i != in.buffers().end() ? Z_FINISH : Z_NO_FLUSH;

		strm.next_in = c_in;

		do {
			strm.avail_out = max_len;
			strm.next_out = c_out;
			ret = deflate(&strm, flush);    /* no bad return value */
			if (ret == Z_STREAM_ERROR) {
				 dout(10) << "Compression error: compress return Z_STREAM_ERROR("
						  << ret << ")" << dendl;
				 deflateEnd(&strm);
				 return 1;
			}
			have = max_len - strm.avail_out;
			out.append((char*)c_out, have);
		} while (strm.avail_out == 0);
		if (strm.avail_in != 0) {
			dout(10) << "Compression error: unused input" << dendl;
			deflateEnd(&strm);
			return 1;
		}
		printf ("\nc_in = %s, cout = %s, ret = %d\n", c_in, c_out, ret);
	}

	deflateEnd(&strm);
	delete [] c_out;
	return 0;
}

int CompressionZlib::decompress(bufferlist &in, bufferlist &out)
{

  int ret;
  unsigned have;
  z_stream strm;
  unsigned char* c_in, *c_out;

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit(&strm);
  if (ret != Z_OK) {
    dout(10) << "Decompression init error: init return "
         << ret << " instead of Z_OK" << dendl;
    return ret;
  }

  long unsigned int max_len = 0;
  for (std::list<buffer::ptr>::const_iterator i = in.buffers().begin();
        i != in.buffers().end(); ++i)
  max_len = (*i).length() > max_len ? (*i).length() : max_len;
  c_out = new unsigned char[max_len];

  for (std::list<buffer::ptr>::const_iterator i = in.buffers().begin();
      i != in.buffers().end(); ++i) {

    c_in = (unsigned char*) (*i).c_str();
    long unsigned int len = (*i).length();

    strm.avail_in = len;
    strm.next_in = c_in;

    do {
      strm.avail_out = max_len;
      strm.next_out = c_out;
      ret = inflate(&strm, Z_NO_FLUSH);
      if (ret != Z_OK && ret != Z_STREAM_END) {
       dout(10) << "Decompression error: decompress return "
            << ret << dendl;
       inflateEnd(&strm);
       return ret;
      }
      have = max_len - strm.avail_out;
      out.append((char*)c_out, have);
      printf ("\nc_in = %s, cout = %s, ret = %d\n", c_in, c_out, strm.avail_out);
    } while (strm.avail_out == 0);

  }

  /* clean up and return */
  (void)inflateEnd(&strm);
  delete [] c_out;
  return 0;
}
