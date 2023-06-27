/*
 * MIT License
 *
 * Copyright (c) 2023 adasdead
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef INI_H_INCLUDED
#define INI_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INI_MAP_START_CAPACITY              16
#define INI_DEFAULT_SECTION_NAME            "DEFAULT"
#define INI_IO_PEEK                         '\0'

#define ini_strdup(str)                                                     \
    ((str) ? ini_strndup(str, strlen(str)) : NULL)

#define ini_map_index(hash, capacity)                                       \
    ((hash) & (capacity - 1))

#define ini_map_keys_equal(hash1, key1, hash2, key2)                        \
    ((hash1 == hash2) && (strcmp(key1, key2) == 0))

#define ini_is_comment_char(ch)     ((ch) == ';' || (ch) == '#')
#define ini_is_delimiter_char(ch)   ((ch) == ':' || (ch) == '=')

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

enum ini_bool {
    ini_false,
    ini_true,
};

enum ini_io_type {
    INI_IO_READ,
    INI_IO_WRITE
};

typedef struct ini_map                     *ini_t;
typedef enum ini_bool                       ini_bool_t;

typedef void (*ini_map_free_value)(void *ptr);

struct ini_map_entry {
    unsigned int                            hash;
    char                                   *key;
    void                                   *value;
    struct ini_map_entry                   *next;
};

/* implementation partly taken from libcutils/hashmap.c */
struct ini_map {
    struct ini_map_entry                  **values;
    ini_map_free_value                      free;
    size_t                                  capacity;
    size_t                                  size;
};

struct ini_io {
    void                                   *raw;
    enum ini_io_type                        type;
    char                                    peek;

    int (*getc)(struct ini_io*);
    void (*putc)(struct ini_io*, int);
    ini_bool_t (*eof)(struct ini_io*);
};

struct ini_parse_state {
    struct ini_map                         *cur_section;
    ini_t                                   ini;
};

static char *ini_strndup(const char *str, size_t size)
{
    char *tmp = NULL;

    if (str != NULL) {
        if (tmp = (char*) calloc(size + 1, sizeof *tmp))
            strncpy(tmp, str, size);
    }

    return tmp;
}

/* djb2 algo: http://www.cse.yorku.ca/~oz/hash.html */
static unsigned int ini_djb2_hash(const char *string)
{
    unsigned char ch;
    unsigned int hash = 5381;

    if (string != NULL) {
        while ((ch = *string++) != '\0') {
            hash = ((hash << 5) + hash) + ch;
        }
    }

    return hash;
}

static struct ini_map *ini_map_new(ini_map_free_value free_fn)
{
    struct ini_map *map = (struct ini_map*) malloc(sizeof *map);

    if (map != NULL) {
        memset(map, 0, sizeof *map);

        map->capacity = INI_MAP_START_CAPACITY;
        map->values = (struct ini_map_entry**)
                calloc(map->capacity, sizeof *map->values);
        map->free = free_fn;

        if (!map->values) {
            free(map);
            return NULL;
        }
    }

    return map;
}

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

static void ini_map_expand(struct ini_map *map)
{
    size_t new_capacity, index;
    struct ini_map_entry **new_values;
    struct ini_map_entry *entry, *next;
    int i;

    if (map != NULL && map->size >= map->capacity) {
        new_capacity = map->capacity << 1;
        new_values = (struct ini_map_entry**)
                calloc(new_capacity, sizeof *new_values);

        if (new_values == NULL)
            return;

        for (i = 0; i < map->capacity; ++i) {
            next = map->values[i];

            while (entry = next) {
                next = entry->next;
                index = ini_map_index(entry->hash, new_capacity);
                entry->next = new_values[index];
                new_values[index] = entry;
            }
        }

        free(map->values);
        map->capacity = new_capacity;
        map->values = new_values;
    }
}

static ini_bool_t
ini_map_put(struct ini_map *map, const char *key, void *value)
{
    struct ini_map_entry **p, *cur;
    unsigned int hash;

    if (map != NULL && key != NULL) {
        hash = ini_djb2_hash(key);

        p = &(map->values[ini_map_index(hash, map->capacity)]);

        while (p != NULL) {
            if ((cur = *p) == NULL) {
                *p = ini_map_entry_new(hash, key, value);
                
                if (*p == NULL)
                    break; /* it's not leak */
                
                map->size++;

                ini_map_expand(map);
                break;
            }

            if (ini_map_keys_equal(hash, key, cur->hash, cur->key)) {
                if (cur->value != NULL)
                    map->free(cur->value);
                
                cur->value = value;
                return ini_true;
            }

            p = &cur->next;
        }
    }

    return ini_false;
}

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

static ini_bool_t ini_map_remove(struct ini_map *map, const char *key)
{
    struct ini_map_entry **p, *cur;
    unsigned int hash;

    if (map != NULL && key != NULL) {
        hash = ini_djb2_hash(key);

        p = &(map->values[ini_map_index(hash, map->capacity)]);

        while (cur = *p) {
            if (ini_map_keys_equal(cur->hash, cur->key, hash, key)) {
                *p = cur->next;
                map->free(cur->value);
                free(cur->key);
                free(cur);
                map->size--;
                return ini_true;
            }

            p = &cur->next;
        }
    }

    return ini_false;
}

static size_t
ini_map_enumerate(struct ini_map *map, struct ini_map_entry ***entries)
{
    struct ini_map_entry **tmp, **block = NULL;
    struct ini_map_entry *entry, *next;
    size_t size = 0;
    int i;

    if (map != NULL && map->values) {
        for (i = 0; i < map->capacity; ++i) {
            next = map->values[i];

            while (entry = next) {
                next = entry->next;
                tmp = (struct ini_map_entry**)
                    realloc(block, ++size * sizeof *tmp);

                if (tmp != NULL) {
                    block = tmp;
                    block[size - 1] = entry;
                }
                else {
                    size--;
                    break;
                }
            }
        }
    }

    *entries = block;
    return size;
}

static void ini_map_free(struct ini_map *map)
{
    struct ini_map_entry **entries, *cur;
    size_t size;
    int i;

    if (map != NULL && map->values) {
        size = ini_map_enumerate(map, &entries);

        for (i = 0; i < size; ++i) {
            cur = entries[i];

            if (cur->value != NULL)
                map->free(cur->value);

            free(cur->key);
            free(cur);
        }

        free(entries);
        free(map->values);
        free(map);
    }
}

static ini_t ini_new(void) {
    return ini_map_new((ini_map_free_value) ini_map_free);
}

static const char*
ini_get(ini_t ini, const char *section, const char *key, const char *def)
{
    struct ini_map *_section;
    const char *value;

    section = (section != NULL) ? section : INI_DEFAULT_SECTION_NAME;

    if (ini != NULL && key != NULL && ini->size > 0) {
        _section = (struct ini_map*) ini_map_get(ini, section);

        if (_section != NULL) {
            value = (const char *) ini_map_get(_section, key);
            return (value != NULL) ? value : def;
        }
    }

    return def;
}

static void
ini_set(ini_t ini, const char *section, const char *key, const char *value)
{
    struct ini_map *_section;

    section = (section != NULL) ? section : INI_DEFAULT_SECTION_NAME;

    if (ini != NULL && key != NULL) {
        _section = (struct ini_map*) ini_map_get(ini, section);

        if (_section == NULL) {
            _section = ini_map_new(free);
            ini_map_put(ini, section, _section);
        }

        ini_map_put(_section, key, (void*) ini_strdup(value));
    }
}

static void ini_free(ini_t ini)
{
    if (ini != NULL)
        ini_map_free(ini);
}

static ini_bool_t ini_io_string_eof(struct ini_io *io)
{
    const char *p = (const char*) io->raw;
    return (ini_bool_t) ((io != NULL) ? (*p == '\0') : ini_true);
}

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

static ini_bool_t ini_io_file_eof(struct ini_io *io)
{
    return (ini_bool_t) ((io != NULL) ? feof((FILE*) io->raw) : ini_true);
}

static int ini_io_file_getc(struct ini_io *io)
{
    if (io->eof(io))
        io->peek = EOF;
    else
        io->peek = fgetc((FILE*) io->raw);

    return io->peek;
}

static void ini_io_file_putc(struct ini_io *io, int ch)
{
    if (io != NULL)
        fputc(ch, (FILE*) io->raw);
}

static char *ini_io_read_line(struct ini_io *io)
{
    size_t size = 1;
    char *buffer = NULL;
    char *block;

    if (io && io->type == INI_IO_READ) {
        while (!io->eof(io)) {
            if (io->getc(io) == '\n' || io->peek == EOF)
                break;

            block = (char*) realloc(buffer, ++size);

            if (block) {
                buffer = block;
                buffer[size - 2] = io->peek;
                buffer[size - 1] = '\0';
            }
            else break;
        }
    }

    return buffer;
}

static void ini_io_write_line(struct ini_io *io, const char *line)
{
    if (line != NULL && io != NULL && io->type == INI_IO_WRITE) {
        while (*line != '\0')
            io->putc(io, *line++);
        
        io->putc(io, '\n');
    }
}

static void ini_line_unquote(char **line)
{
    size_t len;
    char *result, *p = *line;

    if (line == NULL || p == NULL)
        return;

    len = strlen(*line);

    if (len >= 2 && p[0] == '"' && p[len - 1] == '"') {
        result = (char*) malloc(len - 1);

        if (result == NULL)
            return;
        
        strncpy(result, p + 1, len - 2);
        result[len - 2] = '\0';
        free(*line);
        *line = result;
    }
}

static void ini_line_remove_comment(char *line)
{
    if (line == NULL)
        return;

    do {
        if (ini_is_comment_char(*line)) {
            *line = '\0';
            break;
        }

    } while (*line++);
}

static void ini_line_trim(char **line)
{
    size_t len;
    char *p = *line;

    if (line != NULL && p != NULL) {
        len = strlen(p);

        while (isspace(p[len - 1])) --len;
        while (*p && isspace(*p)) ++p, --len;

        p = ini_strndup(p, len);
        free(*line);
        *line = p;
    }
}

static ini_bool_t
ini_parse_line_section(struct ini_parse_state *state, const char *line)
{
    struct ini_map *section;
    char *section_name;

    if (line != NULL && *line == '[') {
        section_name = strdup(line);

        if (section_name != NULL) {
            sscanf(line, "[%[^]]", section_name);

            section = (struct ini_map*)
                ini_map_get(state->ini, section_name);

            if (section == NULL) {
                section = ini_map_new(free);
                ini_map_put(state->ini, section_name, section);
            }

            state->cur_section = section;

            free(section_name);
            return ini_true;
        }
    }

    return ini_false;
}

static void ini_line_split(const char *line, char **key, char **value)
{
    size_t delim_pos = 0;
    ini_bool_t delim_found = ini_false;
    char *_value, *_key;

    if ((*key) == NULL && (*value) == NULL)
        return;

    if (line != NULL) {
        while (*(line + delim_pos) != '\0') {
            if (ini_is_delimiter_char(*(line + delim_pos))) {
                delim_found = ini_true;
                break;
            }

            delim_pos++;
        }

        if (delim_found) {
            _key = ini_strndup(line, delim_pos);
            _value = ini_strdup(line + delim_pos + 1);
            
            ini_line_trim(&_key);
            ini_line_trim(&_value);
            
            *key = _key;

            if (*(*value = _value) == '\0') {
                free(_value);
                *value = NULL;
            }
            else
                sscanf(_value, "\"%[^\"]\"", *value);
        }
        else {
            *key = strdup(line);
            *value = NULL;
        }
    }
}

static ini_t ini_parse(struct ini_io *io)
{
    char *line, *key, *value;
    struct ini_parse_state state = {0};

    if (io == NULL)
        return NULL;

    state.ini = ini_new();
    state.cur_section = ini_map_new(free);

    ini_map_put(state.ini, INI_DEFAULT_SECTION_NAME, state.cur_section);

    while (!io->eof(io)) {
        line = ini_io_read_line(io);

        ini_line_unquote(&line);
        ini_line_remove_comment(line);
        ini_line_trim(&line);

        if (line != NULL) {
            if (ini_parse_line_section(&state, line)) {
                free(line);
                continue;
            }

            ini_line_split(line, &key, &value);

            if (*key)
                ini_map_put(state.cur_section, key, (void*) value);
            else
                free(value);

            free(key);
        }

        free(line);
    }

    return state.ini;
}

static ini_t ini_parse_from_str(const char *str)
{
    struct ini_io io = {0};

    io.eof = ini_io_string_eof;
    io.getc = ini_io_string_getc;
    io.peek = INI_IO_PEEK;
    io.raw = (void*) str;
    io.type = INI_IO_READ;

    return ini_parse(&io);
}

static ini_t ini_parse_from_file(FILE *fp)
{
    struct ini_io io = {0};

    io.eof = ini_io_file_eof;
    io.getc = ini_io_file_getc;
    io.peek = INI_IO_PEEK;
    io.raw = (void*) fp;
    io.type = INI_IO_READ;

    return ini_parse(&io);
}

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

static size_t ini_store_section(struct ini_map *sec, struct ini_io *io)
{
    struct ini_map_entry **entries, *cur;
    size_t size, tmp_size = 0;
    const char *_value;
    char *tmp;
    int i;

    if (sec != NULL && io != NULL && io->type == INI_IO_WRITE) {
        size = ini_map_enumerate(sec, &entries);

        for (i = 0; i < size; ++i) {
            cur = entries[i];

            if (!*cur->key) continue; 

            _value = (const char *) cur->value;
            _value = (_value) ? _value : "";

            tmp_size = strlen(cur->key) + 4 + strlen(_value);
            tmp = ini_strndup("", tmp_size);

            sprintf(tmp, "%s = %s", cur->key, _value);

            ini_io_write_line(io, tmp);
            
            free(tmp);
        }

        free(entries);
    }

    return size;
}

static void ini_store(ini_t ini, struct ini_io *io)
{
    struct ini_map_entry **entries, *cur;
    struct ini_map *_value;
    size_t size;
    char *tmp;
    int i;

    if (ini != NULL && io != NULL && io->type == INI_IO_WRITE) {
        size = ini_map_enumerate(ini, &entries);

        _value = (struct ini_map*) ini_map_get(ini,
                    INI_DEFAULT_SECTION_NAME);

        if (_value != NULL) {
            if (ini_store_section(_value, io))
                io->putc(io, '\n');
        }

        for (i = 0; i < size; ++i) {
            cur = entries[i];

            _value = (struct ini_map*) cur->value;

            if (strcmp(cur->key, INI_DEFAULT_SECTION_NAME) == 0)
                continue;

            tmp = ini_strndup("", strlen(cur->key) + 3);

            sprintf(tmp, "[%s]", cur->key);

            ini_io_write_line(io, tmp);
            ini_store_section(_value, io);

            if (i <= size - 2) io->putc(io, '\n');
            
            free(tmp);
        }

        free(entries);
    }
}

static void ini_store_to_file(ini_t ini, FILE *fp)
{
    struct ini_io io = {0};
    
    io.peek = INI_IO_PEEK;
    io.putc = ini_io_file_putc;
    io.raw = (void*) fp;
    io.type = INI_IO_WRITE;
    
    ini_store(ini, &io);
}

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