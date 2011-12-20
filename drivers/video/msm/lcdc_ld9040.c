/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/delay.h>
#include <mach/gpio.h>
#include "msm_fb.h"
#include <linux/regulator/pmic8058-regulator.h>
#define LCDC_DEBUG

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk("ld9040 " x)
#else
#define DPRINT(x...)	
#endif

#define PMIC_GPIO_MLCD_RESET	PM8058_GPIO(17)  /* PMIC GPIO Number 17 */

/*
 * Serial Interface
 */
#define LCD_CSX_HIGH	gpio_set_value(spi_cs, 1);
#define LCD_CSX_LOW	gpio_set_value(spi_cs, 0);

#define LCD_SCL_HIGH	gpio_set_value(spi_sclk, 1);
#define LCD_SCL_LOW		gpio_set_value(spi_sclk, 0);

#define LCD_SDI_HIGH	gpio_set_value(spi_sdi, 1);
#define LCD_SDI_LOW		gpio_set_value(spi_sdi, 0);

#define DEFAULT_USLEEP	1	
static int lcdc_ld9040_panel_off(struct platform_device *pdev);

static int lcd_brightness = -1;

static int spi_cs;
static int spi_sclk;
static int spi_sdi;

static int delayed_backlight_value = -1;
static void lcdc_ld9040_set_brightness(int level);

struct ld9040_state_type{
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

static struct ld9040_state_type ld9040_state = { 0 };
static struct msm_panel_common_pdata *lcdc_ld9040_pdata;

#if 0
#define CMC623_nRST			28
#define CMC623_SLEEP		103
#define CMC623_BYPASS		104
#define CMC623_FAILSAFE		106

static int cmc623_init(void)
{

	int ret;
	DPRINT("%s\n",__func__);

	udelay(20);
	ret = gpio_request(CMC623_nRST, "CMC623_nRST");
	if(ret < 0) {
		printk(KERN_ERR "%s CMC gpio %d request failed\n",__func__,CMC623_nRST);
	}
	ret = gpio_request(CMC623_SLEEP, "CMC623_SLEEP");
	if(ret < 0) {
		printk(KERN_ERR "%s CMC gpio %d request failed\n",__func__,CMC623_SLEEP);
	}

	ret = gpio_request(CMC623_BYPASS, "CMC623_BYPASS");
	if(ret < 0) {
		printk(KERN_ERR "%s CMC gpio %d request failed\n",__func__,CMC623_BYPASS);
	}

	ret = gpio_request(CMC623_FAILSAFE, "CMC623_FAILSAFE");
	if(ret < 0) {
		printk(KERN_ERR "%s CMC gpio %d request failed\n",__func__,CMC623_FAILSAFE);
	}

	gpio_direction_output(CMC623_nRST, 1);
	gpio_direction_output(CMC623_SLEEP, 0);
	gpio_direction_output(CMC623_BYPASS, 0);
	gpio_direction_output(CMC623_FAILSAFE, 0);
}
static int cmc623_bypass_mode(void)
{

	int ret;
	DPRINT("%s\n",__func__);
	udelay(20);
	gpio_set_value(CMC623_FAILSAFE, 1);
	ret = gpio_get_value(CMC623_FAILSAFE);
	printk("%s, CMC623_FAILSAFE : %d\n",__func__,ret);
	udelay(20);
//	gpio_set_value(CMC623_BYPASS, 0);
	ret = gpio_get_value(CMC623_BYPASS);
	printk("%s, CMC623_BYPASS : %d\n",__func__,ret);
	udelay(20);
	gpio_set_value(CMC623_SLEEP, 1);
	ret = gpio_get_value(CMC623_SLEEP);
	printk("%s, CMC623_SLEEP : %d\n",__func__,ret);
	udelay(20);
	gpio_set_value(CMC623_nRST, 1);
	ret = gpio_get_value(CMC623_nRST);
	printk("%s, CMC623_nRST : %d\n",__func__,ret);
	DPRINT("%s end\n",__func__);

	return 0;
}

#endif


static void ld9040_disp_powerup(void)
{
	DPRINT("start %s\n", __func__);	

	if (!ld9040_state.disp_powered_up && !ld9040_state.display_on) {
		/* Reset the hardware first */

		/* Include DAC power up implementation here */
		
	    ld9040_state.disp_powered_up = TRUE;
	}
}

static void ld9040_disp_powerdown(void)
{
	DPRINT("start %s\n", __func__);	

	/* Reset Assert */

	/* turn off LDO */
	//TODO: turn off LDO

	ld9040_state.disp_powered_up = FALSE;
}


void ld9040_disp_on(void)
{
	int i;

	DPRINT("start %s - %d\n", __func__,system_rev);	

	if (ld9040_state.disp_powered_up && !ld9040_state.display_on) {

		mdelay(1);
		ld9040_state.display_on = TRUE;
	}
}

void ld9040_sleep_off(void)
{
	int i;

	DPRINT("start %s\n", __func__);	

	mdelay(1);
}

void ld9040_sleep_in(void)
{
	int i;

	DPRINT("start %s\n", __func__); 

	mdelay(1);
	
}

static int lcdc_ld9040_panel_on(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	

	if (!ld9040_state.disp_initialized) {
		/* Configure reset GPIO that drives DAC */
		lcdc_ld9040_pdata->panel_config_gpio(1);
		ld9040_disp_powerup();
		ld9040_disp_on();
		ld9040_state.disp_initialized = TRUE;

//		cmc623_init();
//		cmc623_bypass_mode();

	}

	return 0;
}

static int lcdc_ld9040_panel_off(struct platform_device *pdev)
{
	int i;

	DPRINT("start %s\n", __func__);	

	if (ld9040_state.disp_powered_up && ld9040_state.display_on) {
		lcdc_ld9040_pdata->panel_config_gpio(0);
		ld9040_state.display_on = FALSE;
		ld9040_state.disp_initialized = FALSE;
		ld9040_disp_powerdown();
	}
	return 0;
}


static void lcdc_ld9040_set_brightness(int level)
{

}

static void lcdc_ld9040_set_backlight(struct msm_fb_data_type *mfd)
{	
	int bl_level = mfd->bl_level;
	int tune_level;

}


static int __devinit ld9040_probe(struct platform_device *pdev)
{
	DPRINT("start %s: pdev->name:%s\n", __func__,pdev->name );	

	if (pdev->id == 0) {
		lcdc_ld9040_pdata = pdev->dev.platform_data;
		DPRINT("pdev->id = 0\n");

		return 0;
	}
	DPRINT("msm_fb_add_device START\n");
	msm_fb_add_device(pdev);
		
	DPRINT("msm_fb_add_device end\n");
	
	return 0;
}

static void ld9040_shutdown(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	

	lcdc_ld9040_panel_off(pdev);
}

static struct platform_driver this_driver = {
	.probe  = ld9040_probe,
	.shutdown	= ld9040_shutdown,
	.driver = {
		.name   = "lcdc_ld9040_wvga",
	},
};

static struct msm_fb_panel_data ld9040_panel_data = {
	.on = lcdc_ld9040_panel_on,
	.off = lcdc_ld9040_panel_off,
	.set_backlight = lcdc_ld9040_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel",
	.id	= 1,
	.dev	= {
		.platform_data = &ld9040_panel_data,
	}
};

#if 0
#define LCDC_FB_XRES	800
#define LCDC_FB_YRES	1280
#define LCDC_HPW		70
#define LCDC_HBP		60
#define LCDC_HFP		70
#define LCDC_VPW		3
#define LCDC_VBP		12
#define LCDC_VFP		1
#else
#define LCDC_FB_XRES	800
#define LCDC_FB_YRES	1280

#define LCDC_HPW		16
#define LCDC_VPW		3
#define LCDC_HBP		90
#define LCDC_VBP		12
#define LCDC_HFP		48
#define LCDC_VFP		4
/*
 *          .h_sync_width = 16, //32,
 *          .v_sync_width = 3, //10,
 *          .h_back_porch = 90, //80,
 *          .v_back_porch = 12, //24,
 *          .h_active = 800,
 *          .v_active = 1280,
 *          .h_front_porch = 48, //48,
 *          .v_front_porch = 4, //3,
 */
#endif
 
static int __init lcdc_ld9040_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_MDDI_AUTO_DETECT
	if (msm_fb_detect_client("lcdc_ld9040_wvga"))
	{
		printk(KERN_ERR "%s: msm_fb_detect_client failed!\n", __func__); 
		return 0;
	}
#endif
	DPRINT("start %s\n", __func__);	
	
	ret = platform_driver_register(&this_driver);
	if (ret)
	{
		printk(KERN_ERR "%s: platform_driver_register failed! ret=%d\n", __func__, ret); 
		return ret;
	}
        DPRINT("platform_driver_register(&this_driver) is done \n");
	pinfo = &ld9040_panel_data.panel_info;
	
	pinfo->xres = LCDC_FB_XRES;
	pinfo->yres = LCDC_FB_YRES;
	MSM_FB_SINGLE_MODE_PANEL(pinfo);
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 32;
	pinfo->fb_num = 2;
  	pinfo->clk_rate = 76800000;//24576000;
	pinfo->bl_max = 255;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = LCDC_HBP;
	pinfo->lcdc.h_front_porch = LCDC_HFP;
	pinfo->lcdc.h_pulse_width = LCDC_HPW;
	pinfo->lcdc.v_back_porch = LCDC_VBP;
	pinfo->lcdc.v_front_porch = LCDC_VFP;
	pinfo->lcdc.v_pulse_width = LCDC_VPW;

	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0xff;       /* blue */
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	DPRINT("platform_device_register(&this_device) is done \n");
	if (ret)
	{
		printk(KERN_ERR "%s: platform_device_register failed! ret=%d\n", __func__, ret); 
		platform_driver_unregister(&this_driver);
	}

	return ret;
}

module_init(lcdc_ld9040_panel_init);


