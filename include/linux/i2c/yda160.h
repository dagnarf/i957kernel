/*
 * =====================================================================================
 *
 *       Filename:  yda160.h
 *
 *    Description:  yda160 stereo amp driver
 *
 *        Version:  1.0
 *
 *         Author:  Justin Park
 *
 * =====================================================================================
 */


#ifndef	_YDA160_H_
#define	_YDA160_H_

/********************************************************************************************/
/* 
	Register Name
	bit 31 - 24	: reserved
	bit 23 - 16	: register number 
	bit 15 -  8	: mask bit
	bit  7 -  0	: shift bit
*/

/* #0 */
#define PDPC		0x000100
#define PDP			0x000201

/* #2 */
#define PD_CHP		0x020100
#define PD_REG		0x020402
#define HIZ_HP		0x020803

/* #3 */
#define HIZ_SPR		0x030402
#define HIZ_SPL		0x030803
#define PD_REC		0x031004
#define PD_SNT		0x038007

/* #4 */
#define DPLT		0x047004
#define DALC		0x040700

/* #5 */
#define DREL		0x050C02
#define DATT		0x050300

/* #7 */
#define MNX			0x071F00
#define ZCS_MV		0x078007

/* #8 */
#define SVLA		0x081F00
#define LAT_VA		0x082005
#define ZCS_SVA		0x088007

/* #9 */
#define SVRA		0x091F00

/* #10 */
#define SVLB		0x0A1F00
#define LAT_VB		0x0A2005
#define ZCS_SVB		0x0A8007

/* #11 */
#define SVRB		0x0B1F00

/* #12 */
#define HPL_BMIX	0x0C0100
#define HPL_AMIX	0x0C0201
#define HPL_MMIX	0x0C0402
#define MONO_HP		0x0C0803
#define HPR_BMIX	0x0C1004
#define HPR_AMIX	0x0C2005
#define HPR_MMIX	0x0C4006

/* #13 */
#define SPL_BMIX	0x0D0100
#define SPL_AMIX	0x0D0201
#define SPL_MMIX	0x0D0402
#define MONO_SP		0x0D0803
#define SPR_BMIX	0x0D1004
#define SPR_AMIX	0x0D2005
#define SPR_MMIX	0x0D4006
#define SWAP_SP		0x0D8007

/* #14 */
#define MNA			0x0E1F00
#define SVOL_SP		0x0E4006
#define ZCS_SPA		0x0E8007

/* #15 */
#define SALA		0x0F1F00
#define LAT_HP		0x0F2005
#define SVOL_HP		0x0F4006
#define ZCS_HPA		0x0F8007

/* #16 */
#define SARA		0x101F00

/* #17 */
#define OTP_ERR		0x118006
#define OCP_ERR		0x118007
/********************************************************************************************/

/* return value */
#define D4NP2_SUCCESS			0
#define D4NP2_ERROR				-1

/* D-4NP2 value */
#define MAX_WRITE_REGISTER_NUMBER		19
#define MAX_READ_REGISTER_NUMBER		20

#define D4NP2_SOFTVOLUME_STEPTIME		250		/* soft volume step time (micro second) */ 

/* D-4NP2 state */
#define D4NP2_STATE_POWER_ON	0
#define D4NP2_STATE_POWER_OFF	1

/* type */
#define SINT32 signed long
#define UINT32 unsigned long
#define SINT8 signed char
#define UINT8 unsigned char

/* user setting */
/****************************************************************/
/* #define HP_HIZ_ON */			/* HP Hi-Z is on */
/* #define SP_HIZ_ON */			/* SP Hi-Z is on */
#define SHUNT_SW				/* using shunt switch(PD_SNT) */

/* define parameter */
#define D4NP2_VREFCHARGETIME		6	/* VREF charge time  ( milli second ) */
#define D4NP2_I2CCTRLTIME			23	/* I2C control time  ( micro second ) */
#define D4NP2_CHARGEPUMPWAKEUPTIME	500	/* charge pump time   ( micro second ) */
#define D4NP2_HPIDLINGTIME			8	/* HP idling time time   ( milli second ) */
/****************************************************************/

/* user implementation function */
/****************************************************************/
signed long d4np2Write(unsigned char bWriteRN, unsigned char bWritePrm);								/* write register function */
signed long d4np2WriteN(unsigned char bWriteRN, unsigned char *pbWritePrm, unsigned char dWriteSize);	/* write register function */
signed long d4np2Read(unsigned char bReadRN, unsigned char *pbReadPrm);		/* read register function */
void d4np2Wait(unsigned long dTime);			/* wait function */
void d4np2Sleep(unsigned long dTime);			/* sleep function */
void d4np2ErrHandler(signed long dError);		/* error handler function */
/****************************************************************/

/* structure */
/********************************************************************************************/
/* D-4NP2 setting information */
typedef struct {
	unsigned char bInVolMode;		/* Input volume control mode */
	unsigned char bMinVol;			/* MIN volume */
	unsigned char bLine1Vol;		/* LINE1 volume */
	unsigned char bLine2Vol;		/* LINE2 volume */

	unsigned char bHpCh;			/* HP channel ( stereo/mono ) */
	unsigned char bHpVolMode;		/* HP volume control mode */
	unsigned char bHpMixer_Min;		/* HP Mixer MIN setting */
	unsigned char bHpMixer_Line1;	/* HP Mixer LINE1 setting */
	unsigned char bHpMixer_Line2;	/* HP Mixer LINE2 setting */
	unsigned char bHpVol;			/* HP volume */

	unsigned char bSpCh;			/* SP channel ( stereo/mono ) */
	unsigned char bSpSwap;			/* SP Swap */
	unsigned char bSpVolMode;		/* SP volume control mode */
	unsigned char bSpMixer_Min;		/* SP Mixer MIN setting */
	unsigned char bSpMixer_Line1;	/* SP Mixer LINE1 setting */
	unsigned char bSpMixer_Line2;	/* SP Mixer LINE2 setting */
	unsigned char bSpVol;			/* SP volume */

	unsigned char bSpNonClip;		/* Non Clip / Power Limit setting */
	unsigned char bPowerLimit;		/* Power Limit */
	unsigned char bDistortion;		/* Non Clip Distortion */
	unsigned char bReleaseTime;		/* Non Clip Release Time */
	unsigned char bAttackTime;		/* Non Clip Attack Time */

	unsigned char bRecSW;			/* Receiver Switch */
} D4NP2_SETTING_INFO;
/********************************************************************************************/

/* D-4NP2 Control module API */
/********************************************************************************************/
void D4Np2_PowerOn(D4NP2_SETTING_INFO *pstSettingInfo);	/* power on function */
void D4Np2_PowerOff(void);								/* power off function */
void D4Np2_WriteRegisterBit( unsigned long bName, unsigned char bPrm );		/* 1bit write register function */
void D4Np2_WriteRegisterByte( unsigned char bNumber, unsigned char bPrm );	/* 1byte write register function */
void D4Np2_WriteRegisterByteN( unsigned char bNumber, unsigned char *pbPrm, unsigned char bPrmSize );	/* N byte write register function */
void D4Np2_ReadRegisterBit( unsigned long bName, unsigned char *pbPrm);		/* 1bit read register function */
void D4Np2_ReadRegisterByte( unsigned char bNumber, unsigned char *pbPrm);	/* 1byte read register function */
/********************************************************************************************/

struct snd_set_ampgain {
	int in1_gain;
	int in2_gain;
	int hp_att;
	int hp_gainup;
	int sp_att;
	int sp_gainup;
};

struct yda160_i2c_data {
	struct snd_set_ampgain *ampgain;
	int num_modes;
};

void yda160_speaker_on(void); 					/* speaker path amp on */
void yda160_headset_on(void); 					/* headset path amp on */
void yda160_speaker_headset_on(void); 	/* speaker+headset path amp on */
void yda160_off(void); 										/* amp off */

#endif	/* _YDA160_H_ */
