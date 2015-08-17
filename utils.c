#include <math.h>
#include <stdio.h>

#include <ccore/file.h>

#include "utils.h"

int loadTextFile(const char *file, char **result)
{
	FILE *fp;
	ccFileInfo fi;

	fi = ccFileInfoGet(file);
	if(fi.size <= 0){
		return -1;
	}
	*result = (char*)malloc(fi.size);

	fp = fopen(file, "rb");
	fread(*result, 1, fi.size, fp);
	fclose(fp);

	(*result)[fi.size - 1] = '\0';

	return fi.size;
}

double rootMeanSquare(const short *buf, int len)
{
	long int sum = 0;
	int i;
	for(i = 0; i < len; i++){
		sum += buf[i] * buf[i];
	}

	return sqrt(sum / len);
}
