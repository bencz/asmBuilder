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

#define NUMREGS 20
tregisters registers[] =
{
	"al",0x00,regal|reg8		,"ah",0x04,regah|reg8		,"ax",0x00,regax|reg16,
	"cl",0x01,regcl|reg8		,"ch",0x05,regch|reg8		,"cx",0x01,regcx|reg16,
	"dl",0x02,regdl|reg8		,"dh",0x06,regdh|reg8		,"dx",0x02,regdx|reg16,
	"bl",0x03,regbl|reg8		,"bh",0x07,regbh|reg8		,"bx",0x03,regbx|reg16,
	"sp",0x04,regsp|reg8		,"bp",0x05,reg16|regbp		,"si",0x06,reg16|regsi,
	"di",0x07,reg16|regdi		,"es",0x00,reges|seg16		,"ds",0x03,regds|seg16,
	"ss",0x02,regss|seg16		,"cs",0x01,regcs|seg16
};

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