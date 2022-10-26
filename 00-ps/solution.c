#include <solution.h>
#include <time.h>

void ps(void)
{
	for (;;) {
		struct timespec infty = {.tv_sec = ~1U, .tv_nsec = 0};
		nanosleep(&infty, NULL);
	}
}
