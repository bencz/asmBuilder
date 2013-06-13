#include <stdio.h>
#include <stdlib.h>
#include "defineList.h"

#define lower(x) (((x)>0x40 && (x)<0x5b)?(x)|0x20:(x))

#define READBUFFERSIZE	512
#define WRITEBUFFERSIZE	512

// table to store labels, names and values
typedef struct sltable
{
	struct sltable *next;
	char *name;
	unsigned long value;
} tltable;

// table to store equate names and values
typedef struct setable
{
	struct setable *next;
	char *name;
	char *value;
	char flag;
} tetable;

tltable *ltable = NULL;
tetable *etable = NULL;

typedef struct
{
	char name[4];
	unsigned char value;
	unsigned long flag;
} tregisters;

/* #define NUMREGS 20
tregisters registers[] =
{
}; */

char outFile[13], ext[]=".com";
int main(int argc, char **argv)
{
	int i;
	FILE *fOutPut;
	char *inFile, *outFileStart;

	inFile = argv[1];
	outFileStart = inFile;

	while(*inFile)
	{
		if(*inFile == "\\")
			outFileStart = inFile+1;
		inFile++;
	}

	inFile = outFile;

	while(*outFileStart != '.' && *outFileStart)
	{
		*inFile = *outFileStart;
		inFile++;
		outFileStart++;
	}
	*inFile = 0;

	if(argc <= 1)
		printf("Modo de uso: asmBuilder <fileName>\n");
	else
	{
		fOutPut = fopen(outFile, "wb");

		for(i=1;i<3;i++)
		{
		}

		fclose(fOutPut);
	}

	return 0;
}