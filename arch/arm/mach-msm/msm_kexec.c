/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
 */

#include <linux/kexec.h>
#include <linux/io.h>

#ifdef CONFIG_MSM_WATCHDOG
#include <mach/msm_iomap.h>

#define WDT0_EN        (MSM_TMR_BASE + 0x40)
#endif

void arch_kexec(void)
{
#ifdef CONFIG_MSM_WATCHDOG
	/* Prevent watchdog from resetting SoC */
	writel(0, WDT0_EN);
	pr_crit("KEXEC: MSM Watchdog Exit - Deactivated\n");
#endif
	return;
}
