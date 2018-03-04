#include "window.h"
#include "game.h"

void printBits(size_t const size, void const * const ptr)
{
	unsigned char *b = (unsigned char*) ptr;
	unsigned char byte;
	int i, j;

	for (i=size-1;i>=0;i--)
	{
		for (j=7;j>=0;j--)
		{
			byte = (b[i] >> j) & 1;
			printf("%u", byte);
		}
	}
	puts("");
}

int main(int argc, char *argv[]) {
	run();
	cleanup_window();
	//int mask = 0b1111111111;
	//float f = -0.5f;
	//int i = (int)(f * 511) & mask;
	//printBits(sizeof(i), &i);

	return 0;
}
