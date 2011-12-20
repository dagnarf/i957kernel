/*
 *  SMB328A-charger.c
 *  SMB328A charger interface driver
 *
 *  Copyright (C) 2011 Samsung Electronics
 *
 *  <jongmyeong.ko@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include <linux/regulator/machine.h>
#include <linux/smb328a_charger.h>

/* Register define */

enum {
	BAT_NOT_DETECTED,
	BAT_DETECTED
};

struct smb328a_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		psy_bat;
	struct smb328a_platform_data	*pdata;
};

static enum power_supply_property smb328a_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
};

static int smb328a_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int smb328a_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static void smb328a_print_reg(struct i2c_client *client, int reg)
{
	u8 data = 0;

	data = i2c_smbus_read_byte_data(client, reg);

	if (data < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, data);
	else
		printk("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);
}

/* vf check */
static bool smb328a_check_detbat(struct i2c_client *client)
{
	u8 data = 0;
	bool ret = true;

	printk("%s : \n", __func__);
	
	return ret;
}

/* whether charging enabled or not */
static bool smb328a_check_vdcin(struct i2c_client *client)
{
	u8 data = 0;
	bool ret = false;

	printk("%s : \n", __func__);
	
	return ret;
}

static int smb328a_chg_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct smb328a_chip *chip = container_of(psy,
				struct smb328a_chip, psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (smb328a_check_vdcin(chip->client))
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		if (smb328a_check_detbat(chip->client))
			val->intval = BAT_NOT_DETECTED;
		else
			val->intval = BAT_DETECTED;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* battery is always online */
		val->intval = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int smb328a_set_charging_current(struct i2c_client *client, int chg_current)
{
	int ret;
	u8 val;

	printk("%s : \n", __func__);
	
	if (chg_current < 200 || chg_current > 950)
		return -EINVAL;

	return ret;
}

static int smb328a_enable_charging(struct i2c_client *client)
{
	int ret;
	u8 val, mask;

	printk("%s : \n", __func__);

	return ret;
}

static int smb328a_disable_charging(struct i2c_client *client)
{
	int ret;
	u8 mask;

	printk("%s : \n", __func__);
	
	return ret;
}

static int smb328a_chg_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct smb328a_chip *chip = container_of(psy,
				struct smb328a_chip, psy_bat);
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_NOW: /* Set charging current */
		ret = smb328a_set_charging_current(chip->client, val->intval);
		break;
	case POWER_SUPPLY_PROP_STATUS:	/* Enable/Disable charging */
		if (val->intval == POWER_SUPPLY_STATUS_CHARGING)
			ret = smb328a_enable_charging(chip->client);
		else
			ret = smb328a_disable_charging(chip->client);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL: /* Set recharging current */
		if (val->intval < 50 || val->intval > 200) {
			dev_err(&chip->client->dev, "%s: invalid topoff current(%d)\n",
					__func__, val->intval);
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int __devinit smb328a_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct smb328a_chip *chip;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	printk("%s: SMB328A driver Loading! \n", __func__);

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	chip->psy_bat.name = "smb328a-charger",
	chip->psy_bat.type = POWER_SUPPLY_TYPE_BATTERY,
	chip->psy_bat.properties = smb328a_battery_props,
	chip->psy_bat.num_properties = ARRAY_SIZE(smb328a_battery_props),
	chip->psy_bat.get_property = smb328a_chg_get_property,
	chip->psy_bat.set_property = smb328a_chg_set_property,

	/* init power supplier framework */
	ret = power_supply_register(&client->dev, &chip->psy_bat);
	if (ret) {
		pr_err("Failed to register power supply psy_bat\n");
		goto err_kfree;
	}

	chip->pdata->hw_init(); /* important */

	smb328a_print_reg(client, 0x31);
	smb328a_print_reg(client, 0x32);
	smb328a_print_reg(client, 0x33);
	smb328a_print_reg(client, 0x34);
	smb328a_print_reg(client, 0x35);
	smb328a_print_reg(client, 0x36);
	smb328a_print_reg(client, 0x37);
	smb328a_print_reg(client, 0x38);
	smb328a_print_reg(client, 0x39);
	
	return 0;

err_kfree:
	kfree(chip);
	return ret;
}

static int __devexit smb328a_remove(struct i2c_client *client)
{
	struct smb328a_chip *chip = i2c_get_clientdata(client);

	power_supply_unregister(&chip->psy_bat);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_PM
static int smb328a_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct smb328a_chip *chip = i2c_get_clientdata(client);

	return 0;
}

static int smb328a_resume(struct i2c_client *client)
{
	struct smb328a_chip *chip = i2c_get_clientdata(client);

	return 0;
}
#else
#define smb328a_suspend NULL
#define smb328a_resume NULL
#endif /* CONFIG_PM */

static const struct i2c_device_id smb328a_id[] = {
	{ "smb328a", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, smb328a_id);

static struct i2c_driver smb328a_i2c_driver = {
	.driver	= {
		.name	= "smb328a",
	},
	.probe		= smb328a_probe,
	.remove		= __devexit_p(smb328a_remove),
	.suspend	= smb328a_suspend,
	.resume		= smb328a_resume,
	.id_table	= smb328a_id,
};

static int __init smb328a_init(void)
{
	return i2c_add_driver(&smb328a_i2c_driver);
}
module_init(smb328a_init);

static void __exit smb328a_exit(void)
{
	i2c_del_driver(&smb328a_i2c_driver);
}
module_exit(smb328a_exit);


MODULE_DESCRIPTION("SMB328A charger control driver");
MODULE_AUTHOR("<jongmyeong.ko@samsung.com>");
MODULE_LICENSE("GPL");
