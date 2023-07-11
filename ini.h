/*  ____  ____   ____   Copyright (c) 2023 adasdead
 * |    ||  _ \ |    |  This software is licensed under the MIT License.
 *  |  | |  |  | |  |   Header-only C/C++ INI library
 * |____||__|__||____|  https://github.com/adasdead/ini
*/

#ifndef INI_H_INCLUDED
#define INI_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define INI_MAP_START_CAPACITY              16
#define INI_MAP_LOAD_FACTOR                 0.75
#define INI_DEFAULT_SECTION_NAME            "DEFAULT"
#define INI_COMMENT_SYMBOLS                 ";#"
#define INI_KEY_VALUE_SEPARATORS            "=:"

#define ini_strdup(str)                                                     \
    ((str) ? ini_strndup(str, strlen(str)) : NULL)

#define ini_map_index(hash, capacity)                                       \
    ((hash) & (capacity - 1))

#define ini_map_keys_equal(hash1, key1, hash2, key2)                        \
    ((hash1 == hash2) && (strcmp(key1, key2) == 0))

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

enum ini_io_mode {
    INI_IO_MODE_READ, /* READ ONLY */
    INI_IO_MODE_WRITE /* WRITE ONLY */
};

/**
 * The `ini_t` type is a storage of sections, keys and their values.
 * In fact, it is a hash map in which another hash map is nested.
 * 
 * WARNING: Don't forget to free memory with `ini_free`
 * 
 * Stores INI data in the form of the following table:
 * +---------------------------+---------------+---------------+
 * |          section          |      key      |     value     |
 * +---------------------------+---------------+---------------+
 * |          DEFAULT          |      ...      |      ...      |
 * |            ...            |      ...      |      ...      |
 * +^^^^^^^^^^^^^^^^^^^^^^^^^^^+^^^^^^^^^^^^^^^+^^^^^^^^^^^^^^^+
*/
typedef struct ini_map                     *ini_t;

/**
 * The `ini_map_free_value` type is used to pass a function to free
 * memory for values stored in the `ini_map` structure.
*/
typedef void (*ini_map_free_value)(void *ptr);

struct ini_map_entry {
    unsigned int                            hash;
    char                                   *key;
    void                                   *value;
    struct ini_map_entry                   *next;
};

/**
 * Hash table that stores all key-value pairs.
 * 
 * WARNING: Don't forget to free memory with `ini_map_free`
 * 
 * Based on implementation `libcutils/hashmap.c`
 */
struct ini_map {
    struct ini_map_entry                  **values;
    ini_map_free_value                      free;
    size_t                                  capacity;
    size_t                                  size;
};

/**
 * This is a structure that defines the I/O interface for working
 * with INI.
*/
struct ini_io {
    /* Raw pointer to the data to be processed */
    void                                   *raw;
    /* INI_IO_MODE_READ or INI_IO_MODE_WRITE */
    enum ini_io_mode                        mode;
    /* Last read character. Default `\0` */
    char                                    peek;
    /* Pointer to a function to read one character from `raw` */
    int (*getc)(struct ini_io*);
    /* Pointer to a function to write one character to `raw` */
    void (*putc)(struct ini_io*, int);
    /* Pointer to a function that checks `raw` for the end-of-file */
    bool (*eof)(struct ini_io*);
};

/**
 * A structure for storing the current state of the parser.
*/
struct ini_parse_state {
    struct ini_map                         *cur_section;
    ini_t                                   ini;
};

/**
 * Creates a `str` duplicate and returns a pointer to it. In case of
 * error, NULL is returned. `size` - the size of the string to be
 * duplicated.
 * 
 * WARNING: Memory is allocated for a duplicate of a string. Don't
 * forget to release it.
 * 
 * This function is written because POSIX strndup is not part of the
 * ANSI/ISO standard.
 */
static char *ini_strndup(const char *str, size_t size)
{
    char *tmp = NULL;

    if (str != NULL) {
        tmp = (char*) calloc(size + 1, sizeof *tmp);

        if (tmp != NULL)
            strncpy(tmp, str, size);
    }

    return tmp;
}

/**
 * Returns the current string, but with leading and trailing spaces
 * removed. If a string consists of only spaces, then its string is
 * replaced with zeros.
 */
static char *ini_strtrim(char *str)
{
    size_t last, first;
    size_t size;

    if (str == NULL || *str == '\0')
        return str;
    
    size = strlen(str);

    /* Finding the first non-whitespace character */
    for (first = 0; isspace(str[first]); ++first);
    /* Finding the last non-whitespace character */
    for (last = size - 1; isspace(str[last]) && last > 0; --last);

    if (first > last) {
        memset(str, '\0', size);
        return str;
    }
    
    memmove(str, str + first, last - first + 1);
    str[last - first + 1] = '\0';
    return str;
}

/**
 * Hash function to get the hash of the string `str`, as an
 * unsigned int value, using the djb2 algorithm. Used in hash maps to
 * look up the value of a key.
 * 
 * WARNING: The string must end with `\0`, otherwise it may lead to
 * undefined behavior.
 * 
 * Read more: http://www.cse.yorku.ca/~oz/hash.html
 */
static unsigned int ini_djb2_hash(const char *str)
{
    unsigned char ch;
    unsigned int hash = 5381;

    if (str != NULL) {
        while ((ch = *str++) != '\0') {
            hash = ((hash << 5) + hash) + ch;
        }
    }

    return hash;
}

/**
 * Creates a new hash map and returns NULL on error.
 * 
 * WARNING: If the `free_fn` function pointer is NULL, the hash map
 * values will not be freed and a memory leak may occur.
*/
static struct ini_map *ini_map_new(ini_map_free_value free_fn)
{
    struct ini_map *map = (struct ini_map*) malloc(sizeof *map);

    if (map != NULL) {
        memset(map, 0, sizeof *map);

        map->capacity = INI_MAP_START_CAPACITY;
        map->free = free_fn;
        map->values = (struct ini_map_entry**)
                calloc(map->capacity, sizeof *map->values);

        if (!map->values) {
            free(map);
            return NULL;
        }
    }

    return map;
}

/**
 * Creates a new hash table entry. Returns a pointer to the new
 * element of the hash table, or NULL on error.
 */
static struct ini_map_entry*
ini_map_entry_new(unsigned int hash, const char *key, void *value)
{
    struct ini_map_entry *entry = (struct ini_map_entry*)
        malloc(sizeof *entry);
    
    if (entry != NULL) {
        entry->hash = hash;
        entry->key = ini_strdup(key);
        entry->value = value;
        entry->next = NULL;
    }

    return entry;
}

/**
 * Expands the hash table by doubling its capacity and rehashing all
 * elements. Returns nothing, but may change the hash table pointer
 * if memory is re-allocated.
*/
static void ini_map_expand(struct ini_map *map)
{
    struct ini_map_entry *entry, *next;
    struct ini_map_entry **old_values;
    size_t old_capacity;
    size_t i, index;

    if (map == NULL)
        return;

    if ((double)map->size / map->capacity > INI_MAP_LOAD_FACTOR) {
        old_capacity = map->capacity;
        old_values = map->values;

        map->capacity <<= 1;
        map->values = (struct ini_map_entry**)
            calloc(map->capacity, sizeof *map->values);

        if (map->values == NULL) {
            map->capacity = old_capacity;
            map->values = old_values;
            return;
        }

        for (i = 0; i < old_capacity; ++i) {
            entry = old_values[i];
            
            while (entry != NULL) {
                next = entry->next;
                index = ini_map_index(entry->hash, map->capacity);
                entry->next = map->values[index];
                map->values[index] = entry;
                entry = next;
            }
        }
        
        free(old_values);
    }
}

/**
 * Associates the specified value with the specified key in this map.
 * Does nothing if `map` or `key` is NULL. Returns true if everything
 * went well.
 * 
 * NOTE: Creates a copy of the `key` string inside. If you have
 * allocated memory for `key`, don't forget to free it.
*/
static bool ini_map_put(struct ini_map *map, const char *key, void *value)
{
    unsigned int hash;
    struct ini_map_entry *entry;
    size_t index;

    if (map == NULL && key == NULL && *key == '\0')
        return false;

    hash = ini_djb2_hash(key);
    index = ini_map_index(hash, map->capacity);
    entry = map->values[index];

    while (entry != NULL) {
        if (ini_map_keys_equal(hash, key, entry->hash, entry->key)) {
            if (map->free != NULL)
                map->free(entry->value);

            entry->value = value;
            return true;
        }
        
        entry = entry->next;
    }
    
    entry = ini_map_entry_new(hash, key, value);

    if (entry != NULL) {
        entry->next = map->values[index];
        map->values[index] = entry;
        map->size++;
        ini_map_expand(map);
        return true;
    }

    return false;
}

/**
 * Returns the value associated with the specified key, or NULL if
 * this map does not contain the given key.
*/
static void *ini_map_get(struct ini_map *map, const char *key)
{
    struct ini_map_entry *entry;
    void *value = NULL;
    unsigned int hash;

    if (map != NULL && key != NULL) {
        hash = ini_djb2_hash(key);
        entry = map->values[ini_map_index(hash, map->capacity)];

        while (entry != NULL) {
            if (ini_map_keys_equal(entry->hash, entry->key, hash, key))
                return entry->value;
            else
                entry = entry->next;
        }
    }

    return value;
}

/**
 * Returns an array of pointers to the ini_map_entry elements via the
 * `entries` pointer, stored in the ini_map structure. This function
 * can be used to iterate over all key-value pairs in a hash map.
 * The function returns the number of elements in the `entries` array.
 * If an error occurs, the function returns 0.
 * 
 * WARNING: Memory is allocated for `entries`. Don't forget to free it
 * with `free`
*/
static size_t
ini_map_enumerate(struct ini_map *map, struct ini_map_entry ***entries)
{
    struct ini_map_entry *entry;
    size_t count = 0;
    size_t i;

    if (map == NULL && entries == NULL)
        return 0;

    *entries = (struct ini_map_entry **)
        malloc(map->size * sizeof(struct ini_map_entry*));
    
    if (*entries == NULL)
        return 0;
    
    for (i = 0; i < map->capacity; ++i) {
        entry = map->values[i];

        while (entry != NULL) {
            (*entries)[count++] = entry;
            entry = entry->next;
        }
    }
    
    return count;
}

/**
 * Frees memory for `map`
*/
static void ini_map_free(struct ini_map *map)
{
    struct ini_map_entry **entries, *cur;
    size_t size;
    int i;

    if (map != NULL && map->values) {
        size = ini_map_enumerate(map, &entries);

        for (i = 0; i < size; ++i) {
            cur = entries[i];

            if (cur->value != NULL && map->free != NULL)
                map->free(cur->value);

            free(cur->key);
            free(cur);
        }

        free(entries);
        free(map->values);
        free(map);
    }
}

/**
 * Creates a new ini_t object, which is a data structure for working
 * with INI files.
*/
static ini_t ini_new(void) {
    return ini_map_new((ini_map_free_value) ini_map_free);
}

/**
 * Retrieves a string from the specified section in `ini` by key.
 * 
 * Returns the value of the key `key` in the `section`, otherwise, if,
 * for example, the key does not exist, returns the value `def`.
 * 
 * If `section` is NULL, then the default `INI_DEFAULT_SECTION_NAME`
 * constant will be used.
*/
static const char*
ini_get(ini_t ini, const char *section, const char *key, const char *def)
{
    struct ini_map *_section;
    const char *section_name = section ? section : INI_DEFAULT_SECTION_NAME;
    const char *value = NULL;

    if (ini != NULL && key != NULL && ini->size > 0) {
        _section = (struct ini_map*) ini_map_get(ini, section_name);

        if (_section != NULL)
            value = (const char *) ini_map_get(_section, key);
    }

    return (value != NULL) ? value : def;
}

/**
 * Associates a string with a key in the specified section in `ini`.
 * 
 * If `section` is NULL, then the default `INI_DEFAULT_SECTION_NAME`
 * constant will be used.
 * 
 * NOTE: You can also assign NULL to `value` to make the value not
 * equal to anything.
*/
static void
ini_set(ini_t ini, const char *section, const char *key, const char *value)
{
    struct ini_map *_section;
    const char *section_name = section ? section : INI_DEFAULT_SECTION_NAME;

    if (ini != NULL && key != NULL) {
        _section = (struct ini_map*) ini_map_get(ini, section_name);

        if (_section == NULL) {
            _section = ini_map_new(free);
            ini_map_put(ini, section_name, _section);
        }

        ini_map_put(_section, key, (void*) ini_strdup(value));
    }
}

/**
 * Frees memory for `ini`
*/
static void ini_free(ini_t ini)
{
    if (ini != NULL)
        ini_map_free(ini);
}

/**
 * Checks if the end of a line has been reached in the I/O stream.
*/
static bool ini_io_string_eof(struct ini_io *io)
{
    const char *p = (const char*) io->raw;
    return (bool) ((io != NULL) ? (*p == '\0') : true);
}

/**
 * Returns the next character from the I/O string stream.
*/
static int ini_io_string_getc(struct ini_io *io)
{
    int ch = EOF;
    const char *p = (const char*) io->raw;

    if (io == NULL) return ch;
    if (!io->eof(io)) ch = *p++;
    
    io->raw = (void*) p;
    io->peek = ch;
    return ch;
}

/**
 * Checks if the end of the file has been reached in the I/O stream.
*/
static bool ini_io_file_eof(struct ini_io *io)
{
    return (bool) ((io != NULL) ? feof((FILE*) io->raw) : true);
}

/**
 * Returns the next character from the I/O file stream.
*/
static int ini_io_file_getc(struct ini_io *io)
{
    if (io->eof(io))
        io->peek = EOF;
    else
        io->peek = fgetc((FILE*) io->raw);

    return io->peek;
}

/**
 * Writes the character ch to the I/O file stream.
*/
static void ini_io_file_putc(struct ini_io *io, int ch)
{
    if (io != NULL)
        fputc(ch, (FILE*) io->raw);
}

/**
 * Reads a line from the I/O input stream and returns it. Returns NULL
 * if the end of file has been reached or an error has occurred.
*/
static char *ini_io_read_line(struct ini_io *io)
{
    size_t size = 0;
    size_t capacity = 0;
    char *buffer = NULL;
    char *block;

    if (io != NULL && io->mode == INI_IO_MODE_READ) {
        while (!io->eof(io)) {
            if (io->getc(io) == '\n' || io->peek == EOF)
                break;

            if ((size + 1) >= capacity) {
                capacity += 64;

                block = (char*) realloc(buffer, capacity);

                if (block)
                    buffer = block;
                else
                    break;
            }

            buffer[size++] = io->peek;
            buffer[size] = '\0';
        }
    }

    return buffer;
}

/**
 * Writes the string line to the output stream I/O.
*/
static void ini_io_write(struct ini_io *io, const char *line)
{
    if (line != NULL && io != NULL && io->mode == INI_IO_MODE_WRITE) {
        while (*line != '\0')
            io->putc(io, *line++);
    }
}

/**
 * The ini_parse_line_section function parses the line containing the
 * section name and creates a new section in the ini_t structure.
*/
static void
ini_parse_line_section(struct ini_parse_state *state, const char *line)
{
    struct ini_map *section;
    char *section_name;
    const char *end;

    if (line != NULL && *line == '[') {
        end = strchr(line, ']');

        if (end == NULL)
            return;

        section_name = ini_strndup(line + 1, end - line - 1);
        
        if (section_name == NULL)
            return;

        section = (struct ini_map*)
            ini_map_get(state->ini, section_name);

        if (section == NULL) {
            section = ini_map_new(free);

            if (section == NULL)
                goto clean;
            
            ini_map_put(state->ini, section_name, section);
        }

        state->cur_section = section;

clean:
        free(section_name);
    }
}

/**
 * The ini_parse_line function parses one line from the I/O stream and
 * updates the parse state.
*/
static void ini_parse_line(struct ini_parse_state *state, const char *line)
{
    size_t pos = 0;
    char *value, *unquoted_value;
    char *key;

    if (line != NULL && *line != '\0') {
        pos = strcspn(line, INI_KEY_VALUE_SEPARATORS);

        if (pos != strlen(line)) {
            key = ini_strtrim(ini_strndup(line, pos));
            value = ini_strtrim(ini_strdup(line + pos + 1));
            unquoted_value = ini_strdup(value);

            if (key == NULL || value == NULL || unquoted_value == NULL)
                return;

            if (*unquoted_value != '\0') {
                sscanf(value, "\"%[^\"]\"", unquoted_value);
                ini_map_put(state->cur_section, key, unquoted_value);
            }
            else
                free(unquoted_value);

            free(value);
        }
        else
            ini_parse_line_section(state, line);
    }
}

/**
 * The ini_parse function parses the I/O stream and creates an ini_t
 * structure with configuration data.
 * 
 * The following fields of the I/O structure must not be NULL:
 * - raw 
 * - mode (should be INI_IO_MODE_READ)
 * - getc
 * - eof 
*/
static ini_t ini_parse(struct ini_io *io, struct ini_parse_state *state)
{
    char *line;
    size_t comment_pos;

    if (io == NULL && io->mode == INI_IO_MODE_READ)
        goto ret;

    state->ini = ini_new();
    state->cur_section = ini_map_new(free);

    /* Add a DEFAULT section */
    ini_map_put(state->ini, INI_DEFAULT_SECTION_NAME, state->cur_section);

    while (!io->eof(io)) {
        line = ini_io_read_line(io);

        if (line != NULL && *line != '\0') {
            /* Remove a comment */
            comment_pos = strcspn(line, INI_COMMENT_SYMBOLS);
            line[comment_pos] = '\0';

            /* Trim & parse */
            ini_strtrim(line);
            ini_parse_line(state, line);
        }

        free(line);
    }

ret:
    return state->ini;
}

/**
 * Writes section properties to the I/O stream.
*/
static size_t ini_store_section(struct ini_io *io, struct ini_map *sec)
{
    struct ini_map_entry **entries, *cur;
    size_t size = 0;
    int i;

    if (sec != NULL && io != NULL && io->mode == INI_IO_MODE_WRITE) {
        size = ini_map_enumerate(sec, &entries);

        for (i = 0; i < size; ++i) {
            cur = entries[i];

            if (cur->key != NULL) {
                ini_io_write(io, cur->key);
                io->putc(io, ' ');
                io->putc(io, INI_KEY_VALUE_SEPARATORS[0]);
                io->putc(io, ' ');
                ini_io_write(io, (const char*) cur->value);
                io->putc(io, '\n');
            }
        }

        free(entries);
    }

    return size;
}

/**
 * Saves the ini_t structure to the specified I/O stream.
 * 
 * The following fields of the I/O structure must not be NULL:
 * - raw 
 * - mode (should be INI_IO_MODE_WRITE)
 * - putc 
*/
static void ini_store(ini_t ini, struct ini_io *io)
{
    struct ini_map_entry **entries, *cur;
    struct ini_map *default_section;
    size_t size;
    int i;

    if (ini != NULL && io != NULL && io->mode == INI_IO_MODE_WRITE) {
        size = ini_map_enumerate(ini, &entries);

        default_section = (struct ini_map*)
            ini_map_get(ini, INI_DEFAULT_SECTION_NAME);

        if (default_section != NULL)
            ini_store_section(io, default_section);

        for (i = 0; i < size; ++i) {
            cur = entries[i];

            if (strcmp(cur->key, INI_DEFAULT_SECTION_NAME) == 0)
                continue;

            io->putc(io, '[');
            ini_io_write(io, cur->key);
            ini_io_write(io, "]\n");
            ini_store_section(io, (struct ini_map*) cur->value);
        }

        free(entries);
    }
}

/**
 * Creates an ini structure from data read from a string.
 * 
 * WARNING: The string must end with `\0`, otherwise it may lead to
 * undefined behavior.
*/
static ini_t ini_parse_from_str(const char *str)
{
    struct ini_parse_state state = {0};
    struct ini_io io = {0};

    io.eof = ini_io_string_eof;
    io.getc = ini_io_string_getc;
    io.raw = (void*) str;
    io.mode = INI_IO_MODE_READ;

    return ini_parse(&io, &state);
}

/**
 * Creates an ini structure from data read from an open file stream.
*/
static ini_t ini_parse_from_file(FILE *fp)
{
    struct ini_parse_state state = {0};
    struct ini_io io = {0};

    io.eof = ini_io_file_eof;
    io.getc = ini_io_file_getc;
    io.raw = (void*) fp;
    io.mode = INI_IO_MODE_READ;

    return ini_parse(&io, &state);
}

/**
 * Creates an ini structure from data read from a file at the given
 * path.
*/
static ini_t ini_parse_from_path(const char *path)
{
    ini_t tmp = NULL;
    FILE *fp = fopen(path, "r");

    if (fp != NULL) {
        tmp = ini_parse_from_file(fp);
        fclose(fp);
    }

    return tmp;
}

/**
 * Saves the contents of the ini structure to an open file stream.
*/
static void ini_store_to_file(ini_t ini, FILE *fp)
{
    struct ini_io io = {0};
    io.putc = ini_io_file_putc;
    io.raw = (void*) fp;
    io.mode = INI_IO_MODE_WRITE;
    
    ini_store(ini, &io);
}

/**
 * Saves the contents of the ini structure to a file at the specified
 * path.
*/
static void ini_store_to_path(ini_t ini, const char *path)
{
    FILE *fp;

    if ((fp = fopen(path, "w")) != NULL) {
        ini_store_to_file(ini, fp);
        fclose(fp);
    }
}

#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif /* INI_H_INCLUDED */