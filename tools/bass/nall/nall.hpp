#pragma once

/* nall
 * author: byuu
 * license: ISC
 *
 * nall is a header library that provides both fundamental and useful classes
 * its goals are portability, consistency, minimalism and reusability
 */

//include the most common nall headers with one statement
//does not include the most obscure components with high cost and low usage

#include "platform.hpp"

#include "algorithm.hpp"
#include "any.hpp"
#include "arithmetic.hpp"
#include "array.hpp"
#include "atoi.hpp"
#include "bit.hpp"
#include "bit-field.hpp"
#include "bit-vector.hpp"
#include "chrono.hpp"
#include "directory.hpp"
#include "dl.hpp"
#include "endian.hpp"
#include "file.hpp"
#include "filemap.hpp"
#include "function.hpp"
#include "hashset.hpp"
#include "hid.hpp"
#include "image.hpp"
#include "inode.hpp"
#include "interpolation.hpp"
#include "intrinsics.hpp"
#include "location.hpp"
#include "map.hpp"
#include "matrix.hpp"
#include "maybe.hpp"
#include "memory.hpp"
#include "path.hpp"
#include "primitives.hpp"
#include "property.hpp"
#include "queue.hpp"
#include "random.hpp"
#include "range.hpp"
#include "run.hpp"
#include "serializer.hpp"
#include "set.hpp"
#include "shared-pointer.hpp"
#include "sort.hpp"
#include "stdint.hpp"
#include "string.hpp"
#include "thread.hpp"
#include "traits.hpp"
#include "unique-pointer.hpp"
#include "utility.hpp"
#include "varint.hpp"
#include "vector.hpp"
#include "decode/base64.hpp"
#include "decode/bmp.hpp"
#include "decode/gzip.hpp"
#include "decode/inflate.hpp"
#include "decode/png.hpp"
#include "decode/url.hpp"
#include "decode/zip.hpp"
#include "encode/base64.hpp"
#include "encode/url.hpp"
#include "hash/crc16.hpp"
#include "hash/crc32.hpp"
#include "hash/crc64.hpp"
#include "hash/sha256.hpp"

#if defined(PLATFORM_WINDOWS)
  #include "windows/registry.hpp"
  #include "windows/utf8.hpp"
#endif

#if defined(API_POSIX)
  #include "serial.hpp"
#endif
