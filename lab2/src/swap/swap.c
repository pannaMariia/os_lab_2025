#include "swap.h"

void Swap(char *left, char *right)
{
	char oldLeft = *left;
	*left = *right;
	*right = oldLeft;

}
