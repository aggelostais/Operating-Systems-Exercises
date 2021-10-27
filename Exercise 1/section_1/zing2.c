#include <stdio.h>
#include <unistd.h>

void zing(void)
{
	printf("Goodbye %s\n", getlogin());
}

