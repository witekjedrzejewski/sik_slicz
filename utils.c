#include <stdlib.h>
#include <string.h>

#include "utils.h"

int starts_with(char* text, char* prefix) {
	size_t i;
	size_t text_len = strlen(text);
	size_t prefix_len = strlen(prefix);
	for (i = 0; i < prefix_len; i++) {
		if (i >= text_len || text[i] != prefix[i])
			return 0;
	}
	return 1;
}