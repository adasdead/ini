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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
namespace ini
{
extern "C" {
#endif /* __cplusplus */

#define INI_DEF					static
#define INI_MAP_START_CAPACITY			16
#define INI_DEF_SECTION_NAME			"DEFAULT"
#define INI_IO_PEEK				'\0'
#define INI_IO_EOF				EOF

#define ini__strdup(str)					\
	((str) ? ini__strndup(str, strlen(str)) : NULL)

#define ini__map_index(hash, capacity)				\
	((hash) & (capacity - 1))

#define ini__map_keys_equal(hash1, key1, hash2, key2)		\
	((hash1 == hash2) && (strcmp(key1, key2) == 0))

#define ini__is_comment(ch)					\
	((ch) == ';' || (ch) == '#')

#define ini__is_delim(ch)					\
	((ch) == ':' || (ch) == '=')

typedef unsigned long				ini__hash_t;
typedef enum ini_bool				ini_bool_t;

typedef struct ini__map				*ini_t;

typedef void (*ini__map_free_value_fn)(void *ptr);

enum ini_bool {
	ini_false, ini_true,
};

enum ini__io_type {
	INI__IO_READ, INI__IO_WRITE
};

struct ini__map_entry {
	ini__hash_t				hash;
	char					*key;
	void					*value;
	struct ini__map_entry			*next;
};

struct ini__map {
	struct ini__map_entry			**values;
	ini__map_free_value_fn			free;
	size_t					capacity;
	size_t					size;
};

struct ini__io {
	void					*raw;
	enum ini__io_type			type;
	char					peek;
	int (*getc)(struct ini__io*);
	void (*putc)(struct ini__io*, int);
	ini_bool_t (*eof)(struct ini__io*);
};

struct ini__parse_state {
	struct ini__map				*cur_section;
	ini_t					ini;
};

INI_DEF char *ini__strndup(const char *str, size_t size)
{
	char *tmp = NULL;

	if (str) {
		if (tmp = (char*) calloc(size + 1, sizeof *tmp)) {
			strncpy(tmp, str, size);
		}
	}

	return tmp;
}

/* djb2 algo: http://www.cse.yorku.ca/~oz/hash.html */
INI_DEF ini__hash_t ini__hash(const char *value)
{
	unsigned char ch;
	unsigned int hash = 5381;

	if (value) {
		while ((ch = *value++) != '\0') {
			hash = ((hash << 5) + hash) + ch;
		}
	}

	return hash;
}

/*				MAP				*/

INI_DEF struct ini__map *ini__map_new(ini__map_free_value_fn free_fn)
{
	struct ini__map *map;

	if (map = (struct ini__map*) malloc(sizeof *map)) {
		memset(map, 0, sizeof *map);

		map->capacity = INI_MAP_START_CAPACITY;
		map->values = calloc(map->capacity, sizeof *map->values);
		map->free = free_fn;

		if (!map->values) {
			free(map);
			return NULL;
		}
	}

	return map;
}

INI_DEF struct ini__map_entry *ini__map_entry_new(ini__hash_t hash,
				const char *key, void *value)
{
	struct ini__map_entry *entry;
	
	if (entry = (struct ini__map_entry*) malloc(sizeof *entry)) {
		entry->hash = hash;
		entry->key = ini__strdup(key);
		entry->value = value;
		entry->next = NULL;
	}

	return entry;
}

INI_DEF void ini__map_expand(struct ini__map *map)
{
	size_t new_capacity, index;
	struct ini__map_entry **new_values;
	struct ini__map_entry *entry, *next;
	int i;

	if (map && map->size >= map->capacity) {
		new_capacity = map->capacity << 1;
		new_values = calloc(new_capacity, sizeof *new_values);

		if (new_values == NULL) return;

		for (i = 0; i < map->capacity; i++) {
			next = map->values[i];

			while (entry = next) {
				next = entry->next;
				index = ini__map_index(entry->hash,
							new_capacity);
				entry->next = new_values[index];
				new_values[index] = entry;
			}
		}

		free(map->values); /* it's not leak */

		map->capacity = new_capacity;
		map->values = new_values;
	}
}

INI_DEF ini_bool_t ini__map_put(struct ini__map *map, const char *key,
						void *value)
{
	struct ini__map_entry **p, *cur;
	ini__hash_t hash;

	if (map && key) {
		hash = ini__hash(key);

		p = &(map->values[ini__map_index(hash, map->capacity)]);

		while (p != NULL) {
			if ((cur = *p) == NULL) {
				*p = ini__map_entry_new(hash, key, value);
				if (*p == NULL) break; /* it's not leak */
				map->size++;

				ini__map_expand(map);
				break;
			}

			if (ini__map_keys_equal(hash, key, cur->hash,
							cur->key)) {
				cur->value = value;
				return ini_true;

			}

			p = &cur->next;
		}

	}

	return ini_false;
}

INI_DEF void *ini__map_get(struct ini__map *map, const char *key)
{
	struct ini__map_entry *entry;
	void *value = NULL;
	ini__hash_t hash;

	if (map && key) {
		hash = ini__hash(key);
		entry = map->values[ini__map_index(hash, map->capacity)];

		while (entry) {
			if (ini__map_keys_equal(entry->hash, entry->key,
							hash, key)) {
				return entry->value;
			}
			else {
				entry = entry->next;
			}
		}
		
	}

	return value;
}

INI_DEF ini_bool_t ini__map_remove(struct ini__map *map, const char *key)
{
	struct ini__map_entry **p, *cur;
	ini__hash_t hash;

	if (map && key) {
		hash = ini__hash(key);

		p = &(map->values[ini__map_index(hash, map->capacity)]);

		while (cur = *p) {
			if (ini__map_keys_equal(cur->hash, cur->key,
							hash, key)) {
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

INI_DEF size_t ini__map_enumerate(struct ini__map *map,
				struct ini__map_entry ***entries)
{
	struct ini__map_entry **tmp, **block = NULL;
	struct ini__map_entry *entry, *next;
	size_t size = 0;
	int i;

	if (map && map->values) {
		for (i = 0; i < map->capacity; i++) {
			next = map->values[i];

			while (entry = next) {
				next = entry->next;
				tmp = realloc(block, ++size * sizeof *tmp);

				if (tmp) {
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

INI_DEF void ini__map_free(struct ini__map *map)
{
	struct ini__map_entry **entries, *cur;
	size_t size;
	int i;

	if (map && map->values) {
		size = ini__map_enumerate(map, &entries);

		for (i = 0; i < size; i++) {
			cur = entries[i];

			if (cur->value) {
				map->free(cur->value);
			}

			free(cur->key);
			free(cur);
		}

		free(entries);
		free(map->values);
		free(map);
	}
}

/*				INI				*/

INI_DEF ini_t ini_new(void) {
	return ini__map_new((ini__map_free_value_fn) ini__map_free);
}

INI_DEF const char *ini_get(ini_t ini, const char *section, const char *key,
						const char *_default)
{
	struct ini__map *_section;
	const char *value;

	section = (section) ? section : INI_DEF_SECTION_NAME;

	if (ini && key && ini->size > 0) {
		_section = (struct ini__map*) ini__map_get(ini, section);

		if (_section) {
			value = (const char *) ini__map_get(_section, key);
			return (value) ? value : _default;
		}
	}

	return _default;
}

INI_DEF void ini_set(ini_t ini, const char *section, const char *key,
						const char *value)
{
	struct ini__map *_section;

	section = (section) ? section : INI_DEF_SECTION_NAME;

	if (ini && key && value) {
		_section = (struct ini__map*) ini__map_get(ini, section);

		if (!_section) {
			_section = ini__map_new(free);
			ini__map_put(ini, section, _section);
		}

		ini__map_put(_section, key, (void*) ini__strdup(value));
	}
}

INI_DEF void ini_free(ini_t ini)
{
	if (ini) {
		ini__map_free(ini);
	}
}

/*				IO				*/

INI_DEF ini_bool_t ini__io_string_eof(struct ini__io *io)
{
	const char *p = (const char*) io->raw;
	return *p == '\0';
}

INI_DEF int ini__io_string_getc(struct ini__io *io)
{
	int ch = INI_IO_EOF;
	const char *p = (const char*) io->raw;

	if (!ini__io_string_eof(io)) ch = *p++;
	
	io->raw = (void*) p;
	io->peek = ch;
	return ch;
}

INI_DEF void ini__io_string(struct ini__io *io, const char *str)
{
	if (io) {
		io->raw = (void*) str;
		io->type = INI__IO_READ;
		io->peek = INI_IO_PEEK;

		io->getc = ini__io_string_getc;
		io->eof = ini__io_string_eof;
	}
}

INI_DEF ini_bool_t ini__io_file_eof(struct ini__io *io)
{
	return feof((FILE*) io->raw);
}

INI_DEF int ini__io_file_getc(struct ini__io *io)
{
	if (ini__io_file_eof(io)) {
		io->peek = INI_IO_EOF;
	}
	else {
		io->peek = fgetc((FILE*) io->raw);
	}

	return io->peek;
}

INI_DEF void ini__io_file_putc(struct ini__io *io, int ch)
{
	fputc(ch, (FILE*) io->raw);
}

INI_DEF void ini__io_file(struct ini__io *io, FILE *fp,
				enum ini__io_type type)
{
	if (io) {
		io->raw = (void*) fp;
		io->type = type;
		io->peek = INI_IO_PEEK;

		if (type == INI__IO_READ) {
			io->getc = ini__io_file_getc;
			io->eof = ini__io_file_eof;
		}
		else {
			io->putc = ini__io_file_putc;
		}
	}
}

INI_DEF char *ini__io_read_line(struct ini__io *io)
{
	size_t size = 1;
	char *buffer = NULL;
	char *block;

	if (io && io->type == INI__IO_READ) {
		while (!io->eof(io)) {
			if (io->getc(io) == '\n' || io->peek == INI_IO_EOF) {
				break;
			}

			block = (char*) realloc(buffer, ++size);

			if (block) {
				buffer = block;

				buffer[size - 2] = io->peek;
				buffer[size - 1] = '\0';
			}
			else {
				break;
			}
		}
		
	}

	return buffer;
}

INI_DEF void ini__io_write_line(struct ini__io *io, const char *line)
{
	if (line && io && io->type == INI__IO_WRITE) {
		while (*line != '\0') {
			io->putc(io, *line++);
		}
		
		io->putc(io, '\n');
	}
}

/*				PARSE				*/

INI_DEF size_t ini__parse_line_in_quotes(const char *line)
{
	size_t index = 0;

	if (line && *line == '"') {
		while (*line++) {
			if (*line == '\0') {
				index = 0;
				break;
			}

			if (*line == '"') break;

			index++;
		}
	}

	return index;
}

INI_DEF void ini__parse_line_remove_comment(char *line)
{
	ini_bool_t is_quoted = ini_false;

	if (!line) return;

	do {
		if (*line == '"') {
			is_quoted = ini__parse_line_in_quotes(line);
			continue;
		}

		if (!is_quoted && ini__is_comment(*line)) {
			*line = '\0';
			break;
		}

	} while (*line++);
}

INI_DEF void ini__parse_line_trim(char **line)
{
	size_t len;
	char *p = *line;

	if (line && p) {
		len = strlen(p);

		while (isspace(p[len - 1])) --len;
		while (*p && isspace(*p)) ++p, --len;

		p = ini__strndup(p, len);
		free(*line);
		*line = p;
	}
}

INI_DEF ini_bool_t ini__parse_line_section(struct ini__parse_state *state,
						const char *line)
{
	struct ini__map *section;
	char *section_name;

	if (line && *line == '[') {
		section_name = strdup(line);

		if (section_name) {
			sscanf(line, "[%[^]]", section_name);

			section = (struct ini__map*) ini__map_get(state->ini,
								section_name);

			if (!section) {
				section = ini__map_new(free);
				ini__map_put(state->ini, section_name,
							section);
			}

			state->cur_section = section;

			free(section_name);
			return ini_true;
		}
	}

	return ini_false;
}

INI_DEF void ini__parse_line_split(const char *line, char **key,
						char **value)
{
	size_t delim_pos = 0;
	ini_bool_t delim_found = ini_false;
	char *_value, *_key;

	if (line) {
		while (*(line + delim_pos) != '\0') {
			if (ini__is_delim(*(line + delim_pos))) {
				delim_found = ini_true;
				break;
			}

			delim_pos++;
		}

		if (delim_found) {
			_key = ini__strndup(line, delim_pos);
			_value = ini__strdup(line + delim_pos + 1);
			
			ini__parse_line_trim(&_key);
			ini__parse_line_trim(&_value);
			
			*key = _key;

			if (*(*value = _value) == '\0') {
				free(_value);
				*value = NULL;
			}
			else {
				sscanf(_value, "\"%[^\"]\"", *value);
			}
		}
		else {
			*key = strdup(line);
			*value = NULL;
		}
	}

}

INI_DEF ini_t ini__parse(struct ini__io *io)
{
	char *line, *key, *value;
	struct ini__parse_state state = {0};

	if (!io) return NULL;

	state.cur_section = ini__map_new(free);
	state.ini = ini_new();

	ini__map_put(state.ini, INI_DEF_SECTION_NAME, state.cur_section);

	while (!io->eof(io)) {
		line = ini__io_read_line(io);
		ini__parse_line_remove_comment(line);
		ini__parse_line_trim(&line);

		if (line) {
			if (ini__parse_line_section(&state, line)) {
				free(line);
				continue;
			}

			ini__parse_line_split(line, &key, &value);

			if (*key) {
				ini__map_put(state.cur_section, key,
							(void*) value);
			}
			else {
				free(value);
			}

			free(key);
		}

		free(line);
	}

	return state.ini;
}

INI_DEF ini_t ini_parse_from_str(const char *str)
{
	struct ini__io io = {0};
	ini__io_string(&io, str);
	return ini__parse(&io);
}

INI_DEF ini_t ini_parse_from_file(FILE *fp)
{
	struct ini__io io = {0};
	ini__io_file(&io, fp, INI__IO_READ);
	return ini__parse(&io);
}

INI_DEF ini_t ini_parse_from_path(const char *path)
{
	FILE *fp;
	ini_t tmp = NULL;

	if ((fp = fopen(path, "r")) != NULL) {
		tmp = ini_parse_from_file(fp);
		fclose(fp);
	}

	return tmp;
}

/*				STORE				*/

INI_DEF size_t ini__store_section(struct ini__map *sec,
					struct ini__io *io)
{
	struct ini__map_entry **entries, *cur;
	size_t size, tmp_size = 0;
	const char *_value;
	char *tmp;
	int i;

	if (sec && io && io->type == INI__IO_WRITE) {
		size = ini__map_enumerate(sec, &entries);

		for (i = 0; i < size; i++) {
			cur = entries[i];

			if (!*cur->key) continue; 

			_value = (const char *) cur->value;
			_value = (_value) ? _value : "";

			tmp_size = strlen(cur->key) + 4 + strlen(_value);
			tmp = ini__strndup("", tmp_size);

			sprintf(tmp, "%s = %s", cur->key, _value);

			ini__io_write_line(io, tmp);
			
			free(tmp);
		}

		free(entries);
	}

	return size;
}

INI_DEF void ini__store(ini_t ini, struct ini__io *io)
{
	struct ini__map_entry **entries, *cur;
	struct ini__map *_value;
	size_t size;
	char *tmp;
	int i;

	if (ini && io && io->type == INI__IO_WRITE) {
		size = ini__map_enumerate(ini, &entries);

		_value = (struct ini__map*) ini__map_get(ini,
					INI_DEF_SECTION_NAME);

		if (_value) {
			if (ini__store_section(_value, io))
				io->putc(io, '\n');
		}

		for (i = 0; i < size; i++) {
			cur = entries[i];

			_value = (struct ini__map*) cur->value;

			if (strcmp(cur->key, INI_DEF_SECTION_NAME) == 0) {
				continue;
			}

			tmp = ini__strndup("", strlen(cur->key) + 3);

			sprintf(tmp, "[%s]", cur->key);

			ini__io_write_line(io, tmp);
			ini__store_section(_value, io);
			io->putc(io, '\n');
			
			free(tmp);
		}

		free(entries);
	}
}

INI_DEF void ini_store_to_file(ini_t ini, FILE *fp)
{
	struct ini__io io = {0};
	ini__io_file(&io, fp, INI__IO_WRITE);
	ini__store(ini, &io);
}

INI_DEF void ini_store_to_path(ini_t ini, const char *path)
{
	FILE *fp;

	if ((fp = fopen(path, "w")) != NULL) {
		ini_store_to_file(ini, fp);
		fclose(fp);
	}
}

#ifdef __cplusplus
}
} /* namespace ini */
#endif /* __cplusplus */

#endif /* INI_H_INCLUDED */