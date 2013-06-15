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

#define NUMINSTRUCTIONS (262+17)
/* machine language values */
topcodes opcodes[NUMINSTRUCTIONS] =
{
	/* opcode operand1,operand2,operand3 */
	/* ascii adjusts */
	opaaa,1,0x37,0x00,0x00,none,none,none,Inothing,
	opaad,2,0xd5,0x0a,0x00,none,none,none,Inothing,
	opaam,2,0xd4,0x0a,0x00,none,none,none,Inothing,
	opaas,1,0x3f,0x00,0x00,none,none,none,Inothing,

	/* add with carry */
	opadc,1,0x14,0x00,0x00,regal,immed8,none,Iimmedsecond,
	opadc,1,0x15,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opadc,1,0x80,0xd0,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opadc,1,0x10,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	opadc,1,0x81,0x06,0x00,reg16,immed16,none,Ifirstval|Ical2|Icalreg|Iimmedsecond,
	opadc,1,0x11,0x40,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,
	opadc,1,0x12,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opadc,1,0x12,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opadc,1,0x13,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opadc,1,0x13,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opadc,1,0x81,0x10,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opadc,1,0x83,0x10,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,

	/* add */
	opadd,1,0x04,0x00,0x00,regal,immed8,none,Iimmedsecond,
	opadd,1,0x05,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opadd,1,0x80,0xc0,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opadd,1,0x00,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	opadd,1,0x81,0xc0,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opadd,1,0x01,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,
	opadd,1,0x02,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opadd,1,0x02,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opadd,1,0x03,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opadd,1,0x03,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opadd,1,0x81,0x00,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opadd,1,0x83,0x00,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,

	/* and */
	opand,1,0x24,0x00,0x00,regal,immed8,none,Iimmedsecond,
	opand,1,0x25,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opand,1,0x80,0xe0,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opand,1,0x20,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	opand,1,0x81,0xe0,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opand,1,0x21,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,
	opand,1,0x22,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opand,1,0x22,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opand,1,0x23,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opand,1,0x23,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opand,1,0x81,0x20,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opand,1,0x83,0x20,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,

	/* call */
	opcall,1,0xe8,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump,
	opcall,1,0xff,0x10,0x00,reg16|memref,none,none,Ifirstval|Icalreg,
	opcall,1,0x9a,0x00,0x00,pointer,none,none,Iimmedfirst,
	/* clear flag bits */
	opcbw,1,0x98,0x00,0x00,none,none,none,Inothing,
	opclc,1,0xf8,0x00,0x00,none,none,none,Inothing,
	opcld,1,0xfc,0x00,0x00,none,none,none,Inothing,
	opcli,1,0xfa,0x00,0x00,none,none,none,Inothing,
	opcmc,1,0xf5,0x00,0x00,none,none,none,Inothing,

	/* compare */
	opcmp,1,0x3c,0x00,0x00,regal,immed8,none,Iimmedsecond,
	opcmp,1,0x3d,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opcmp,1,0x3a,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opcmp,1,0x3b,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opcmp,1,0x3a,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opcmp,1,0x38,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	opcmp,1,0x3b,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opcmp,1,0x39,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,

	opcmp,1,0x80,0xf8,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opcmp,1,0x81,0xf8,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opcmp,1,0x81,0x38,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opcmp,1,0x80,0x38,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,

	opcmpsb,1,0xa6,0x00,0x00,none,none,none,Inothing,
	opcmpsw,1,0xa7,0x00,0x00,none,none,none,Inothing,
	opcwd,1,0x99,0x00,0x00,none,none,none,Inothing,
	opdaa,1,0x27,0x00,0x00,none,none,none,Inothing,
	opdas,1,0x2f,0x00,0x00,none,none,none,Inothing,

	/* decrease */
	opdec,1,0xfe,0xc8,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opdec,0,0x48,0x00,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	opdec,0,0x48,0x00,0x00,reg16|memref,none,none,Ifirstval|Ical1|Icalreg,
	/* divide */
	opdiv,1,0xf6,0xf0,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opdiv,1,0xf7,0xf0,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	/* halt */
	ophlt,1,0xf4,0x00,0x00,none,none,none,Inothing,
	/* integer divide */
	opidiv,1,0xf6,0xf8,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opidiv,1,0xf7,0xf8,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	/* integer multiply */
	opimul,1,0xf6,0xe8,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opimul,1,0xf7,0xe8,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	/* in */

	/* this could be a slight flaw */
	opin,1,0xe4,0x00,0x00,regal,immed8,none,Iimmedsecond, 
	opin,1,0xec,0x00,0x00,regal,regdx,none,Inothing,
	opin,1,0xe5,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opin,1,0xed,0x00,0x00,regax,regdx,none,Inothing,
	/* inc */
	opinc,1,0xfe,0xc0,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opinc,0,0x40,0x00,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	opinc,0,0x40,0x00,0x00,reg16|memref,none,none,Ifirstval|Icalreg,
	/* issue interrupt */
	opint,1,0xcd,0x00,0x00,immed16,none,none,Iimmedfirst|Iwrite8,
	/* interrupt return */
	opiret,1,0xcf,0x00,0x00,none,none,none,Inothing,
	/* basic jump */
	opjmp,1,0xe9,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump,
	opjmp,1,0xea,0x00,0x00,pointer,none,none,Iimmedfirst,
	opjmpf,1,0xff,0x2e,0x00,reg16|memref,none,none,Ifirstval|Icalreg,
	/* comparitive jumps */
	opja, 1,0x77,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjae,1,0x73,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjb, 1,0x72,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjbe,1,0x76,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjc, 1,0x72,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjcxz,1,0xe3,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opje, 1,0x74,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjg, 1,0x7f,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjge,1,0x7d,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjl, 1,0x7c,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjle,1,0x7e,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjna,1,0x76,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnae,1,0x72,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnb,1,0x73,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnbe,1,0x77,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnc,1,0x73,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjne,1,0x75,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjng,1,0x7e,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnge,1,0x7c,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnl,1,0x7d,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnle,1,0x7f,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjno,1,0x71,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnp,1,0x7b,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjns,1,0x79,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjnz,1,0x75,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjo, 1,0x70,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjp, 1,0x7a,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjpe,1,0x7a,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjpo,1,0x7b,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjs, 1,0x78,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	opjz, 1,0x74,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	/* lahf */
	oplahf,1,0x9f,0x00,0x00,none,none,none,Inothing,
	/* lds */
	oplds,1,0xc5,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	/*les*/
	oplds,1,0x8d,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	/* les */
	oples,1,0xc4,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	/* loads */
	oplodsb,1,0xac,0x00,0x00,none,none,none,Inothing,
	oplodsw,1,0xad,0x00,0x00,none,none,none,Inothing,
	/* loops */
	oploop,1,0xe2,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	oploope,1,0xe1,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	oploopne,1,0xe0,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	oploopnz,1,0xe0,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	oploopz,1,0xe1,0x00,0x00,immed16,none,none,Iimmedfirst|Ijump|Iwrite8,
	/* from reg to mem */
	/* 8 bit two byte operands should be rewritten */
	opmov,1,0x88,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,


	/* 16 bit */
	opmov,1,0x89,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,

	/* 8 and 16 bit register moves */
	opmov,1,0x8a,0x00,0x00,reg8 ,reg8,none,Ifirstval|Isecondval|Icalreg,
	opmov,1,0x8b,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opmov,1,0x8e,0x00,0x00,seg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opmov,1,0x8c,0x00,0x00,reg16,seg16,none,Ifirstval|Isecondval|Icalreg|Iswapval,
	/* from mem to reg */
	/* 8bit */
	opmov,1,0x8a,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	/* 16 bit */
	opmov,1,0x8b,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	/* 8 bit immediate moves */
	opmov,0,0xb0,0x00,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	/* 16 bit immediate moves */
	opmov,0,0xb8,0x00,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opmov,1,0xc7,0x00,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opmov,1,0xc6,0x00,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,

	/* string moves */
	opmovsb,1,0xa4,0x00,0x00,none,none,none,Inothing,
	opmovsw,1,0xa5,0x00,0x00,none,none,none,Inothing,
	/* multiply */
	opmul,1,0xf6,0xe0,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opmul,1,0xf7,0xe0,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	/* negative */
	opneg,1,0xf6,0xd8,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opneg,1,0xf7,0xd8,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	/* no operation */
	opnop,1,0x90,0x00,0x00,none,none,none,Inothing,
	/* not */
	opnot,1,0xf6,0xd0,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opnot,1,0xf7,0xd0,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,

	/* or */
	opor,1,0x0c,0x00,0x00,regal,immed8,none,Iimmedsecond,
	opor,1,0x0d,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opor,1,0x80,0xc8,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opor,1,0x08,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	opor,1,0x81,0xc8,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opor,1,0x09,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,
	opor,1,0x0a,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opor,1,0x0a,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opor,1,0x0b,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opor,1,0x0b,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opor,1,0x81,0x08,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opor,1,0x80,0x08,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,
	/* out */
	opout,1,0xe6,0x00,0x00,immed16,regal,none,Iimmedsecond|Iwrite8,
	opout,1,0xee,0x00,0x00,regdx,regal,none,Inothing,
	opout,1,0xe7,0x00,0x00,immed16,regax,none,Iimmedsecond,
	opout,1,0xef,0x00,0x00,regdx,regax,none,Inothing,
	/* pop */
	oppop,0,0x07,0x00,0x00,seg16,none,none,Ifirstval|Ical2|Icalreg,
	oppop,0,0x58,0x00,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	oppop,1,0x8f,0x00,0x00,reg16|memref,none,none,Ifirstval|Icalreg,
	oppopf,1,0x9d,0x00,0x00,none,none,none,Inothing,
	/* push */
	oppush,0,0x06,0x00,0x00,seg16,none,none,Ifirstval|Ical2|Icalreg,
	oppush,0,0x50,0x00,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	oppush,1,0xff,0x30,0x00,reg16|memref,none,none,Ifirstval|Icalreg,
	oppushf,1,0x9c,0x00,0x00,none,none,none,Inothing,
	/* rotate left through carry */
	oprcl,1,0xd0,0xd0,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	oprcl,1,0xd2,0xd0,0x00,reg8,regcl,none,Ifirstval|Ical1|Icalreg,
	oprcl,1,0xd1,0xd0,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	oprcl,1,0xd3,0xd0,0x00,reg16,regcl,none,Ifirstval|Ical1|Icalreg,
	/* rotate right through carry */
	oprcr,1,0xd0,0xd8,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	oprcr,1,0xd2,0xd8,0x00,reg8,regcl,none,Ifirstval|Ical1|Icalreg,
	oprcr,1,0xd1,0xd8,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	oprcr,1,0xd3,0xd8,0x00,reg16,regcl,none,Ifirstval|Ical1|Icalreg,
	/* repeat */
	oprep,1,0xf3,0x00,0x00,none,none,none,Inothing,
	oprepe,1,0xf3,0x00,0x00,none,none,none,Inothing,
	oprepne,1,0xf2,0x00,0x00,none,none,none,Inothing,
	oprepnz,1,0xf2,0x00,0x00,none,none,none,Inothing,
	oprepz,1,0xf3,0x00,0x00,none,none,none,Inothing,
	/* return */
	opretf,1,0xcb,0x00,0x00,none,none,none,Inothing,
	opret,1,0xc2,0x00,0x00,immed16,none,none,Iimmedfirst,
	opret,1,0xc3,0x00,0x00,none,none,none,Inothing,
	/* rotate left */
	oprol,1,0xd0,0xc0,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	oprol,1,0xd2,0xc0,0x00,reg8,regcl,none,Ifirstval|Ical1|Icalreg,
	oprol,1,0xd1,0xc0,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	oprol,1,0xd3,0xc0,0x00,reg16,regcl,none,Ifirstval|Ical1|Icalreg,
	/* rotate right */
	opror,1,0xd0,0xc8,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opror,1,0xd2,0xc8,0x00,reg8,regcl,none,Ifirstval|Ical1|Icalreg,
	opror,1,0xd1,0xc8,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	opror,1,0xd3,0xc8,0x00,reg16,regcl,none,Ifirstval|Ical1|Icalreg,
	/* store flags in ah */
	opsahf,1,0x9e,0x00,0x00,none,none,none,Inothing,
	/* arithmetic shift left */
	opsal,1,0xd0,0xe0,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opsal,1,0xd2,0xe0,0x00,reg8,regcl,none,Ifirstval|Ical1|Icalreg,
	opsal,1,0xd1,0xe0,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	opsal,1,0xd3,0xe0,0x00,reg16,regcl,none,Ifirstval|Ical1|Icalreg,
	/* arithmetic shift right */
	opsar,1,0xd0,0xf8,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opsar,1,0xd2,0xf8,0x00,reg8,regcl,none,Ifirstval|Ical1|Icalreg,
	opsar,1,0xd1,0xf8,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	opsar,1,0xd3,0xf8,0x00,reg16,regcl,none,Ifirstval|Ical1|Icalreg,
	/* sbb */
	opsbb,1,0x1c,0x00,0x00,regal,immed8,none,Iimmedsecond,
	opsbb,1,0x1d,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opsbb,1,0x80,0xd8,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opsbb,1,0x18,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	opsbb,1,0x81,0xd8,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opsbb,1,0x19,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,
	opsbb,1,0x1a,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opsbb,1,0x1a,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opsbb,1,0x1b,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opsbb,1,0x1b,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opsbb,1,0x81,0x18,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opsbb,1,0x80,0x18,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,
	/* compares */
	opscasb,1,0xae,0x00,0x00,none,none,none,Inothing,
	opscasw,1,0xaf,0x00,0x00,none,none,none,Inothing,
	/* shift left */
	opshl,1,0xc0,0xe0,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opshl,1,0xc1,0xe0,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond|Iwrite8,
	opshl,1,0xd2,0xe0,0x00,reg8,regcl,none,Ifirstval|Ical1|Icalreg,
	opshl,1,0xd0,0xe0,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opshl,1,0xd3,0xe0,0x00,reg16,regcl,none,Ifirstval|Ical1|Icalreg,
	opshl,1,0xd1,0xe0,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,
	/* shift right */
	/* I know that these are 286 instructions, but I had to have them */
	opshr,1,0xc0,0xe8,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opshr,1,0xc1,0xe8,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond|Iwrite8,
	opshr,1,0xd2,0xe8,0x00,reg8,regcl,none,Ifirstval|Ical1|Icalreg,
	opshr,1,0xd0,0xe8,0x00,reg8,none,none,Ifirstval|Ical1|Icalreg,
	opshr,1,0xd3,0xe8,0x00,reg16,regcl,none,Ifirstval|Ical1|Icalreg,
	opshr,1,0xd1,0xe8,0x00,reg16,none,none,Ifirstval|Ical1|Icalreg,

	/* set flags */
	opstc,1,0xf9,0x00,0x00,none,none,none,Inothing,
	opstd,1,0xfd,0x00,0x00,none,none,none,Inothing,
	opsti,1,0xfb,0x00,0x00,none,none,none,Inothing,
	/* store */
	opstosb,1,0xaa,0x00,0x00,none,none,none,Inothing,
	opstosw,1,0xab,0x00,0x00,none,none,none,Inothing,
	/* sub */
	opsub,1,0x2c,0x00,0x00,regal,immed8,none,Iimmedsecond,
	opsub,1,0x2d,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opsub,1,0x80,0xe8,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opsub,1,0x28,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	opsub,1,0x81,0xe8,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opsub,1,0x29,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,
	opsub,1,0x2a,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opsub,1,0x2a,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opsub,1,0x2b,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opsub,1,0x2b,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opsub,1,0x81,0x28,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opsub,1,0x80,0x28,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,
	/* test */
	optest,1,0xa8,0x00,0x00,regal,immed8,none,Iimmedsecond,
	optest,1,0xa9,0x00,0x00,regax,immed16,none,Iimmedsecond,
	optest,1,0xf6,0xc0,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	optest,1,0x84,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	optest,1,0xf7,0xc0,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	optest,1,0x85,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg|Iimmedfirst,
	optest,1,0x84,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	optest,1,0x84,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	optest,1,0x85,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	optest,1,0x85,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	optest,1,0xf7,0x00,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	/* wait */
	opwait,1,0x9b,0x00,0x00,none,none,none,Inothing,
	/* exchange */
	opxchg,0,0x90,0x00,0x00,regax,reg16,none,Ifirstval|Ical1|Icalreg,
	opxchg,1,0x86,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opxchg,1,0x87,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	/* translate */
	opxlat,1,0xd7,0x00,0x00,none,none,none,Inothing,
	/* xor */
	opxor,1,0x34,0x00,0x00,regal,immed8,none,Iimmedsecond,
	opxor,1,0x35,0x00,0x00,regax,immed16,none,Iimmedsecond,
	opxor,1,0x80,0xe8,0x00,reg8,immed8,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opxor,1,0x30,0x00,0x00,reg16|memref,reg8,none,Ifirstval|Isecondval|Icalreg,
	opxor,1,0x81,0xe8,0x00,reg16,immed16,none,Ifirstval|Ical1|Icalreg|Iimmedsecond,
	opxor,1,0x31,0x00,0x00,reg16|memref,reg16,none,Ifirstval|Isecondval|Icalreg,
	opxor,1,0x32,0x00,0x00,reg8,reg8,none,Ifirstval|Isecondval|Icalreg,
	opxor,1,0x32,0x00,0x00,reg8,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opxor,1,0x33,0x00,0x00,reg16,reg16,none,Ifirstval|Isecondval|Icalreg,
	opxor,1,0x33,0x00,0x00,reg16,reg16|memref,none,Ifirstval|Isecondval|Icalreg,
	opxor,1,0x81,0x30,0x00,reg16|memref,immed16,none,Ifirstval|Icalreg|Iimmedsecond,
	opxor,1,0x80,0x30,0x00,reg16|memref,immed8,none,Ifirstval|Icalreg|Iimmedsecond,

	op32bit,1,0x66,0x00,0x00,none,none,none,Inothing
};


int stringlen(char *str)
{
	int i=0;
	while(str[i] != 0)
		i++;
	return i;
}

// check to see if sub-string is in the string at the current dest
char matches(char *source, char *dest)
{
	int i,j=0;
	if (source == NULL)
		printf("1\n");
	for (i=0;source[i] != 32 && source[i] != 9 && source[i] != ']'&&
		source[i] != '+' && source[i] != '-' && source[i] != 0x0d
		&& source[i] != 0x0a && source[i] != 0 && source[i] != ','
		&& source[i] != ':' && source[i] != '('&& source[i] != '['&&
		source[i] != '*' && source[i] != '/' && source[i] != ';'
		;i++)
		if (lower(source[i]) != lower(dest[i]))
			return 0;
	while (dest[j]!=0 && dest[j]!=']')
		j++;
	if (i<j)
		return 0;
	else
		return 1;
}

void parse(char * infile,FILE * fpout,int pass)
{
#ifndef DEBUG
	static char readbuffer[READBUFFERSIZE];
#endif
	FILE * fpin;
	char * parser;
	int opcode;
	unsigned long operands[3];
	unsigned long immediates[3];
	int linenum = 0;

	fpin=fopen(infile,"rt");
	if (fpin == NULL)
	{
		printf("Unable to open file %s.\n",infile);
		exit(1);
	}
	while (!feof(fpin))
	{
		fgets(readbuffer,256,fpin);
		if (feof(fpin) && *(readbuffer+stringlen(readbuffer)-1) == 10)
			continue;
		linenum ++;
		parser = readbuffer;
		// keep going if not at the end of the line or at a comment
		while (*parser != 0 && *parser != ';' && *parser != 0x0d
			&& *parser != 0x0a)
		{
			opcode = opnone;
			// skip past the white space
			while (*parser == 32 || *parser == 9) parser++;
			// check to see if at the end of the line or at a comment
			if (*parser != 0 && *parser != ';' && *parser != 0x0d
				&& *parser != 0x0a)
			{
				// now check to see if it's an opcode, a label or data
				// check to see if preproc
				if (*parser == '.')
				{
					char fname[80];
					int i;
					parser++;
					if (matches(parser,"include"))
					{ /* have included another file */
						while (*parser != 32 && *parser != 9) parser++;
						while (*parser == 32 || *parser == 9) parser++;
						if (*parser != '"')
						{
							printf("** include definition with no open quotes ** ");
							printf("line %d, file %s\n",linenum,infile);
							printf(readbuffer);
							exit(1);
						}
						parser++; // get rid of `"'
						i = 0;
						while (*parser != '"')
						{
							if (*parser == 32 || *parser == 0 || *parser == 0x0a)
							{
								printf("** include definition with no closing quotes ** ");
								printf("line %d, file %s\n",linenum,infile);
								printf(readbuffer);
								exit(1);
							}
							if (i>=79)
							{
								printf("** filename to long ** ");
								printf("line %d, file %s\n",linenum,infile);
								printf(readbuffer);
								exit(1);
							}
							fname[i]=*parser;
							parser++;
							i++;
						}
						fname[i]=0;
						parse(fname,fpout,pass);
						// force to continue to next line
						opcode = 0;
						*parser=0;
						continue;
					}
					else
						// origin has changed
						if (matches(parser,"org"))
						{
							while (*parser != 32 && *parser != 9) parser++;
							while (*parser == 32 || *parser == 9) parser++;
							filesize = (offset-org)+filesize;
							offset = getval(&parser,2);
							org = offset;
						}
						if (matches(parser,"echo"))
						{
							while (*parser != 32 && *parser != 9) parser++;
							parser++;
							if (pass==2)
								printf("%s",parser);
							*parser=0;
							continue;
						}
				}
				else
				{
					// now continue on with the rest of the program 
					if (*parser == '@') // it's a label
					{
						if (pass == 1)
							putlabel(parser);
						else
							if (checklabel(&parser))
								printf("** label redundency check failed ** line %d, file %s\n",
								linenum,infile);
						if (errorflag)
						{ // an error occured
							printf("line %d, file %s\n",linenum,infile);
							printf(readbuffer);
							exit(1);
						}
						// now skip past the label
						while(*parser != ':' && *parser != ';' && *parser !=0)
							parser++;
						if (*parser != ':')
						{
							printf("** Incomplete label ** line %d, file %s\n",
								linenum,infile);
							printf(readbuffer);
							exit(1);
						}
						parser++;
					}
				}
			}
		}
	}
	fclose(fpin);
}

char outfile[13],ext[]=".com";
int main(int argc,char * argv[])
{
	int i;
	FILE *fpout;
	char *infile , *outfilestart;

	infile = argv[1];
	outfilestart = infile;

	while (*infile)
	{
		if (*infile == '\\')
			outfilestart = infile+1;
		infile++;
	}
	infile = outfile;

	while (*outfilestart != '.' && *outfilestart)
	{
		*infile = *outfilestart;
		infile++;
		outfilestart++;
	}
	outfilestart = ext;
	while (*outfilestart)
	{
		*infile = *outfilestart;
		infile++;
		outfilestart++;
	}
	*infile = 0;

	if (argc<=1)
		printf("Usage: compiler <filename>\n");
	else
	{
		fpout=fopen(outfile,"wb");
		for (i=1;i<3;i++)
		{
		}
		fclose(fpout);
	}

	return 0;
}