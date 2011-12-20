/*
 * =====================================================================================
 *
 *       Filename:  yda160.c
 *
 *    Description:  yda160 stereo amp driver
 *
 *        Version:  1.0
 *
 *         Author:  Justin Park 
 *
 * =====================================================================================
 */


#include <linux/i2c/yda160.h>
#include <linux/i2c.h>
#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <mach/vreg.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#define MODULE_NAME "yda160"
#define AMPREG_DEBUG 0
#define MODE_NUM_MAX 30

static struct yda160_i2c_data g_data;


static struct i2c_client *pclient;
static struct snd_set_ampgain g_ampgain[MODE_NUM_MAX];
static struct snd_set_ampgain temp;
static int set_mode = 0;
static int cur_mode = 0;

static ssize_t mode_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", set_mode);
}
static ssize_t mode_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg < MODE_NUM_MAX ) )
	{
		set_mode = reg;	
	}
	return count;
}

static ssize_t in1_gain_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.in1_gain);
}
static ssize_t in1_gain_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 7 ) )
	{
		temp.in1_gain = reg;
	}
	return count;
}
static ssize_t in2_gain_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.in2_gain);
}
static ssize_t in2_gain_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 7 ) )
	{
		temp.in2_gain = reg;
	}
	return count;
}
static ssize_t hp_att_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.hp_att);
}
static ssize_t hp_att_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 31 ) )
	{
		temp.hp_att = reg;
	}
	return count;
}
static ssize_t hp_gainup_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.hp_gainup);
}
static ssize_t hp_gainup_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 3 ) )
	{
		temp.hp_gainup = reg;
	}
	return count;
}
static ssize_t sp_att_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.sp_att);
}
static ssize_t sp_att_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 31 ) )
	{
		temp.sp_att = reg;
	}
	return count;
}
static ssize_t sp_gainup_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.sp_gainup);
}
static ssize_t sp_gainup_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 2 ) )
	{
		temp.sp_gainup = reg;
	}
	return count;
}
static ssize_t gain_all_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d,%d,%d,%d,%d,%d,%d\n",
											  set_mode,
											  temp.in1_gain ,
                                              temp.in2_gain ,
                                              temp.hp_att   , 
                                              temp.hp_gainup,
                                              temp.sp_att   , 
                                              temp.sp_gainup);
}
static ssize_t save_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if(reg == 1)
	{
		memcpy(&g_ampgain[set_mode], &temp, sizeof(struct snd_set_ampgain));
	}
		
	return count;
}

static DEVICE_ATTR(mode		, S_IRUGO|S_IWUGO, mode_show, 		mode_store);
static DEVICE_ATTR(in1_gain	, S_IRUGO|S_IWUGO, in1_gain_show, 	in1_gain_store);
static DEVICE_ATTR(in2_gain	, S_IRUGO|S_IWUGO, in2_gain_show, 	in2_gain_store);
static DEVICE_ATTR(hp_att	, S_IRUGO|S_IWUGO, hp_att_show, 	hp_att_store);
static DEVICE_ATTR(hp_gainup, S_IRUGO|S_IWUGO, hp_gainup_show, 	hp_gainup_store);
static DEVICE_ATTR(sp_att	, S_IRUGO|S_IWUGO, sp_att_show, 	sp_att_store);
static DEVICE_ATTR(sp_gainup, S_IRUGO|S_IWUGO, sp_gainup_show, 	sp_gainup_store);
static DEVICE_ATTR(gain_all	, S_IRUGO, 				   gain_all_show, 	NULL);
static DEVICE_ATTR(save		, S_IWUGO, 				   NULL, 			save_store);

static struct attribute *yda160_attributes[] = {
    &dev_attr_mode.attr,
    &dev_attr_in1_gain.attr,
    &dev_attr_in2_gain.attr,
    &dev_attr_hp_att.attr,
    &dev_attr_hp_gainup.attr,
    &dev_attr_sp_att.attr,
    &dev_attr_sp_gainup.attr,
    &dev_attr_gain_all.attr,
    &dev_attr_save.attr,
    NULL
};

static struct attribute_group yda160_attribute_group = {
    .attrs = yda160_attributes
};

static int load_ampgain(void)
{
	struct yda160_i2c_data *yd ;
	int numofmodes;
	int index=0;
	yd = &g_data;
	numofmodes = yd->num_modes;
	for(index=0; index<numofmodes; index++)
	{
		memcpy(&g_ampgain[index], &yd->ampgain[index], sizeof(struct snd_set_ampgain));
#if AMPREG_DEBUG
		pr_info(MODULE_NAME ":[%d].in1_gain = %d \n",index,g_ampgain[index].in1_gain  );
		pr_info(MODULE_NAME ":[%d].in2_gain = %d \n",index,g_ampgain[index].in2_gain  );
		pr_info(MODULE_NAME ":[%d].hp_att   = %d \n",index,g_ampgain[index].hp_att    );
		pr_info(MODULE_NAME ":[%d].hp_gainup= %d \n",index,g_ampgain[index].hp_gainup );
		pr_info(MODULE_NAME ":[%d].sp_att   = %d \n",index,g_ampgain[index].sp_att    );
		pr_info(MODULE_NAME ":[%d].sp_gainup= %d \n",index,g_ampgain[index].sp_gainup );
#endif
	}
	memcpy(&temp, &g_ampgain[0], sizeof(struct snd_set_ampgain));
#if AMPREG_DEBUG
	pr_info(MODULE_NAME ":temp.in1_gain = %d \n",temp.in1_gain  );
	pr_info(MODULE_NAME ":temp.in2_gain = %d \n",temp.in2_gain  );
	pr_info(MODULE_NAME ":temp.hp_att   = %d \n",temp.hp_att    );
	pr_info(MODULE_NAME ":temp.hp_gainup= %d \n",temp.hp_gainup );
	pr_info(MODULE_NAME ":temp.sp_att   = %d \n",temp.sp_att    );
	pr_info(MODULE_NAME ":temp.sp_gainup= %d \n",temp.sp_gainup );
#endif

	pr_info(MODULE_NAME ":%s completed\n",__func__);
	return 0;
}

/*******************************************************************************
 *	d4np2Write
 *
 *	Function:
 *			write register parameter function
 *	Argument:
 *			UINT8 bWriteRN  : register number
 *			UINT8 bWritePrm : register parameter
 *
 *	Return:
 *			SINT32	>= 0 success
 *					<  0 error
 *
 ******************************************************************************/
SINT32 d4np2Write(UINT8 bWriteRN, UINT8 bWritePrm)
{
	/* Write 1 byte data to the register for each system. */

	int rc = 0;
	
	if(pclient == NULL)
	{
		pr_err(MODULE_NAME ": i2c_client error\n");
		return -1;
	}
	//pr_err(MODULE_NAME ": addr %02x bRN %02x bPm %02x\n", pclient->addr, bWriteRN, bWritePrm);
	rc = i2c_smbus_write_byte_data(pclient, 0x80+bWriteRN, bWritePrm); 
	if(rc)
	{
		pr_err(MODULE_NAME ": i2c write error %d\n", rc);
		return rc;
	}
	
	d4np2Wait( D4NP2_I2CCTRLTIME );
	
	return 0;
}

/*******************************************************************************
 *	d4np2WriteN
 *
 *	Function:
 *			write register parameter function
 *	Argument:
 *			UINT8 bWriteRN  : register number
 *			UINT8 *pbWritePrm : register parameter
 *			UINT8 dWriteSize : size of "*pbWritePrm"
 *
 *	Return:
 *			SINT32	>= 0 success
 *					<  0 error
 *
 ******************************************************************************/
SINT32 d4np2WriteN(UINT8 bWriteRN, UINT8 *pbWritePrm, UINT8 bWriteSize)
{
	/* Write N byte data to the register for each system. */
	
#if 0	
	int rc = 0;

	if(pclient == NULL)
	{
		pr_err(MODULE_NAME ": i2c_client error\n");
		return -1;
	}

	rc = i2c_smbus_write_block_data(pclient, bWriteRN, bWriteSize, pbWritePrm); 
	if(rc)
	{
		pr_err(MODULE_NAME ": i2c write error %d\n", rc);
		return rc;
	}
	
	return 0;
#else
	
	int rc = 0;
	int i = 0;
	
	if(pclient == NULL)
	{
		pr_err(MODULE_NAME ": i2c_client error\n");
		return -1;
	}
	
	for(i=0; i<bWriteSize; i++)
	{
		rc = i2c_smbus_write_byte_data(pclient, 0x80+bWriteRN+i, pbWritePrm[i]); 
		if(rc)
		{
			pr_err(MODULE_NAME ": i2c write error %d\n", rc);
			return rc;
		}
	}

	return 0;
#endif
}

/*******************************************************************************
 *	d4np2Read
 *
 *	Function:
 *			read register parameter function
 *	Argument:
 *			UINT8 bReadRN : register number
 *			UINT8 *pbReadPrm : register parameter
 *
 *	Return:
 *			SINT32	>= 0 success
 *					<  0 error
 *
 ******************************************************************************/
SINT32 d4np2Read(UINT8 bReadRN, UINT8 *pbReadPrm)
{
	/* Read byte data to the register for each system. */
	
	int buf;
	
	if(pclient == NULL)
	{
		pr_err(MODULE_NAME ": i2c_client error\n");
		return -1;
	}

	buf  = i2c_smbus_read_byte_data(pclient, 0x80+bReadRN);	
	if(buf < 0)
	{
		pr_err(MODULE_NAME ": i2c read error %d\n", buf);
		return buf;
	}
	
	*pbReadPrm = (UINT8)buf;
	
	d4np2Wait( D4NP2_I2CCTRLTIME );

	return 0;
}

/*******************************************************************************
 *	d4np2Wait
 *
 *	Function:
 *			wait function
 *	Argument:
 *			UINT32 dTime : wait time [ micro second ]
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void d4np2Wait(UINT32 dTime)
{
	/* Wait procedure for each system */
	
	udelay(dTime);
}

/*******************************************************************************
 *	d4np2Sleep
 *
 *	Function:
 *			sleep function
 *	Argument:
 *			UINT32 dTime : sleep time [ milli second ]
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void d4np2Sleep(UINT32 dTime)
{
	/* Sleep procedure for each system */

	msleep(dTime);
}

/*******************************************************************************
 *	d4np2ErrHandler
 *
 *	Function:
 *			error handler function
 *	Argument:
 *			SINT32 dError : error code
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void d4np2ErrHandler(SINT32 dError)
{
	/* Error procedure for each system */

	pr_err(MODULE_NAME ": %s error %ld\n", __func__, dError);
}

/* D-4NP2 register map */
UINT8 g_bD4Np2RegisterMap[21] = 
	{	0x03,								/* #0 */
		0x07, 0x07, 0x93, 0x00, 0x0D,		/* #1 - #5 */
		0x00, 0x80, 0x80, 0x00, 0x80,		/* #6 - #10 */
		0x00, 0x00, 0x00, 0x40, 0x40,		/* #11 - #15 */
		0x00, 0x00, 0x00, 0x00, 0x00 };		/* #16 - #20 */
/* check flag : waiting charge time */
UINT8 g_bFlag_WaitCharge = 1;
/* state */
UINT8 g_bSpState = D4NP2_STATE_POWER_OFF;
UINT8 g_bHpState = D4NP2_STATE_POWER_OFF;

/*******************************************************************************
 *	D4Np2_UpdateRegisterMap
 *
 *	Function:
 *			update register map (g_bD4Np2RegisterMap[]) function
 *	Argument:
 *			SINT32	sdRetVal	: update flag
 *			UINT8	bRN			: register number
 *			UINT8	bPrm		: register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_UpdateRegisterMap( SINT32 sdRetVal, UINT8 bRN, UINT8 bPrm )
{
	//UINT32 dCnt = 0;

	if(sdRetVal < 0)
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	else
	{
		/* update register map */
		g_bD4Np2RegisterMap[ bRN ] = bPrm;
	}
}

/*******************************************************************************
 *	D4Np2_UpdateRegisterMapN
 *
 *	Function:
 *			update register map (g_bD4Np2RegisterMap[]) function
 *	Argument:
 *			SINT32	sdRetVal	: update flag
 *			UINT8	bRN			: register number
 *			UINT8	*pbPrm		: register parameter
 *			SINT8	bPrmSize	: size of " *pbPrm"
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_UpdateRegisterMapN( SINT32 sdRetVal, UINT8 bRN, UINT8 *pbPrm, UINT8 bPrmSize )
{
	UINT8 bCnt = 0;

	if(sdRetVal < 0)
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	else
	{
		/* update register map */
		for(bCnt = 0; bCnt < bPrmSize; bCnt++)
		{
			g_bD4Np2RegisterMap[ bRN + bCnt ] = pbPrm[ bCnt ];
		}
	}
}

/*******************************************************************************
 *	D4Np2_WriteRegisterBit
 *
 *	Function:
 *			write register "bit" function
 *	Argument:
 *			UINT32	bName	: register name
 *			UINT8	bPrm	: register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4Np2_WriteRegisterBit( UINT32 dName, UINT8 bPrm )
{
	UINT8 bWritePrm;		/* I2C sending parameter */
	UINT8 bDummy;			/* setting parameter */
	UINT8 bRN, bMB, bSB;	/* register number, mask bit, shift bit */

	/* 
	dName
	bit 31 - 24	: reserved
	bit 23 - 16	: register number 
	bit 15 -  8	: mask bit
	bit  7 -  0	: shift bit
	*/
	bRN = (UINT8)(( dName & 0xFF0000 ) >> 16);
	bMB = (UINT8)(( dName & 0x00FF00 ) >> 8);
	bSB = (UINT8)( dName & 0x0000FF );

	/* check arguments */
	if( bRN > MAX_WRITE_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	/* set register parameter */
	bPrm = (bPrm << bSB) & bMB;
	bDummy = bMB ^ 0xFF;
	bWritePrm = g_bD4Np2RegisterMap[ bRN ] & bDummy;	/* set bit of writing position to 0 */
	bWritePrm = bWritePrm | bPrm;						/* set parameter of writing bit */
	/* call the user implementation function "d4np2Write()", and write register */
	D4Np2_UpdateRegisterMap( d4np2Write( bRN, bWritePrm ), bRN, bWritePrm );
}

/*******************************************************************************
 *	D4Np2_WriteRegisterByte
 *
 *	Function:
 *			write register "byte" function
 *	Argument:
 *			UINT8 bWriteRN  : register number
 *			UINT8 bWritePrm : register parameter
 *
 *	Return:
 *			SINT32	>= 0 success
 *					<  0 error
 *
 ******************************************************************************/
void D4Np2_WriteRegisterByte(UINT8 bNumber, UINT8 bPrm)
{
	/* check arguments */
	if( bNumber > MAX_WRITE_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	D4Np2_UpdateRegisterMap( d4np2Write( bNumber, bPrm ), bNumber, bPrm );
}

/*******************************************************************************
 *	D4Np2_WriteRegisterByteN
 *
 *	Function:
 *			write register "n byte" function
 *	Argument:
 *			UINT8 bNumber	: register number
 *			UINT8 *pbPrm	: register parameter
 *			UINT8 bPrmSize		: size of "*pbPrm"
 *
 *	Return:
 *			SINT32	>= 0 success
 *					<  0 error
 *
 ******************************************************************************/
void D4Np2_WriteRegisterByteN(UINT8 bNumber, UINT8 *pbPrm, UINT8 bPrmSize)
{
	/* check arguments */
	if( bNumber > MAX_WRITE_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	D4Np2_UpdateRegisterMapN( d4np2WriteN( bNumber, pbPrm, bPrmSize ), bNumber, pbPrm, bPrmSize);
}

/*******************************************************************************
 *	D4Np2_ReadRegisterBit
 *
 *	Function:
 *			write register parameter function
 *	Argument:
 *			UINT8	bName	: register name
 *			UINT8	*pbPrm	: register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4Np2_ReadRegisterBit( UINT32 dName, UINT8 *pbPrm)
{
	SINT32 sdRetVal = D4NP2_SUCCESS;
	UINT8 bRN, bMB, bSB;	/* register number, mask bit, shift bit */

	/* 
	dName
	bit 31 - 24	: reserved
	bit 23 - 16	: register number 
	bit 15 -  8	: mask bit
	bit  7 -  0	: shift bit
	*/
	bRN = (UINT8)(( dName & 0xFF0000 ) >> 16);
	bMB = (UINT8)(( dName & 0x00FF00 ) >> 8);
	bSB = (UINT8)( dName & 0x0000FF );

	/* check arguments */
	if(bRN > MAX_READ_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	/* call the user implementation function "d4np2Read()", and read register */
	sdRetVal = d4np2Read( bRN, pbPrm );
	D4Np2_UpdateRegisterMap( sdRetVal, bRN, *pbPrm );
	/* extract the parameter of selected register in the read register parameter */
	*pbPrm = ((g_bD4Np2RegisterMap[ bRN ] & bMB) >> bSB);
}

/*******************************************************************************
 *	D4Np2_ReadRegisterByte
 *
 *	Function:
 *			write register parameter function
 *	Argument:
 *			UINT8	bNumber	: register number
 *			UINT8	*pbPrm	: register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4Np2_ReadRegisterByte( UINT8 bNumber, UINT8 *pbPrm)
{
	SINT32 sdRetVal = D4NP2_SUCCESS;

	/* check arguments */
	if(bNumber > MAX_READ_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	/* call the user implementation function "d4np2Read()", and read register */
	sdRetVal = d4np2Read( bNumber, pbPrm );
	D4Np2_UpdateRegisterMap( sdRetVal, bNumber, *pbPrm );
}

/*******************************************************************************
 *	D4Np2_SoftVolume_Wait
 *
 *	Function:
 *			wait soft volume time
 *	Argument:
 *			UINT8	bStartVol	: old volume
 *			UINT8	bEndVol		: new volume
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_SoftVolume_Wait( UINT8 bStartVol, UINT8 bEndVol )
{
	UINT32 dIdleTime;

	/* volume up */
	if(bStartVol < bEndVol)
	{
		dIdleTime = (UINT32)((bEndVol - bStartVol) * D4NP2_SOFTVOLUME_STEPTIME);
		d4np2Wait( dIdleTime );
	}
	/* volume down */
	else if(bStartVol > bEndVol)
	{
		dIdleTime = (UINT32)((bStartVol - bEndVol) * D4NP2_SOFTVOLUME_STEPTIME);
		d4np2Wait( dIdleTime );
	}
}

/*******************************************************************************
 *	D4Np2_PowerOff_ReceiverOut
 *
 *	Function:
 *			power off Receiver
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOff_ReceiverOut(void)
{
	/* power off PD_REC @#3 */
	D4Np2_WriteRegisterBit( PD_REC, 1 );
	/* power on/off PD_SNT @#3 */
#ifdef SHUNT_SW
	D4Np2_WriteRegisterBit( PD_SNT, 0 );
#else
	D4Np2_WriteRegisterBit( PD_SNT, 1 );
#endif
}

/*******************************************************************************
 *	D4Np2_PowerOff_SpOut
 *
 *	Function:
 *			power off SP part
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOff_SpOut(void)
{
	UINT8 bWriteRN, bWritePrm;
	UINT8 bSvol, bTempVol;

	/* power off SP */
	/* set SP volume to mute @#14 */
	bSvol = (g_bD4Np2RegisterMap[14] & 0x40) >> (SVOL_SP & 0xFF);
	bTempVol = g_bD4Np2RegisterMap[14] & 0x1F;
	D4Np2_WriteRegisterBit( MNA, 0 );
	/* wait soft volume time */
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
	}
	/* set SP mixer to mute @#13 */
	bWriteRN = 13;
	bWritePrm = g_bD4Np2RegisterMap[13] & 0x80;
	D4Np2_WriteRegisterByte( bWriteRN, bWritePrm );
	/* set SP state */
	g_bSpState = D4NP2_STATE_POWER_OFF;
}

/*******************************************************************************
 *	D4Np2_PowerOff_HpOut
 *
 *	Function:
 *			power off HP part
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOff_HpOut(void)
{
	UINT8 bWriteRN, abWritePrm[2];
	UINT8 bSvol, bTempVol;

	/* power off HP */
	/* set HP volume to mute @#15, 16 */
	bSvol = (g_bD4Np2RegisterMap[15] & 0x40) >> (SVOL_HP & 0xFF);
	bTempVol = g_bD4Np2RegisterMap[15] & 0x1F;
	abWritePrm[0] = g_bD4Np2RegisterMap[15] & 0xE0;
	abWritePrm[1] = g_bD4Np2RegisterMap[16] & 0xE0;
	bWriteRN = 15;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
	/* wait soft volume time */
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
	}
	/* set HP mixer to mute @#12 */
	bWriteRN = 12;
	abWritePrm[0] = g_bD4Np2RegisterMap[12] & 0x00;
	D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
	/* set PD_CHP @#2 */
	D4Np2_WriteRegisterBit( PD_CHP, 1 );
	/* set PD_REG @#2 */
	D4Np2_WriteRegisterBit( PD_REG, 1 );
	/* set HP state */
	g_bHpState = D4NP2_STATE_POWER_OFF;
}

/*******************************************************************************
 *	D4Np2_PowerOff_OutputPart
 *
 *	Function:
 *			power off D-4NP2 output part
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOff_OutputPart(void)
{
	/* power off Receiver */
	if( ((g_bD4Np2RegisterMap[3] >> (PD_REC & 0xFF)) & 0x01) == 0x00 )
	{
		D4Np2_PowerOff_ReceiverOut();
	}
	/* power off SP */
	if( g_bSpState == D4NP2_STATE_POWER_ON )
	{
		D4Np2_PowerOff_SpOut();
	}

	/* power off HP */
	if( g_bHpState == D4NP2_STATE_POWER_ON )
	{
		D4Np2_PowerOff_HpOut();
	}
}

/*******************************************************************************
 *	D4Np2_PowerOff_InputPart
 *
 *	Function:
 *			power off D-4NP2 input part
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOff_InputPart(void)
{
	UINT8 bWriteRN, abWritePrm[5];

	/* set input volume @#7 - 11 */
	abWritePrm[0] = g_bD4Np2RegisterMap[7] & 0xE0;
	abWritePrm[1] = g_bD4Np2RegisterMap[8] & 0xE0;
	abWritePrm[2] = g_bD4Np2RegisterMap[9] & 0xE0;
	abWritePrm[3] = g_bD4Np2RegisterMap[10] & 0xE0;
	abWritePrm[4] = g_bD4Np2RegisterMap[11] & 0xE0;
	bWriteRN = 7;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 5 );
}

/*******************************************************************************
 *	D4Np2_PowerOff_CommonPart
 *
 *	Function:
 *			power off D-4NP2 common part
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOff_CommonPart(void)
{
	/* power off  PDP @#1  */
	if( ((g_bD4Np2RegisterMap[0] & 0x02) >> (PDP & 0xFF)) == 0 )
	{
		D4Np2_WriteRegisterBit( PDP, 1 );
	}
	/* power off  PDPC @#0 */
	D4Np2_WriteRegisterBit( PDPC, 1 );
}

/*******************************************************************************
 *	D4Np2_PowerOn_CommonPart
 *
 *	Function:
 *			power on D-4NP2 common part
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOn_CommonPart(void)
{
	/* power on PDPC @#0 */
	if( (g_bD4Np2RegisterMap[0] & 0x01) == 1 )
	{
		D4Np2_WriteRegisterBit( PDPC, 0 );
	}
	/* power on PDP @#1 */
	if( ((g_bD4Np2RegisterMap[0] & 0x02) >> (PDP & 0xFF)) == 1 )
	{
		D4Np2_WriteRegisterBit( PDP, 0 );
	}
}

/*******************************************************************************
 *	D4Np2_PowerOn_InputPart
 *
 *	Function:
 *			power on D-4NP2 input part
 *	Argument:
 *			D4NP2_SETTING_INFO *pstSetInfo : D-4NP2 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOn_InputPart(D4NP2_SETTING_INFO *pstSetInfo)
{
	UINT8 bWriteRN, abWritePrm[5];
	UINT8 bZcs;
	UINT8 bPowerOff_Flag;

	/* check input volume, and power off output part */
	bPowerOff_Flag = 0;
	/* check MIN */
	if( (pstSetInfo->bMinVol == 0) || ((g_bD4Np2RegisterMap[7] & 0x1F) == 0) )
	{
		if(pstSetInfo->bMinVol == (g_bD4Np2RegisterMap[7] & 0x1F))
		{
			pstSetInfo->bHpMixer_Min = 0;
			pstSetInfo->bSpMixer_Min = 0;
		}
		else if(pstSetInfo->bMinVol < (g_bD4Np2RegisterMap[7] & 0x1F))
		{
			bPowerOff_Flag = 1;
			pstSetInfo->bHpMixer_Min = 0;
			pstSetInfo->bSpMixer_Min = 0;
		}
		else
		{
			bPowerOff_Flag=1;
		}
	}
	/* check LINE1 */
	if( (pstSetInfo->bLine1Vol == 0) || ((g_bD4Np2RegisterMap[8] & 0x1F) == 0) )
	{
		if(pstSetInfo->bLine1Vol == (g_bD4Np2RegisterMap[8] & 0x1F))
		{
			pstSetInfo->bHpMixer_Line1 = 0;
			pstSetInfo->bSpMixer_Line1 = 0;
		}
		else if(pstSetInfo->bLine1Vol < (g_bD4Np2RegisterMap[8] & 0x1F))
		{
			bPowerOff_Flag = 1;
			pstSetInfo->bHpMixer_Line1 = 0;
			pstSetInfo->bSpMixer_Line1 = 0;
		}
		else
		{
			bPowerOff_Flag=1;
		}
	}
	/* check LINE2 */
	if( (pstSetInfo->bLine2Vol == 0) || ((g_bD4Np2RegisterMap[10] & 0x1F) == 0) )
	{
		if(pstSetInfo->bLine2Vol == (g_bD4Np2RegisterMap[10] & 0x1F))
		{
			pstSetInfo->bHpMixer_Line2 = 0;
			pstSetInfo->bSpMixer_Line2 = 0;
		}
		else if(pstSetInfo->bLine2Vol < (g_bD4Np2RegisterMap[10] & 0x1F))
		{
			bPowerOff_Flag = 1;
			pstSetInfo->bHpMixer_Line2 = 0;
			pstSetInfo->bSpMixer_Line2 = 0;
		}
		else
		{
			bPowerOff_Flag=1;
		}
	}
	/* power off output part */
	if(bPowerOff_Flag == 1)
	{
		D4Np2_PowerOff_OutputPart();
	}

	/* set input volume  L/R ch synchronous flag and Zero Cross Mute @#7-11 */
	if( pstSetInfo->bInVolMode == 1 )
	{
		bZcs = 1;
	}
	else
	{
		bZcs = 0;
	}
	abWritePrm[0] = ( bZcs << (ZCS_MV & 0xFF)) | ((pstSetInfo->bMinVol & 0x1F) << (MNX & 0xFF));
	abWritePrm[1] = ( bZcs << (ZCS_SVA & 0xFF))	| (0x01 << (LAT_VA & 0xFF))
					| ((pstSetInfo->bLine1Vol & 0x1F) << (SVLA & 0xFF));
	abWritePrm[2] = ((pstSetInfo->bLine1Vol & 0x1F) << (SVRA & 0xFF));
	abWritePrm[3] = (bZcs << (ZCS_SVB & 0xFF))	| (0x01 << (LAT_VB & 0xFF))
					| ((pstSetInfo->bLine2Vol & 0x1F) << (SVLB & 0xFF));
	abWritePrm[4] = ((pstSetInfo->bLine2Vol & 0x1F) << (SVLB & 0xFF));
	bWriteRN = 7;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 5 );
	/* wait VREF charge time */
	d4np2Sleep( D4NP2_VREFCHARGETIME );
}

/*******************************************************************************
 *	D4Np2_PowerOn_ReceiverOut
 *
 *	Function:
 *			power on Receiver
 *	Argument:
 *			D4NP2_SETTING_INFO *pstSetInfo : D-4NP2 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOn_ReceiverOut(D4NP2_SETTING_INFO *pstSetInfo)
{
	/* power off PD_SNT @#3 */
	D4Np2_WriteRegisterBit( PD_SNT, 1 );
	/* power on PD_REC @#3 */
	D4Np2_WriteRegisterBit( PD_REC, pstSetInfo->bRecSW & 0x01 );
}

/*******************************************************************************
 *	D4Np2_PowerOn_SpOut
 *
 *	Function:
 *			power on SP part
 *	Argument:
 *			D4NP2_SETTING_INFO *pstSetInfo : D-4NP2 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOn_SpOut(D4NP2_SETTING_INFO *pstSetInfo)
{
	UINT8 bWriteRN, abWritePrm[3];
	UINT8 bZcs, bSvol, bTempVol, bTempMixer;

	/* set SP Hi-Z @#3 */
#ifdef SP_HIZ_ON
	abWritePrm[0] = (g_bD4Np2RegisterMap[3] & 0xF3)
					| (0x01 << (HIZ_SPL & 0xFF)) | (0x01 << (HIZ_SPR & 0xFF));
#else
	abWritePrm[0] = (g_bD4Np2RegisterMap[3] & 0xF3)
					| (0x00 << (HIZ_SPL & 0xFF)) | (0x00 << (HIZ_SPR & 0xFF));
#endif
	/* set Non Clip and Power Limit */
    /* DALC, DPLT @#4 */
	if( pstSetInfo->bSpNonClip == 1)
	{
		abWritePrm[1] = ((pstSetInfo->bDistortion & 0x07) << (DALC & 0xFF));
	}
	else
	{
		abWritePrm[1] = ((pstSetInfo->bPowerLimit & 0x07) << (DPLT & 0xFF));
	}
	/* DREL, DATT @#5 */
	abWritePrm[2] = ((pstSetInfo->bReleaseTime & 0x03)<< (DREL & 0xFF))
					| ((pstSetInfo->bAttackTime & 0x03) << (DATT & 0xFF));
	bWriteRN = 3;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 3 );

	/* check volume control mode */
	if(pstSetInfo->bSpVolMode == 2)
	{
		bZcs = 0;
		bSvol = 1;
	}
	else if(pstSetInfo->bSpVolMode == 1)
	{
		bZcs = 1;
		bSvol = 0;
	}
	else
	{
		bZcs = 0;
		bSvol = 0;
	}
	bTempMixer = ((pstSetInfo->bSpSwap & 0x01) << (SWAP_SP & 0xFF))
					| ((pstSetInfo->bSpMixer_Min & 0x01) << (SPR_MMIX & 0xFF))
					| ((pstSetInfo->bSpMixer_Line1 & 0x01) << (SPR_AMIX & 0xFF))
					| ((pstSetInfo->bSpMixer_Line2 & 0x01) << (SPR_BMIX & 0xFF))
					| ((pstSetInfo->bSpCh & 0x01) << (MONO_SP & 0xFF))
					| ((pstSetInfo->bSpMixer_Min & 0x01) << (SPL_MMIX & 0xFF))
					| ((pstSetInfo->bSpMixer_Line1 & 0x01) << (SPL_AMIX & 0xFF))
					| ((pstSetInfo->bSpMixer_Line2 & 0x01) << (SPL_BMIX & 0xFF));
	/* power on -> power on sequence */
	if(g_bSpState == D4NP2_STATE_POWER_ON )
	{
		if(bTempMixer != g_bD4Np2RegisterMap[13])
		{	
			/* set SP volume to mute @#14 */
			bTempVol = g_bD4Np2RegisterMap[14] & 0x1F;
			abWritePrm[0] = (bZcs << (ZCS_SPA & 0xFF)) | (bSvol << (SVOL_SP & 0xFF))
							| (0x00 << (MNA & 0xFF));
			bWriteRN = 14;
			D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
			/* wait soft volume time */
			if(bSvol == 1)
			{	
				D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
			}
		}
	}
	/* set SP mixer @#13 */
	abWritePrm[0] = bTempMixer;
	/* set SP volume @#14 */
	bTempVol = g_bD4Np2RegisterMap[14] & 0x1F;
	abWritePrm[1] = (bZcs << (ZCS_SPA & 0xFF)) | (bSvol << (SVOL_SP & 0xFF))
					| ((pstSetInfo->bSpVol & 0x1F) << (MNA & 0xFF));
	bWriteRN = 13;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
	/* wait soft volume idling time */
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, pstSetInfo->bSpVol & 0x1F );
	}
	/* set SP state */
	g_bSpState = D4NP2_STATE_POWER_ON;
}

/*******************************************************************************
 *	D4Np2_PowerOn_HpOut
 *
 *	Function:
 *			power on HP part
 *	Argument:
 *			D4NP2_SETTING_INFO *pstSetInfo : D-4NP2 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOn_HpOut(D4NP2_SETTING_INFO *pstSetInfo)
{
	UINT8 bWriteRN, abWritePrm[2];
	UINT8 bZcs, bSvol, bTempVol, bTempMixer;

	/* power on PD_REG @#2 */
	if( ((g_bD4Np2RegisterMap[2] & 0x04) >> (PD_REG & 0xFF)) == 1 )
	{
		D4Np2_WriteRegisterBit( PD_REG, 0 );
	}
	/* power on PD_CHP, set HP Hi-Z to on / off @#2 */
	if( ((g_bD4Np2RegisterMap[2] & 0x01) >> (PD_CHP & 0xFF)) == 1 )
	{
#ifdef HP_HIZ_ON
		abWritePrm[0] = ((g_bD4Np2RegisterMap[2] & 0xFE) | (0x01 << (HIZ_HP & 0xFF)));
#else
		abWritePrm[0] = (g_bD4Np2RegisterMap[2] & 0xFE);
#endif
		bWriteRN = 2;
		D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
		d4np2Wait(D4NP2_CHARGEPUMPWAKEUPTIME);	/* wait 500 usec */
	}
	/* check volume control mode */
	if(pstSetInfo->bHpVolMode == 2)
	{
			bZcs = 0;
			bSvol = 1;
	}
	else if(pstSetInfo->bHpVolMode == 1)
	{
		bZcs = 1;
		bSvol = 0;
	}
	else
	{
		bZcs = 0;
		bSvol = 0;
	}
	 bTempMixer = ((pstSetInfo->bHpMixer_Min & 0x01) << (HPR_MMIX & 0xFF))
					| ((pstSetInfo->bHpMixer_Line1 & 0x01) << (HPR_AMIX & 0xFF))
					| ((pstSetInfo->bHpMixer_Line2 & 0x01) << (HPR_BMIX & 0xFF))
					| ((pstSetInfo->bHpCh & 0x01) << (MONO_HP & 0xFF))
					| ((pstSetInfo->bHpMixer_Min & 0x01) << (HPL_MMIX & 0xFF))
					| ((pstSetInfo->bHpMixer_Line1 & 0x01) << (HPL_AMIX & 0xFF))
					| ((pstSetInfo->bHpMixer_Line2 & 0x01) << (HPL_BMIX & 0xFF));
	/* power on -> power on sequence */
	if(g_bHpState == D4NP2_STATE_POWER_ON)
	{
		if( bTempMixer != g_bD4Np2RegisterMap[12])
		{
			/* set HP volume to mute @#15, 16 */
			bTempVol = g_bD4Np2RegisterMap[15] & 0x1F;
			abWritePrm[0] = (bZcs << (ZCS_HPA & 0xFF)) | (bSvol << (SVOL_HP & 0xFF))
							| (0x01 << (LAT_HP & 0xFF))
							| (0x00 << (SALA & 0xFF));
			abWritePrm[1] = (0x00 << (SARA & 0xFF));
			bWriteRN = 15;
			D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
			/* wait soft volume time */
			if(bSvol == 1)
			{	
				D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
			}
		}
	}
	/* set HP Mixer @#12 */
	abWritePrm[0] = bTempMixer;
	bWriteRN = 12;
	D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
	/* set HP volume @#15, 16 */
	bTempVol = g_bD4Np2RegisterMap[15] & 0x1F;
	abWritePrm[0] = (bZcs << (ZCS_HPA & 0xFF)) | (bSvol << (SVOL_HP & 0xFF))
					| (0x01 << (LAT_HP & 0xFF))
					| ((pstSetInfo->bHpVol & 0x1F) << (SALA & 0xFF));
	abWritePrm[1] = ((pstSetInfo->bHpVol & 0x1F) << (SARA & 0xFF));
	bWriteRN = 15;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
	/* wait soft volume time */
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, pstSetInfo->bHpVol & 0x1F );
	}
	/* set HP state */
	g_bHpState = D4NP2_STATE_POWER_ON;
}

/*******************************************************************************
 *	D4Np2_PowerOn_OutputPart
 *
 *	Function:
 *			power on D-4NP2 output part
 *	Argument:
 *			D4NP2_SETTING_INFO *pstSetInfo : D-4NP2 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4Np2_PowerOn_OutputPart(D4NP2_SETTING_INFO *pstSetInfo)
{
	/* Receiver */
	if(pstSetInfo->bRecSW == 0){
		/* power on Receiver */
		if( ((g_bD4Np2RegisterMap[3] & 0x10) >> (PD_REC & 0xFF)) == 0x01 )
		{
			D4Np2_PowerOn_ReceiverOut(pstSetInfo);
		}
	}
	else
	{
		/* power off Receiver */
		if( ((g_bD4Np2RegisterMap[3] & 0x10) >> (PD_REC & 0xFF)) == 0x00 )
		{
			D4Np2_PowerOff_ReceiverOut();
		}
	}
	/* SP */
	if( (pstSetInfo->bSpVol == 0)
		|| ((pstSetInfo->bSpMixer_Min + pstSetInfo->bSpMixer_Line1 + pstSetInfo->bSpMixer_Line2) == 0) )
	{
		/* power off SP */
		D4Np2_PowerOff_SpOut();
	}
	else if((pstSetInfo->bMinVol == 0) && (pstSetInfo->bLine1Vol == 0) && (pstSetInfo->bLine2Vol == 0))
	{
		/* power off SP */
		D4Np2_PowerOff_SpOut();
	}
	else
	{
		/* power on SP */
		D4Np2_PowerOn_SpOut(pstSetInfo);
	}
	/* HP */
	if( (pstSetInfo->bHpVol == 0)
		|| ((pstSetInfo->bHpMixer_Min + pstSetInfo->bHpMixer_Line1 + pstSetInfo->bHpMixer_Line2) == 0) )
	{
		/* power off HP */
		D4Np2_PowerOff_HpOut();
	}
	else if((pstSetInfo->bMinVol == 0) && (pstSetInfo->bLine1Vol == 0) && (pstSetInfo->bLine2Vol == 0))
	{
		/* power off SP */
		D4Np2_PowerOff_HpOut();
	}
	else
	{
		/* power on HP */
		D4Np2_PowerOn_HpOut(pstSetInfo);
	}
	/* when HP and SP is off, set Input part and PDP to off */
	if( (g_bSpState == D4NP2_STATE_POWER_OFF) && (g_bHpState == D4NP2_STATE_POWER_OFF) )
	{
		/* power off Input part */
		D4Np2_PowerOff_InputPart();
		/* power off PDP */
		D4Np2_WriteRegisterBit( PDP, 1 );
	}
}

/*******************************************************************************
 *	D4Np2_PowerOn
 *
 *	Function:
 *			power on D-4NP2
 *	Argument:
 *			D4NP2_SETTING_INFO *pstSetInfo : D-4NP2 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4Np2_PowerOn(D4NP2_SETTING_INFO *pstSetInfo)
{
	/* power on common part */
	D4Np2_PowerOn_CommonPart();

	/* check PDPC power */
	if( g_bD4Np2RegisterMap[ 0 ] == 0x00 )
	{
		/* power on input part */
		D4Np2_PowerOn_InputPart(pstSetInfo);
		/* power on output part */
		D4Np2_PowerOn_OutputPart(pstSetInfo);
	}
}

/*******************************************************************************
 *	D4Np2_PowerOff
 *
 *	Function:
 *			power off D-4NP2
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4Np2_PowerOff(void)
{
	/* power off output part */
	D4Np2_PowerOff_OutputPart();
	/* power off input part */
	D4Np2_PowerOff_InputPart();
	/* power off common part */
	D4Np2_PowerOff_CommonPart();
}

void yda160_speaker_on(void) 			
{
	D4NP2_SETTING_INFO stInfo;

	stInfo.bInVolMode=1;		/* Input volume control mode : Zero Cross */
	
	stInfo.bHpCh=0;				/* HP Channel(Stereo/Mono) setting : stereo */
	stInfo.bHpVolMode=2;		/* HP volume control mode : Soft Volume */
	stInfo.bHpVol=0;			/* HP Volume : Mute */

	stInfo.bSpCh=0;				/* SP Channel(Stereo/Mono) setting : stereo */
	stInfo.bSpSwap=0;			/* SP Swap setting : no swap */
	stInfo.bSpVolMode=2;		/* SP volume control mode : Soft Volume */
	stInfo.bSpVol=31;			/* SP Volume : -16dB */

	stInfo.bSpNonClip=1;		/* Non Clip / Power Limit setting : Non Clip */
	stInfo.bPowerLimit=0;		/* Power Limit Parameter : Power Limit OFF */
	stInfo.bDistortion=4;		/* Non Clip Distortion Parameter : 3% */
	stInfo.bReleaseTime=3;		/* Non Clip Release Time : 0.3 sec/dB */
	stInfo.bAttackTime=1;		/* Non Clip Attack Time : 1.2 msec/dB */

	stInfo.bRecSW=1;			/* Receiver Switch : Power OFF */

	D4Np2_PowerOn(&stInfo);
}
void yda160_headset_on(void) 			
{
	D4NP2_SETTING_INFO stInfo;

	stInfo.bInVolMode=1;		/* Input volume control mode : Zero Cross */

	stInfo.bHpCh=0;				/* HP Channel(Stereo/Mono) setting : stereo */
	stInfo.bHpVolMode=2;		/* HP volume control mode : Soft Volume */
	stInfo.bHpVol=15;			/* HP Volume : -16dB */

	stInfo.bSpCh=0;				/* SP Channel(Stereo/Mono) setting : stereo */
	stInfo.bSpSwap=0;			/* SP Swap setting : no swap */
	stInfo.bSpVolMode=2;		/* SP volume control mode : Soft Volume */
	stInfo.bSpVol=0;			/* SP Volume : Mute */

	stInfo.bSpNonClip=1;		/* Non Clip / Power Limit setting : Non Clip */
	stInfo.bPowerLimit=0;		/* Power Limit Parameter : Power Limit OFF */
	stInfo.bDistortion=4;		/* Non Clip Distortion Parameter : 3% */
	stInfo.bReleaseTime=3;		/* Non Clip Release Time : 0.3 sec/dB */
	stInfo.bAttackTime=1;		/* Non Clip Attack Time : 1.2 msec/dB */

	stInfo.bRecSW=1;			/* Receiver Switch : Power OFF */

	D4Np2_PowerOn(&stInfo);
}

void yda160_speaker_headset_on(void) 
{
	D4NP2_SETTING_INFO stInfo;

	stInfo.bInVolMode=1;		/* Input volume control mode : Zero Cross */

	stInfo.bHpCh=0;				/* HP Channel(Stereo/Mono) setting : stereo */
	stInfo.bHpVolMode=2;		/* HP volume control mode : Soft Volume */
	stInfo.bHpVol=15;			/* HP Volume : -16dB */

	stInfo.bSpCh=0;				/* SP Channel(Stereo/Mono) setting : stereo */
	stInfo.bSpSwap=0;			/* SP Swap setting : no swap */
	stInfo.bSpVolMode=2;		/* SP volume control mode : Soft Volume */
	stInfo.bSpVol=15;			/* SP Volume : -16dB */

	stInfo.bSpNonClip=1;		/* Non Clip / Power Limit setting : Non Clip */
	stInfo.bPowerLimit=0;		/* Power Limit Parameter : Power Limit OFF */
	stInfo.bDistortion=4;		/* Non Clip Distortion Parameter : 3% */
	stInfo.bReleaseTime=3;		/* Non Clip Release Time : 0.3 sec/dB */
	stInfo.bAttackTime=1;		/* Non Clip Attack Time : 1.2 msec/dB */

	stInfo.bRecSW=1;			/* Receiver Switch : Power OFF */

	D4Np2_PowerOn(&stInfo);
}
void yda160_off(void)
{
	D4Np2_PowerOff();
}


static int amp_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int amp_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int amp_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	/* int mode;
	 * switch (cmd)
	 * {
		 * case SND_SET_AMPGAIN :
			 * if (copy_from_user(&mode, (void __user *) arg, sizeof(mode)))
			 * {
				 * pr_err(MODULE_NAME ": %s fail\n", __func__);
				 * break;
			 * }
			 * if (mode >= 0 && mode < MODE_NUM_MAX)
				 * cur_mode = mode;

			 * break;
		 * default :
			 * break;
	 * } */
	return 0;
}

static struct file_operations amp_fops = {
	.owner = THIS_MODULE,
	.open = amp_open,
	.release = amp_release,
	.ioctl = amp_ioctl,
};

static struct miscdevice amp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "amp",
	.fops = &amp_fops,
};

static int __devinit yda160_probe(struct i2c_client *client, const struct i2c_device_id * dev_id)
{
	int err = 0;
	
	pr_info(MODULE_NAME ":%s\n",__func__);


	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		goto exit_check_functionality_failed;

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		return err;
	}

	pclient = client;

	memcpy(&g_data, client->dev.platform_data, sizeof(struct yda160_i2c_data));

	if(misc_register(&amp_device))
	{
		pr_err(MODULE_NAME ": misc device register failed\n"); 		
	}
	load_ampgain();

	err = sysfs_create_group(&amp_device.this_device->kobj, &yda160_attribute_group);
	if(err)
	{
		pr_err(MODULE_NAME ":sysfs_create_group failed %s\n", amp_device.name);
		goto exit_sysfs_create_group_failed;
	}

	return 0;

exit_check_functionality_failed:
exit_sysfs_create_group_failed:
	return err;
}

static int __devexit yda160_remove(struct i2c_client *client)
{
	pclient = NULL;

	return 0;
}


static const struct i2c_device_id yda160_id[] = {
	{ "yda160", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, yda160_id);

static struct i2c_driver yda160_driver = {
	.id_table   	= yda160_id,
	.probe 			= yda160_probe,
	.remove 		= yda160_remove,
/*
#ifndef CONFIG_ANDROID_POWER
	.suspend    	= yda160_suspend,
	.resume 		= yda160_resume,
#endif
	.shutdown 		= yda160_shutdown,
*/
	.driver = {
		.name   = "yda160",
		.owner	= THIS_MODULE,
	},
};

static int __init yda160_init(void)
{
	pr_info(MODULE_NAME ":%s\n",__func__);
	return i2c_add_driver(&yda160_driver);
}

static void __exit yda160_exit(void)
{
	i2c_del_driver(&yda160_driver);
}

module_init(yda160_init);
module_exit(yda160_exit);

MODULE_AUTHOR("Justin Park");
MODULE_DESCRIPTION("YDA160 Driver");
MODULE_LICENSE("GPL");



