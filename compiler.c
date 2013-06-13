#include <stdio.h>
#include <stdlib.h>
#include "defineList.h"

#define lower(x) (((x)>0x40 && (x)<0x5b)?(x)|0x20:(x))

#define READBUFFERSIZE	512
#define WRITEBUFFERSIZE	512

#ifdef DEBUG
char readbuffer[READBUFFERSIZE];
#endif

int buffpos = 0;                 // position in write buffer         
char wbuffer[WRITEBUFFERSIZE];   // used to buffer the output        
unsigned long org = 0x100;       // com file start offset           
unsigned long offset;            // keep tabs of the offset           
unsigned long filesize;          // keep tabs of the output file size
char errorflag = 0;              // check for errors               
unsigned char memreg2val;        // second register in a memory ref   
unsigned long memimmediate;      // immediate in a mem ref          
unsigned char PointerRef;        // assemble in 32 bit pointer?    
char dirtylabels=0;              // see if labels need fixing     

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

#ifdef CODE32BIT
	,"eax",0x00,regeax|reg32	,"ecx",0x01,regecx|reg32	,"edx",0x02,regedx|reg32
	,"ebx",0x03,regebx|reg32	,"esp",0x04,regesp|reg32	,"ebp",0x05,regebp|reg32
	,"esi",0x06,regesi|reg32	,"edi",0x07,regedi|reg32
#endif
};

typedef struct
{
	char name[6];
	int value;
} tstr_opcodes;

typedef struct
{
	int ocode;
	int length;
	unsigned char code[3];
	unsigned short val1,val2,val3;
	unsigned short instruct;      // instructions for assembling code
} topcodes;

enum eopcodes { 
	opnone		,opaaa		,opaad		,opaam,
	opaas		,opadc		,opadd		,opand,
	opclc		,opcld		,opcli		,opcall,
	opcbw		,opcmc		,opcmp		,opcmpsb,
	opcmpsw		,opcwd		,opdaa		,opdas,
	opdec		,opdiv		,ophlt		,opidiv,
	opimul		,opin		,opinc		,opint,
	opiret		,opja		,opjae		,opjb,
	opjbe		,opjc		,opjcxz		,opje,
	opjg		,opjge		,opjl		,opjle,
	opjmp		,opjmpf		,opjna		,opjnae,
	opjnb		,opjnbe		,opjnc		,opjne,
	opjng		,opjnge		,opjnl		,opjnle,
	opjno		,opjnp		,opjns		,opjnz,
	opjo		,opjp		,opjpe		,opjpo,
	opjs		,opjz		,oplahf		,oplds,
	oplea		,oples		,oplodsb	,oplodsw,
	oploop		,oploope	,oploopne	,oploopnz,
	oploopz		,opmov		,opmovsb	,opmovsw,
	opmul		,opneg		,opnop		,opnot,
	opor		,opout		,oppopf		,oppushf,
	oprcl		,oprcr		,oppop		,oppush,
	oprep		,oprepe		,oprepne	,oprepnz,
	oprepz		,opret		,opretf		,oprol,
	opror		,opsahf		,opsal		,opsar,
	opsbb		,opsub		,opscasb	,opscasw,
	opshl		,opshr		,opstc		,opstd,
	opsti		,opstosb	,opstosw	,optest,
	opwait		,opxchg		,opxlat		,opxor,
	op32bit
};

#define NUMOPCODES  116

tstr_opcodes str_opcodes[NUMOPCODES] =
{
	"aaa",opaaa,"aad",opaad,"aam",opaam,"aas",opaas,
	"adc",opadc,"add",opadd,"and",opand,"call",opcall,
	"cbw",opcbw,"clc",opclc,"cld",opcld,"cli",opcli,"cmc",opcmc,
	"cmp",opcmp,"cmpsb",opcmpsb,"cmpsw",opcmpsw,"cwd",opcwd,
	"daa",opdaa,"das",opdas,"dec",opdec,"div",opdiv,"hlt",ophlt,
	"idiv",opidiv,"imul",opimul,"in",opin,"inc",opinc,"int",opint,
	"iret",opiret,"ja",opja,"jae",opjae,"jb",opjb,"jbe",opjbe,
	"jc",opjc,"jcxz",opjcxz,"je",opje,"jg",opjg,"jge",opjge,
	"jl",opjl,"jle",opjle,"jna",opjna,"jnae",opjnae,"jnb",opjnb,
	"jnbe",opjnbe,"jnc",opjnc,"jne",opjne,"jng",opjng,"jnge",opjnge,
	"jnl",opjnl,"jnle",opjnle,"jno",opjno,"jnp",opjnp,"jns",opjns,
	"jnz",opjnz,"jo",opjo,"jp",opjp,"jpe",opjpe,"jpo",opjpo,
	"js",opjs,"jz",opjz,"jmp",opjmp,"jmpf",opjmpf,"lahf",oplahf,"lds",oplds,
	"lea",oplea,"les",oples,"lodsb",oplodsb,"lodsw",oplodsw,
	"loop",oploop,"loope",oploope,"loopne",oploopne,"loopnz",oploopnz,
	"loopz",oploopz,"mov",opmov,"movsb",opmovsb,"movsw",opmovsw,
	"mul",opmul,"neg",opneg,"nop",opnop,"not",opnot,"or",opor,
	"out",opout,"pop",oppop,"push",oppush,"popf",oppopf,
	"pushf",oppushf,"rcl",oprcl,"rcr",oprcr,"rep",oprep,
	"repe",oprepe,"repne",oprepne,"repnz",oprepnz,"repz",oprepz,
	"ret",opret,"retf",opretf,"rol",oprol,"ror",opror,"sahf",opsahf,
	"sal",opsal,"sar",opsar,"sbb",opsbb,"scasb",opscasb,
	"scasw",opscasw,"shl",opshl,"shr",opshr,"stc",opstc,"std",opstd,
	"sti",opsti,"stosb",opstosb,"stosw",opstosw,"sub",opsub,
	"test",optest,"wait",opwait,"xchg",opxchg,"xlat",opxlat,
	"xor",opxor,
	"32bit",op32bit
};

int stringlen(char *str)
{
	int i=0;
	while(str[i] != 0)
		i++;
	return i;
}

void parse(char *inFile, FILE *fOutPut, int pass)
{
#ifndef DEBUG
	static char readbuffer[READBUFFERSIZE];
#endif

	FILE *fpin;
	char *parser;
	int opcode;
	unsigned long operands[3];
	unsigned long immediates[3];
	int linenum = 0;

	fpin = fopen(inFile, "rt");
	if(fpin == NULL)
	{
		printf("Unable to open file: %s.\n", inFile);
		exit(1);
	}

	while(!feof(fpin))
	{
		fgets(readbuffer, 256, fpin);

		if(feof(fpin) && *(readbuffer+stringlen(readbuffer)-1) == 10)
			continue;

		linenum++;
		parser = readbuffer;

		// continue if not at the end of the line or at a comment :)
		while(*parser != 0 && *parser != ';' && *parser != 0x0d && *parser != 0x0a)
		{
			opcode = opnone;

			// skip whitespace
			while(*parser == 32 || *parser == 9)
				parser++;

			if(*parser != 0 && *parser != ';' && *parser != 0x0d && *parser != 0x0a)
			{
				if(*parser == '.')
				{
				}
			}
		}
	}
}

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
		if(*inFile == '\\')
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