/*
 * s2mpb02-regulator.h - Voltage regulator driver for the Samsung s2mpb02
 *
 *  Copyright (C) 2014 Samsung Electrnoics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_MFD_S2MPB02_REGULATOR_H
#define __LINUX_MFD_S2MPB02_REGULATOR_H


/* S2MPB02 registers */
enum s2mpb02_reg {
	S2MPB02_REG_ID,
	S2MPB02_REG_INT1,
	S2MPB02_REG_INT1M,
	S2MPB02_REG_ST1,
	S2MPB02_REG_CTRL,
	S2MPB02_REG_RSVD5,
	S2MPB02_REG_RSVD6,
	S2MPB02_REG_RSVD7,
	S2MPB02_REG_RSVD8,
	S2MPB02_REG_BBEN_DBT,
	S2MPB02_REG_RSVDA,
	S2MPB02_REG_B1CTRL1,
	S2MPB02_REG_B1CTRL2,
	S2MPB02_REG_B2CTRL1,
	S2MPB02_REG_B2CTRL2,
	S2MPB02_REG_BB1CTRL1,
	S2MPB02_REG_BB1CTRL2,
	S2MPB02_REG_BUCK_RAMP,
	S2MPB02_REG_FLED_CTRL1,
	S2MPB02_REG_FLED_CTRL2,
	S2MPB02_REG_FLED_CUR1,
	S2MPB02_REG_FLED_TIME1,
	S2MPB02_REG_FLED_CUR2,
	S2MPB02_REG_FLED_TIME2,
	S2MPB02_REG_FLED_IRON1,
	S2MPB02_REG_FLED_IRON2,
	S2MPB02_REG_FLED_IRD1,
	S2MPB02_REG_FLED_IRD2,
	S2MPB02_REG_BST_CTRL1,
	S2MPB02_REG_BST_CTRL2,
	S2MPB02_REG_L1CTRL,
	S2MPB02_REG_L2CTRL,
	S2MPB02_REG_L3CTRL,
	S2MPB02_REG_L4CTRL,
	S2MPB02_REG_L5CTRL,
	S2MPB02_REG_L6CTRL,
	S2MPB02_REG_L7CTRL,
	S2MPB02_REG_L8CTRL,
	S2MPB02_REG_L9CTRL,
	S2MPB02_REG_L10CTRL,
	S2MPB02_REG_L11CTRL,
	S2MPB02_REG_L12CTRL,
	S2MPB02_REG_L13CTRL,
	S2MPB02_REG_L14CTRL,
	S2MPB02_REG_L15CTRL,
	S2MPB02_REG_L16CTRL,
	S2MPB02_REG_L17CTRL,
	S2MPB02_REG_L18CTRL,
	S2MPB02_REG_LDO_DSCH1,
	S2MPB02_REG_LDO_DSCH2,
	S2MPB02_REG_LDO_DSCH3,
};

/* S2MPB02 regulator ids */
enum S2MPB02_regulators {
	S2MPB02_LDO1,
	S2MPB02_LDO2,
	S2MPB02_LDO3,
	S2MPB02_LDO4,
	S2MPB02_LDO5,
	S2MPB02_LDO6,
	S2MPB02_LDO7,
	S2MPB02_LDO8,
	S2MPB02_LDO9,
	S2MPB02_LDO10,
	S2MPB02_LDO11,
	S2MPB02_LDO12,
	S2MPB02_LDO13,
	S2MPB02_LDO14,
	S2MPB02_LDO15,
	S2MPB02_LDO16,
	S2MPB02_LDO17,
	S2MPB02_LDO18,
	S2MPB02_BUCK1,
	S2MPB02_BUCK2,
	S2MPB02_BB1,

	S2MPB02_REG_MAX,
};

#define S2MPB02_BUCK_MIN1	400000
#define S2MPB02_BUCK_MIN2	2600000
#define S2MPB02_LDO_MIN1	600000
#define S2MPB02_BUCK_STEP1	6250
#define S2MPB02_BUCK_STEP2	12500
#define S2MPB02_LDO_STEP1	12500
#define S2MPB02_LDO_STEP2	25000
#define S2MPB02_LDO_VSEL_MASK	0x7F
#define S2MPB02_BUCK_VSEL_MASK	0xFF
#define S2MPB02_BUCK_ENABLE_MASK 0xC0
#define S2MPB02_LDO_ENABLE_MASK	0x80

#define S2MPB02_RAMP_DELAY	12000

#define S2MPB02_ENABLE_TIME_LDO		180
#define S2MPB02_ENABLE_TIME_BUCK	100
#define S2MPB02_ENABLE_TIME_BB		210

#define S2MPB02_BUCK_ENABLE_SHIFT	0x06
#define S2MPB02_LDO_ENABLE_SHIFT	0x07
#define S2MPB02_LDO_N_VOLTAGES	(S2MPB02_LDO_VSEL_MASK + 1)
#define S2MPB02_BUCK_N_VOLTAGES (S2MPB02_BUCK_VSEL_MASK + 1)

#define S2MPB02_REGULATOR_MAX (S2MPB02_REG_MAX)

#define DVS_DEFAULT_VALUE	(0x90)

#endif /* __LINUX_MFD_S2MPB02_PRIV_H */