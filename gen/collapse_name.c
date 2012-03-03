#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "collapse_name.h"

int collapse_name(const char *input, char *_output)
{
	char *output = _output;

	while(1) {
		/* Detect a . or .. component */
		if (input[0] == '.') {
			if (input[1] == '.' && input[2] == '/') {
				/* A .. component */
				if (output == _output)
					return -1;
				input += 2;
				while (*(++input) == '/');
				while(--output != _output && *(output - 1) != '/');
				continue;
			} else if (input[1] == '/') {
				/* A . component */
				input += 1;
				while (*(++input) == '/');
				continue;
			}
		} 

		/* Copy from here up until the first char of the next component */
		while(1) {
			*output++ = *input++;
			if (*input == '/') {
				*output++ = '/';
				while (*(++input) == '/');
				break;
			} else if (*input == 0) {
				*output = 0;
				return 0;
			}
		}
	}
}

/* 
 * Accept a file if:
 * - It begins with input_base
 * - It is a relative path, and input_base + file exists
 * Return a pointer to a name relative to input_base
 */
const char *valid_file(const char *file, const char *base, int len)
{
	struct stat st;
	static char base_buf[PATH_MAX];
	if (!strncmp(file, base, len)) {
		return file + len;
	} else if (file[0] != '/') {
		strncpy(base_buf, base, len);
		strcpy(&base_buf[len], file);
		if (!stat(base_buf, &st))
			return file;
	}
	return NULL;
}

