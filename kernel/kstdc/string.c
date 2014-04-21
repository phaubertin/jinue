#include <stddef.h>


void *memset(void *s, int c, size_t n) {
	char   *dest;
	size_t	count;
	
	dest  = (char *)s;
	count = n;
	
	while(count > 0) {
		*(dest++) = c;
		--count;
	}
	
	return s;
}
