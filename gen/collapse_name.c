int collapse_name(char *name)
{
	char *input = name;
	char *output = name;

	while(1) {
		/* Detect a . or .. component */
		if (input[0] == '.') {
			if (input[1] == '.' && input[2] == '/') {
				/* A .. component */
				if (output == name)
					return -1;
				input += 3;
				while(--output != name && *(output - 1) != '/');
			} else if (input[1] == '/') {
				/* A . component */
				input += 2;
			}
		} 

		/* Copy from here up until the first char of the next component */
		while(1) {
			*output++ = *input++;
			if (*input == '/') {
				*output++ = *input++;
				break;
			} else if (*input == 0) {
				*output = 0;
				return 0;
			}
		}
	}

	return 0;
}
