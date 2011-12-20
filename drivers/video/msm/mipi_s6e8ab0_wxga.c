/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_s6e8ab0_wxga.h"
#include "mipi_s6e8ab0_wxga_gamma.h"
#define LCDC_DEBUG

//#define MIPI_SINGLE_WRITE

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk("[Mipi_LCD] " x)
#else
#define DPRINT(x...)	
#endif

static int state_lcd_on = FALSE;
static int acl_mode = 0; // default is on!

static struct msm_panel_common_pdata *mipi_s6e8ab0_wxga_pdata;
static struct msm_fb_data_type *mipi_mfd;

static struct dsi_buf s6e8ab0_wxga_tx_buf;
static struct dsi_buf s6e8ab0_wxga_rx_buf;

//smart dimming 
#define LCD_ID_BUFFER_SIZE (8)

static int id_read_flag = 0;
static unsigned char id_buffer[LCD_ID_BUFFER_SIZE] = {0, };
static struct dsi_cmd_desc **lcd_gamma_data;
//end smart dimming

static char AUTO_POW_ON[3] = {
		0xF0, 
		0x5A, 0x5A};

static char AUTO_POW_ON_2[3] = {
		0xF1, 
		0x5A, 0x5A};

static char ETC_COND_SET_0[7]= {
		0xF4, 
		0x0B, 0x0A, 0x06, 0x0B, 0x33, 0x02};
static char ETC_COND_SET_1[4] = {
		0xF6, 
		0x04, 0x00, 0x02};
static char ETC_COND_SET_2[22] = {
		0xD9, 
		0x14, 0x5C, 0x20, 0x0C, 0x0F, 0x41, 0x00, 0x10, 0x11, 0x12, 
		0xA8, 0xD5, 0x00, 0x00, 0x00, 0x00, 0x80, 0xCB, 0xED, 0x64, 
		0xAF
};

static char ACL_PARAMETER_SET[31] = {
		0xC1, /* ACL 50% register from SMD */
		0x4D, 0x96, 0x1D, 0x00, 0x00, 0x04, 0xFF, 0x00, 0x00, 0x03,
		0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x0E, 0x15,
		0x1C, 0x23, 0x2A, 0x31, 0x38, 0x3F, 0x46, 0x1D, 0x4D, 0x96, 
};

static char PANEL_COND_SET[41] = {
	0xF8 ,
	0x01, 0x8E, 0x00, 0x00, 0x00, 0xAC, 0x00, 0x9E, 0x8D, 0x1F, 
	0x4E, 0x9C, 0x7D, 0x3F, 0x10, 0x00, 0x20, 0x02, 0x10, 0x7D, 
	0x10, 0x00, 0x00, 0x02, 0x08, 0x10, 0x34, 0x34, 0x34, 0xC0, 
	0xC1, 0x01, 0x00, 0xC1, 0x82, 0x00, 0xC8, 0xC1, 0xE3, 0x01
};
	
static char DISPLAY_COND_SET[4] = {
		0xF2, 
		0xC8, 0x05, 0x0D
};



static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
static char acl_off_cmd[2] = {0xC0, 0x00};
static char acl_on_cmd[2] = {0xC0, 0x01};

static struct dsi_cmd_desc s6e8ab0_wxga_display_off_cmds[] = {
//	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
//	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc s6e8ab0_wxga_display_on_cmds_1[] = {
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(AUTO_POW_ON), AUTO_POW_ON}
};

static struct dsi_cmd_desc s6e8ab0_wxga_display_on_cmds_2[] = {
//    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(AUTO_POW_ON), AUTO_POW_ON},
		
    {DTYPE_DCS_WRITE,  1, 0, 0, 0,   sizeof(exit_sleep), exit_sleep},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(AUTO_POW_ON_2), AUTO_POW_ON_2},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(PANEL_COND_SET), PANEL_COND_SET},    
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(AUTO_POW_ON_2), AUTO_POW_ON_2},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(DISPLAY_COND_SET), DISPLAY_COND_SET},    
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0,   sizeof(gamma_select_cmd), gamma_select_cmd},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(GAMMA_COND_SET), gamma22_30_sm2},    ///////////// low level
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0,   sizeof(gamma_update_cmd), gamma_update_cmd},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(AUTO_POW_ON_2), AUTO_POW_ON_2},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(ETC_COND_SET_0), ETC_COND_SET_0},    
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(ETC_COND_SET_1), ETC_COND_SET_1},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(ETC_COND_SET_2), ETC_COND_SET_2},
    {DTYPE_DCS_WRITE,  1, 0, 0, 0,   sizeof(display_on), display_on}
};

static struct dsi_cmd_desc s6e8ab0_wxga_acl_control_on[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,   sizeof(ACL_PARAMETER_SET), ACL_PARAMETER_SET},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(acl_on_cmd), acl_on_cmd},
};

static struct dsi_cmd_desc s6e8ab0_wxga_acl_control_off[] = {
    {DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(acl_off_cmd), acl_off_cmd},
};

static char manufacture_id1[2] = {0xDA, 0x00}; /* DTYPE_DCS_READ */
static char manufacture_id2[2] = {0xDB, 0x00}; /* DTYPE_DCS_READ */
static char manufacture_id3[2] = {0xDC, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc s6e8aa0_manufacture_id1_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id1), manufacture_id1};

static struct dsi_cmd_desc s6e8aa0_manufacture_id2_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id2), manufacture_id2};

static struct dsi_cmd_desc s6e8aa0_manufacture_id3_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id3), manufacture_id3};

static char manufacture_id[2] = {0x04, 0x00}; /* DTYPE_DCS_READ */
static struct dsi_cmd_desc s6e8aa0_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_s6e8aa0_manufacture_ids(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &s6e8ab0_wxga_tx_buf;
	rp = &s6e8ab0_wxga_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &s6e8aa0_manufacture_id1_cmd;

	mutex_lock(&mfd->dma->ov_mutex);
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 4);
	mutex_unlock(&mfd->dma->ov_mutex);
	
	lp = (uint32 *)rp->data;
	
	DPRINT("%s: manufacture_id1=0x%x\n", __func__, *lp);


	tp = &s6e8ab0_wxga_tx_buf;
	rp = &s6e8ab0_wxga_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &s6e8aa0_manufacture_id2_cmd;

	mutex_lock(&mfd->dma->ov_mutex);
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 4);
	mutex_unlock(&mfd->dma->ov_mutex);
	
	lp = (uint32 *)rp->data;
	
	DPRINT("%s: manufacture_id2=0x%x\n", __func__, *lp);


	tp = &s6e8ab0_wxga_tx_buf;
	rp = &s6e8ab0_wxga_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &s6e8aa0_manufacture_id3_cmd;

	mutex_lock(&mfd->dma->ov_mutex);
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 4);
	mutex_unlock(&mfd->dma->ov_mutex);
	
	lp = (uint32 *)rp->data;
	
	DPRINT("%s: manufacture_id3=0x%x\n", __func__, *lp);

	return *lp;
}


static int p8lte_acl_mode_control(int enable)
{
	struct msm_fb_data_type *mfd = mipi_mfd;
	
	printk("[LCD] AMOLED ACL MODE set to %s\n",enable?"ON":"OFF");


    DPRINT("%s +\n", __func__);

	if (!mfd) {
		printk("mfd is NULL\n");
		return -ENODEV;
	}
	if (mfd->key != MFD_KEY) {
		printk("mfd->key != MFD_KEY\n");
		return -EINVAL;
	}
    if(!state_lcd_on) return 0;

	mutex_lock(&mfd->dma->ov_mutex);

	if(enable) {
		mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_acl_control_on,
			ARRAY_SIZE(s6e8ab0_wxga_acl_control_on));
	} else {
		mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_acl_control_off,
			ARRAY_SIZE(s6e8ab0_wxga_acl_control_off));
	}

	mutex_unlock(&mfd->dma->ov_mutex);

	return 0;

}


static uint32 mipi_s6e8aa0_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &s6e8ab0_wxga_tx_buf;
	rp = &s6e8ab0_wxga_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &s6e8aa0_manufacture_id_cmd;

	mutex_lock(&mfd->dma->ov_mutex);
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 4);
	mutex_unlock(&mfd->dma->ov_mutex);
	
	lp = (uint32 *)rp->data;
	
	DPRINT("%s: manufacture_id=0x%x\n", __func__, *lp);

	return *lp;
}

//smart dimming
void read_reg( char srcReg, int srcLength, char* destBuffer, const int isUseMutex )
{
	const int	one_read_size = 4;
	const int	loop_limit = 16;

	static char read_reg[2] = {0xFF, 0x00}; // first byte = read-register
	static struct dsi_cmd_desc s6e8aa0_read_reg_cmd = 
	{ DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(read_reg), read_reg};

	static char packet_size[] = { 0x04, 0 }; // first byte is size of Register
	static struct dsi_cmd_desc s6e8aa0_packet_size_cmd = 
	{ DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(packet_size), packet_size};

	static char reg_read_pos[] = { 0xB0, 0x00 }; // second byte is Read-position
	static struct dsi_cmd_desc s6e8aa0_read_pos_cmd = 
	{ DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(reg_read_pos), reg_read_pos};

	int read_pos;
	int readed_size;
	int show_cnt;

	int i,j;
	char	show_buffer[256];
	int	show_buffer_pos = 0;

	read_reg[0] = srcReg; //0xd1

	show_buffer_pos += sprintf( show_buffer, "read_reg : %X[%d] : ", srcReg, srcLength );

	read_pos = 0;
	readed_size = 0;
//	if( isUseMutex ) mutex_lock(&(s6e8aa0_lcd.lock));

//	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000); // lp

	packet_size[0] = (char) srcLength;
	mipi_dsi_cmds_tx(mipi_mfd, &s6e8ab0_wxga_tx_buf, &(s6e8aa0_packet_size_cmd),	1);

	show_cnt = 0;
	for( j = 0; j < loop_limit; j ++ ) // 16
	{
		reg_read_pos[1] = read_pos;
		if( mipi_dsi_cmds_tx(mipi_mfd, &s6e8ab0_wxga_tx_buf, &(s6e8aa0_read_pos_cmd), 1) < 1 ) {
			show_buffer_pos += sprintf( show_buffer +show_buffer_pos, "Tx command FAILED" );
			break;
		}

		mipi_dsi_buf_init(&s6e8ab0_wxga_rx_buf);
		readed_size = mipi_dsi_cmds_rx(mipi_mfd, &s6e8ab0_wxga_tx_buf, &s6e8ab0_wxga_rx_buf, &s6e8aa0_read_reg_cmd, one_read_size);
		for( i=0; i < readed_size; i++, show_cnt++) 
		{
			show_buffer_pos += sprintf( show_buffer +show_buffer_pos, "%02x ", s6e8ab0_wxga_rx_buf.data[i] );
			if( destBuffer != NULL && show_cnt < srcLength ) {
				destBuffer[show_cnt] = s6e8ab0_wxga_rx_buf.data[i];
			} // end if
		} // end for
		show_buffer_pos += sprintf( show_buffer +show_buffer_pos, "." );
		read_pos += readed_size;
		if( read_pos > srcLength ) break;
	} // end for
//	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000); // hs
//	if( isUseMutex ) mutex_unlock(&(s6e8aa0_lcd.lock));
	if( j == loop_limit ) show_buffer_pos += sprintf( show_buffer +show_buffer_pos, "Overrun" );

	DPRINT( "%s\n", show_buffer );
	
}
//end smart dimming



static int mipi_s6e8ab0_wxga_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int i;
    int len;
    DPRINT("%s +\n", __func__);
	mfd = platform_get_drvdata(pdev);

	mipi_mfd = mfd;

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mutex_lock(&mfd->dma->ov_mutex);
	mdelay(5);		
    len = ARRAY_SIZE(s6e8ab0_wxga_display_on_cmds_1);
	if(mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_display_on_cmds_1, len) != len)
	{
        goto lcd_on_error;
	}

	//smart dimming
	// ID READ
	if(!id_read_flag)
	{
		id_read_flag = 1;
		read_reg( 0xd1, LCD_ID_BUFFER_SIZE, id_buffer, FALSE ); //huggac
		//	update_id( id_buffer, &s6e8aa0_lcd );
		//	update_LCD_SEQ_by_id( &s6e8aa0_lcd );
		
		//smart dimming - id check
		if( id_buffer[0] == 0x00 )
		{
			for( i = 0; i < LCD_ID_BUFFER_SIZE-2; i ++ ) id_buffer[i] = id_buffer[i+2];
		}
		//printk(" id2 = %x\n",  id_buffer[1]);

		switch(id_buffer[1])
		{
			case 0x15:	lcd_gamma_data = GAMMA_COND_SET_LEVEL_SM2; // sm2
						break;
			case 0x25:	lcd_gamma_data = GAMMA_COND_SET_LEVEL;// m3
						break;
			default:		lcd_gamma_data = GAMMA_COND_SET_LEVEL;// m3
						break;
		}
		//end smart dimming id check
		
	}
	//end smart dimming
	len = ARRAY_SIZE(s6e8ab0_wxga_display_on_cmds_2);
	if(mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_display_on_cmds_2, len) != len)
	{
        goto lcd_on_error;
	}

    state_lcd_on = TRUE;
	mutex_unlock(&mfd->dma->ov_mutex);

	if(acl_mode) {
		p8lte_acl_mode_control(acl_mode);
	}

    	DPRINT("%s -\n", __func__);
	return 0;

lcd_on_error:
    mutex_unlock(&mfd->dma->ov_mutex);
   	DPRINT("%s - ERROR\n", __func__);
	return 0;    
}

static int mipi_s6e8ab0_wxga_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
    int len;
    DPRINT("%s +\n", __func__);
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mutex_lock(&mfd->dma->ov_mutex);
    len = ARRAY_SIZE(s6e8ab0_wxga_display_off_cmds);
	if(mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_display_off_cmds, len) != len)
	{
        goto lcd_off_error;
	}
    state_lcd_on = FALSE;
	mutex_unlock(&mfd->dma->ov_mutex);
   	DPRINT("%s -\n", __func__);
	return 0;
    
lcd_off_error:
    mutex_unlock(&mfd->dma->ov_mutex);
   	DPRINT("%s - ERROR\n", __func__);
	return 0; 
}

static int mipi_s6e8ab0_set_backlight(struct msm_fb_data_type *mfd)
{
	int level;
    int len;
	
	printk("[LCD] BACKLIGHT : %d\n",mfd->bl_level);
#if 1
	if(!mfd)
		return -ENODEV;
	if(mfd->key != MFD_KEY)
		return -EINVAL;

    if(!state_lcd_on) return 0;

	if(mfd->bl_level > 27 ) {
		printk("%d is not in range (1~24)\n",mfd->bl_level);
		return 0;
	}

	if(mfd->bl_level <3) {
		level = 0;
	} else  { 
	 	level = mfd->bl_level -3;
	}

	mutex_lock(&mfd->dma->ov_mutex);
	//mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, GAMMA_COND_SET_LEVEL_SM2[level],//GAMMA_COND_SET_LEVEL
	//mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, GAMMA_COND_SET_LEVEL[level],
	len = ARRAY_SIZE(s6e8ab0_wxga_gamma_control_30);
	if(mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, lcd_gamma_data[level], len) != len)
	{
        goto lcd_bl_error;
	}
	mutex_unlock(&mfd->dma->ov_mutex);

	printk("[LCD] End of %s\n",__func__);
#endif
    return 0;

lcd_bl_error:
    mutex_unlock(&mfd->dma->ov_mutex);
   	DPRINT("%s - ERROR\n", __func__);
	return 0; 
}

/* ##########################################################
 * #
 * # P5LTE ACL on/off  Sysfs node
 * # 
 * ##########################################################*/
static struct class *mdnie_class;
struct device *amoled_dev;

static ssize_t mdnie_cabc_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk("[LCD] Current ACL MODE : %s\n",acl_mode?"ON":"OFF");
	return sprintf(buf,"Current ACL Mode : %s\n",acl_mode?"On":"Off");;
}


static ssize_t mdnie_cabc_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	printk("%s : %d",buf, value);
	if(value != 0 && value != 1) {
		printk("[LCD] INVALID scope of acl mode. 1 or 0\n");
		return size;
	}
	if(acl_mode == value) {
		printk("[LCD] Already ACL MODE %s\n",value?"ON":"OFF");
		return size;
	}
	acl_mode = value;

	p8lte_acl_mode_control(acl_mode);

    return size;
}

static DEVICE_ATTR(mdnie_cabc, 0666, mdnie_cabc_show, mdnie_cabc_store);
/*  ############# END of ACL on / off sysfs node ############### */

/* ##########################################################
 * #
 * # P5LTE Color temperature change node
 * # 
 * ##########################################################*/
static ssize_t mdnie_bg_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk("[LCD] Current ACL MODE : %s\n",acl_mode?"ON":"OFF");
	return sprintf(buf,"Current ACL Mode : %s\n",acl_mode?"On":"Off");;
}


static ssize_t mdnie_bg_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	struct msm_fb_data_type *mfd = mipi_mfd;
	
    DPRINT("%s +\n", __func__);

	if (!mfd) {
		printk("mfd is NULL\n");
		return -ENODEV;
	}
	if (mfd->key != MFD_KEY) {
		printk("mfd->key != MFD_KEY\n");
		return -EINVAL;
	}
    if(!state_lcd_on) return 0;

	sscanf(buf, "%d", &value);
	printk("%s : %d",buf, value);

    mutex_lock(&mfd->dma->ov_mutex);
	if(mfd->bl_level < 14) { /* bl_lvl 255 = 300CD */
		switch(value) {
			case 0:
				mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_1,
							ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));
				break;
			case 1:
				mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_2,
							ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

				break;
			case 2:
				mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_3,
								ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));
				break;
		}

	} else {
		switch(value) {
			case 0:
				mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_4,
								ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

				break;
			case 1:
				mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_5,
								ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

				break;
			case 2:
				mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_6,
								ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

				break;

		}

	}
    mutex_unlock(&mfd->dma->ov_mutex);

    return size;
}

static DEVICE_ATTR(mdnie_bg, 0666, mdnie_bg_show, mdnie_bg_store);
/*  ############# END of ACL on / off sysfs node ############### */

static int p8lte_hw_test_mode(int mode)
{
	struct msm_fb_data_type *mfd = mipi_mfd;
	
	printk("[LCD] AMOLED HW Test MODE set to %s\n",mode?"ON":"OFF");


    DPRINT("%s +\n", __func__);

	if (!mfd) {
		printk("mfd is NULL\n");
		return -ENODEV;
	}
	if (mfd->key != MFD_KEY) {
		printk("mfd->key != MFD_KEY\n");
		return -EINVAL;
	}
    if(!state_lcd_on) return 0;

	mutex_lock(&mfd->dma->ov_mutex);

	switch(mode) {
		case 1:
			mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_1,
			ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));
			break;
		case 2:
			mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_2,
			ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

			break;
		case 3:
			mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_3,
			ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

			break;
		case 4:
			mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_4,
			ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

			break;
		case 5:
			mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_5,
			ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

			break;
		case 6:
			mipi_dsi_cmds_tx(mfd, &s6e8ab0_wxga_tx_buf, s6e8ab0_wxga_hw_test_mode_6,
			ARRAY_SIZE(s6e8ab0_wxga_hw_test_mode_1));

			break;
	}

	mutex_unlock(&mfd->dma->ov_mutex);
	return 0;
}


static int current_hw_setting = 0;
static ssize_t hw_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk("[LCD] Current HW Setting : %d\n",current_hw_setting);
	return sprintf(buf,"Current HW Setting : %d\n",current_hw_setting);;
}


static ssize_t hw_test_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	printk("%s : %d",buf, value);
	if(value < 0 || value > 7) {
		printk("[LCD] INVALID scope of hw test mode. 1 ~ 6\n");
		return size;
	}
	if(current_hw_setting == value) {
		printk("[LCD] Already HW test MODE %s\n",value);
		return size;
	}
	current_hw_setting = value;

	p8lte_hw_test_mode(current_hw_setting);

    return size;
}

static DEVICE_ATTR(hw_test, 0664, hw_test_show, hw_test_store);

static int __devinit mipi_s6e8ab0_wxga_lcd_probe(struct platform_device *pdev)
{
    DPRINT("%s \n", __func__);
	if (pdev->id == 0) {
		mipi_s6e8ab0_wxga_pdata = pdev->dev.platform_data;
		return 0;
	}

	DPRINT("msm_fb_add_device START\n");
	msm_fb_add_device(pdev);
	DPRINT("msm_fb_add_device end\n");

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_s6e8ab0_wxga_lcd_probe,
	.driver = {
		.name   = "mipi_s6e8ab0_wxga",
	},
};

static struct msm_fb_panel_data s6e8ab0_wxga_panel_data = {
	.on				= mipi_s6e8ab0_wxga_lcd_on,
	.off			= mipi_s6e8ab0_wxga_lcd_off,
	.set_backlight 	= mipi_s6e8ab0_set_backlight,
};

static int ch_used[3];

int mipi_s6e8ab0_wxga_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_s6e8ab0_wxga", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	s6e8ab0_wxga_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &s6e8ab0_wxga_panel_data,
		sizeof(s6e8ab0_wxga_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	/*  Prepare sysfs node for acl on/off  */
	mdnie_class = class_create(THIS_MODULE, "mdnie");
	if(IS_ERR(mdnie_class)) {
		printk("Failed to create class(mdnie_class)!!\n");
		ret = -1;
	}

	/*  P8LTE has not cmc623, fake device for TouchWIZ Platform */
    amoled_dev = device_create(mdnie_class, NULL, 0, NULL, "cmc623");
    if (IS_ERR(amoled_dev))
        printk("[LCD] Failed to create device(amoled_dev)!\n");
    if (device_create_file(amoled_dev, &dev_attr_mdnie_cabc) < 0)
       printk("[LCD] Failed to create device file(%s)!\n", dev_attr_mdnie_cabc.attr.name);
	if (device_create_file(amoled_dev, &dev_attr_hw_test) < 0)
       printk("[LCD] Failed to create device file(%s)!\n", dev_attr_hw_test.attr.name);
	if (device_create_file(amoled_dev, &dev_attr_mdnie_bg) < 0)
       printk("[LCD] Failed to create device file(%s)!\n", dev_attr_mdnie_bg.attr.name);

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_s6e8ab0_wxga_lcd_init(void)
{
    DPRINT("%s \n", __func__);
	mipi_dsi_buf_alloc(&s6e8ab0_wxga_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&s6e8ab0_wxga_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_s6e8ab0_wxga_lcd_init);
