#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

#include "classpath.h"

// Use mmap if we can
#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <sys/stat.h>
#define BJVM_USE_MMAP
#endif
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

struct loaded_bytes {
  char *bytes;
  u32 length;
};

static struct loaded_bytes read_file(FILE *f) {
  fseek(f, 0, SEEK_END);
  size_t length = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *data = malloc(length);
  DCHECK(data);
  length = fread(data, 1, length, f);

  DCHECK(length <= UINT32_MAX);

  return (struct loaded_bytes){.bytes = data, .length = (u32) length};
}

#ifdef EMSCRIPTEN
struct loaded_bytes node_read_file(const char *filename) {
  struct loaded_bytes result = {0};
  bool exists = EM_ASM_INT(
      {
        const fs = require('fs');
        return fs.existsSync(UTF8ToString($0));
      },
      filename);
  if (exists) {
    result.bytes = EM_ASM_PTR(
        {
          const fs = require('fs');
          const buffer = fs.readFileSync(UTF8ToString($0));
          const length = buffer.length;

          const result = _malloc(length);
          Module.HEAPU32[$1 >> 2] = length;
          Module.HEAPU8.set(buffer, result);
          return result;
        },
        filename, &result.length);
  }
  return result;
}
#endif

static char *map_jar(const char *filename, bjvm_mapped_jar *jar) {
  char error[256];
#ifdef BJVM_USE_MMAP
  int fd = open(filename, O_RDONLY);
  if (fd == -1)
    goto missing;
  struct stat sb;
  fstat(fd, &sb);
  jar->size_bytes = sb.st_size;
  const char *file = mmap(NULL, jar->size_bytes, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file == MAP_FAILED) {
    close(fd);
    snprintf(error, sizeof(error), "Failed to mmap file %s", filename);
    return strdup(error);
  }
  jar->data = (char *)file;
  jar->is_mmap = true;
  close(fd);
  return nullptr;
#elif EMSCRIPTEN
  int is_node = EM_ASM_INT({ return typeof window === 'undefined'; });
  if (is_node) {
    struct loaded_bytes load = node_read_file(filename);
    if (!load.bytes)
      goto missing;

    jar->data = load.bytes;
    jar->size_bytes = load.length;
    jar->is_mmap = false;
    return nullptr;
  }
#endif
  FILE *f = fopen(filename, "rb");
  if (!f)
    goto missing;
  // Read all into memory
  struct loaded_bytes lb = read_file(f);
  jar->data = lb.bytes;
  jar->size_bytes = lb.length;
  jar->is_mmap = false;
  fclose(f);
  return nullptr;
missing:
  snprintf(error, sizeof(error), "Failed to open file %s", filename);
  return strdup(error);
}

// https://en.wikipedia.org/wiki/ZIP_(file_format)#End_of_central_directory_record_(EOCD)
struct end_of_central_directory_record {
  u32 signature;
  u16 disk_number;
  u16 disk_with_cd;
  u16 num_entries;
  u16 total_entries;
  u32 cd_size;
  u32 cd_offset;
  u16 comment_len;
};

struct central_directory_record {
  u32 header;
  u16 version_made_by;
  u16 version_needed;
  u16 flags;
  u16 compression;
  u16 mod_time;
  u16 mod_date;
  u32 crc32;
  u32 compressed_size;
  u32 uncompressed_size;
  u16 filename_len;
  u16 extra_len;
  u16 comment_len;
  u16 disk_start;
  u16 internal_attr;
  u16 external_attr[2]; // bc padding gets inserted above
  u8 local_header_offset[4];
};

#define CDR_SIZE_BYTES 46
#define CDR_HEADER 0x02014b50

char *parse_central_directory(bjvm_mapped_jar *jar, u64 cd_offset,
                              u32 expected) {
  bjvm_hash_table_reserve(&jar->entries, expected);
  struct central_directory_record cdr = {0};
  char error[256];
  for (u32 i = 0; i < expected; i++) {
    if (cd_offset + sizeof(cdr) > jar->size_bytes) {
      snprintf(error, sizeof(error), "cdr %d out of bounds", i);
      return strdup(error);
    }
    memcpy(&cdr, jar->data + cd_offset, CDR_SIZE_BYTES);
    if (cdr.header != CDR_HEADER)
      return strdup("missing cdr header bytes");
    slice filename = {.chars = jar->data + cd_offset + CDR_SIZE_BYTES,
                          .len = cdr.filename_len};
    u32 header_offset, data_offset;
    memcpy(&header_offset, cdr.local_header_offset, sizeof(data_offset));
    // https://en.wikipedia.org/wiki/ZIP_(file_format)#Local_file_header
    if ((u64)header_offset + 30 + cdr.compressed_size > jar->size_bytes) {
      snprintf(error, sizeof(error), "cdr %d local header out of bounds", i);
      return strdup(error);
    }
    if (cdr.compression != 0 && cdr.compression != 8) {
      snprintf(error, sizeof(error),
               "cdr %d has unsupported compression type %d (supported: 0, 8)",
               i, cdr.compression);
      return strdup(error);
    }
    bool is_compressed = cdr.compression != 0;
    cd_offset +=
        CDR_SIZE_BYTES + cdr.filename_len + cdr.extra_len + cdr.comment_len;

    bjvm_jar_entry *ent = malloc(sizeof(bjvm_jar_entry));
    ent->header = jar->data + header_offset;
    ent->compressed_size = cdr.compressed_size;
    ent->claimed_uncompressed_size = cdr.uncompressed_size;
    ent->is_compressed = is_compressed;

    void *old = bjvm_hash_table_insert(&jar->entries, filename.chars,
                                       filename.len, ent);
    if (old) {
      free(old);
      snprintf(error, sizeof(error), "duplicate filename in JAR: %.*s",
               fmt_slice(filename));
      return strdup(error);
    }
  }
  return nullptr;
}

static void free_jar(bjvm_mapped_jar *jar) {
  bjvm_free_hash_table(jar->entries);
  if (!jar->is_mmap) {
    free(jar->data);
    jar->data = nullptr;
  } else {
#ifdef BJVM_USE_MMAP
    munmap(jar->data, jar->size_bytes);
    jar->is_mmap = false;
#else
    UNREACHABLE();
#endif
  }
  free(jar);
}

static char *load_filesystem_jar(const char *filename, bjvm_mapped_jar *jar) {
  char error[256];
  char *specific_error;
  bool error_needs_free = false;
  char *map_err = map_jar(filename, jar);
  if (map_err) {
    free_jar(jar);
    return map_err;
  }
  // Search 22 bytes from the end for the ZIP end of central directory record
  // signature
  const char sig[4] = "PK\005\006";
  if (jar->size_bytes < 22 ||
      memcmp(jar->data + jar->size_bytes - 22, sig, 4) != 0) {
    specific_error = "Missing end of central directory record";
    goto inval;
  }

  struct end_of_central_directory_record eocdr = {0};
  memcpy(&eocdr, (void *)(jar->data + jar->size_bytes - 22), 22);

  if (eocdr.disk_number != 0 || eocdr.disk_with_cd != 0 ||
      eocdr.num_entries != eocdr.total_entries) {
    specific_error = "Multi-disk JARs not supported";
    goto inval;
  }

  specific_error =
      parse_central_directory(jar, eocdr.cd_offset, eocdr.num_entries);
  error_needs_free = true;
  if (!specific_error) {
    return nullptr; // ok
  }

inval:
  free_jar(jar);
  snprintf(error, sizeof(error), "Invalid JAR file %s%s%s", filename,
           specific_error ? ": " : "", specific_error);
  if (error_needs_free)
    free(specific_error);
  return strdup(error);
}

static char *add_classpath_entry(bjvm_classpath *cp, slice entry) {
  // If entry ends in .jar, load it as a JAR, otherwise treat it as a folder
  if (entry.len >= 4 && memcmp(entry.chars + entry.len - 4, ".jar", 4) == 0) {
    bjvm_mapped_jar *jar = calloc(1, sizeof(bjvm_mapped_jar));
    jar->entries = bjvm_make_hash_table(free, 0.75, 1);

    char *filename = calloc(1, entry.len + 1);
    memcpy(filename, entry.chars, entry.len);
    char *error = load_filesystem_jar(filename, jar);
    free(filename);

    if (error) {
      printf("JAR loading error: %s\n", error);
      return error;
    }

    bjvm_classpath_entry ent = (bjvm_classpath_entry){.name = make_heap_str_from(entry), .jar = jar};
    arrput(cp->entries, ent);

    return nullptr;
  }
  bjvm_classpath_entry ent = (bjvm_classpath_entry){.name = make_heap_str_from(entry), .jar = nullptr};
  arrput(cp->entries, ent);
  return nullptr;
}

char *bjvm_init_classpath(bjvm_classpath *cp, slice path) {
  cp->entries = nullptr;
  cp->as_colon_separated = make_heap_str_from(path);
  int start = 0;
  for (u32 i = 0; i <= path.len; i++) {
    if (i == path.len || path.chars[i] == ':') {
      slice entry = subslice_to(path, start, i);
      if (entry.len > 0) {
        char *err = add_classpath_entry(cp, entry);
        if (err) {
          bjvm_free_classpath(cp);
          return err;
        }
      }
      start = i + 1;
    }
  }
  return nullptr;
}

void bjvm_free_classpath(bjvm_classpath *cp) {
  for (int i = 0; i < arrlen(cp->entries); i++) {
    free_heap_str(cp->entries[i].name);
    if (cp->entries[i].jar) {
      free_jar(cp->entries[i].jar);
    }
  }
  arrfree(cp->entries);
  free_heap_str(cp->as_colon_separated);
  memset(cp, 0, sizeof(*cp));
}

enum jar_lookup_result { NOT_FOUND, FOUND, CORRUPT };

// Returns true if found
enum jar_lookup_result jar_lookup(bjvm_mapped_jar *jar, slice filename,
                                  u8 **bytes, size_t *len) {
  bjvm_jar_entry *jar_entry =
      bjvm_hash_table_lookup(&jar->entries, filename.chars, filename.len);
  if (jar_entry) {
    // Check header at jar_entry->header
    if (memcmp(jar_entry->header, "PK\003\004", 4) != 0) {
      return CORRUPT;
    }

    // Find the compressed data
    u16 filename_len, extra_len;
    memcpy(&filename_len, jar_entry->header + 26, 2);
    memcpy(&extra_len, jar_entry->header + 28, 2);
    u32 offset = 30 + filename_len + extra_len;
    if ((u64)offset + jar_entry->compressed_size +
            (jar_entry->header - jar->data) >
        jar->size_bytes) {
      return CORRUPT;
    }

    char *data = jar_entry->header + offset;
    if (!jar_entry->is_compressed) {
      *bytes = malloc(*len = jar_entry->claimed_uncompressed_size);
      memcpy(*bytes, data, jar_entry->claimed_uncompressed_size);
      return FOUND;
    }

    // Call into zlib
    *bytes = malloc(jar_entry->claimed_uncompressed_size);

    z_stream stream = {0};
    stream.next_in = (unsigned char *)jar_entry->header + offset;
    stream.avail_in = jar_entry->compressed_size;
    stream.next_out = *bytes;
    stream.avail_out = jar_entry->claimed_uncompressed_size;

    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
      return CORRUPT;
    }

    int result = inflate(&stream, Z_FINISH);
    if (result != Z_STREAM_END) {
      inflateEnd(&stream);
      return CORRUPT;
    }

    *len = stream.total_out;
    inflateEnd(&stream);
    return FOUND;
  }
  return NOT_FOUND;
}

static heap_string concat_path(heap_string name, slice filename) {
  bool slash = !name.len || name.chars[name.len - 1] != '/';
  heap_string result = make_heap_str(name.len + slash + filename.len);
  [[maybe_unused]] slice slice =
      bprintf(hslc(result), "%.*s%s%.*s", fmt_slice(name), slash ? "/" : "",
              fmt_slice(filename));
  DCHECK(slice.len == result.len);
  return result;
}

bool bad_filename(const slice filename) {
  for (u32 i = 0; i < filename.len - 1; ++i) {
    if (filename.chars[i] == '.' && filename.chars[i + 1] == '.') {
      return true;
    }
  }
  return false;
}

int bjvm_lookup_classpath(bjvm_classpath *cp, const slice filename,
                          u8 **bytes, size_t *len) {
  *bytes = nullptr;
  *len = 0;
  if (bad_filename(filename)) {
    return -1;
  }
  for (int i = 0; i < arrlen(cp->entries); i++) {
    bjvm_classpath_entry *entry = &cp->entries[i];
    if (entry->jar) {
      enum jar_lookup_result result =
          jar_lookup(entry->jar, filename, bytes, len);
      if (result == NOT_FOUND)
        continue;
      return -(result == CORRUPT);
    }
    // Concatenate with the desired filename (and optionally a / in between)
    heap_string search = concat_path(entry->name, filename);
    DCHECK(search.chars[search.len] == '\0', "Must be null terminated");

    struct loaded_bytes lb;
#ifdef EMSCRIPTEN
    int is_node = EM_ASM_INT({ return ENVIRONMENT_IS_NODE; });
    if (is_node) {
      lb = node_read_file(search.chars);
      free_heap_str(search);
      if (!lb.bytes)
        continue;
    } else
#endif
    {
      FILE *f = fopen(search.chars, "rb");
      free_heap_str(search);
      if (!f)
        continue;
      lb = read_file(f);
      fclose(f);
    }
    *bytes = (u8 *)lb.bytes;
    *len = lb.length;
    return 0;
  }
  return -1;
}