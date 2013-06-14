
#define none	0x00000000

#define reg8	0x00000001
#define reg16	0x00000002
#define seg16	0x00000004

#define reg32	0x00000005
#define immed8	0x00000006
#define immed16	0x00000007
#define pointer 0x00000005 // maybe 0x00000008

#define memref	0x00000008

#define regal	0x00000010
#define regcl	0x00000020
#define regdl	0x00000030
#define regbl	0x00000040
#define regah	0x00000050
#define regch	0x00000060
#define regdh	0x00000070
#define regbh	0x00000080
#define regax	0x00000090
#define regcx	0x000000a0
#define regdx	0x000000b0
#define regbx	0x000000c0
#define regsp	0x000000d0
#define regbp	0x000000e0
#define regsi	0x000000f0
#define regdi	0x00000100
#define reges	0x00000110
#define regds	0x00000120
#define regss	0x00000130
#define regcs	0x00000140

#define regfs	0x00000150
#define reggs	0x00000160
#define regeax	0x00000170
#define regecx	0x00000180
#define regedx	0x00000190
#define regebx	0x00000200
#define regesp	0x00000210
#define regebp	0x00000220
#define regesi	0x00000230
#define regedi	0x00000240

// instructions
#define Inothing		0x0000
#define Ifirstval		0x0002
#define Isecondval		0x0004
#define Iswapval		0x0008
#define Iimmedfirst		0x0010
#define Iimmedsecond	0x0020
#define Icalreg			0x0040
#define Ijump			0x0080
#define Iwrite8			0x0100
#define Ical1			0x0200
#define Ical2			0x0400