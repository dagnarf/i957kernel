
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/regulator/machine.h>
#include <asm/irq.h>

#define BIT_SIZE 32
#define MAX_SIZE 1024
#define NANO_SEC 1000*1000*1000
#define MICRO_SEC 1000*1000

#define IRDA_OUT	73
#define IRDA_EN		62

extern struct class *sec_class;

struct ir_remocon_data
{
	struct mutex mutex;
	struct work_struct work;
	int gpio;
	int pwr_en;	
	unsigned int signal[MAX_SIZE];
};


static int gpio_init(struct ir_remocon_data *data)
{
	int err;
	err = gpio_request(data->gpio, "ir_gpio");
	if (err < 0) {
		pr_err("failed to request GPIO %d, error %d\n",
			data->gpio, err);
	} else
	
		gpio_set_value(data->gpio, 0);
	return err;
}

static void enable_high(struct ir_remocon_data *data, unsigned int cnt, unsigned int period)
{
	int i;
	unsigned int duty = period /3;
	unsigned int on,off = 0;

	if ( cnt <=27){
		on = duty-3;
		off = period-duty-4;
	}else if(cnt <=95){
		on = duty-2;
		off = period-duty-4;
	}else{
		on = duty-2;
		off = period-duty-3;
	}

	for ( i =0; i < cnt ; i++) {
		gpio_direction_output(data->gpio, 1);
		__udelay(on);
		gpio_direction_output(data->gpio, 0);
		__udelay(off);
	}
}



static void ir_remocon_send(struct ir_remocon_data *data)
{
	struct regulator *regulator;

	unsigned int period = 0;
	unsigned int off_period = 0;
	int i;
	period = MICRO_SEC / data->signal[0];

	regulator = regulator_get(NULL, "8058_l11");
	regulator_set_voltage(regulator, 2850000, 2850000);
	regulator_enable(regulator);


	if(data->pwr_en != NULL)
	gpio_direction_output(data->pwr_en, 1);

	local_irq_disable();
	for ( i =1 ; i < MAX_SIZE; ) {
		if (data->signal[i] == 0)
			break;
		enable_high(data, data->signal[i++], period);
		off_period = data->signal[i++]*period+50;
		__udelay(off_period);
	}
	local_irq_enable();

	if(data->pwr_en != NULL)
	gpio_direction_output(data->pwr_en, 0);

	regulator_disable(regulator);
}

static void ir_remocon_send_test(struct ir_remocon_data *data)
{
	struct regulator *regulator;

	unsigned int period = 0;
	int i;
	period = MICRO_SEC / data->signal[0];

	regulator = regulator_get(NULL, "8058_l11");
	regulator_set_voltage(regulator, 2850000, 2850000);
	regulator_enable(regulator);

	if(data->pwr_en != NULL)
	gpio_direction_output(data->pwr_en, 1);

	local_irq_disable();
	for ( i =1 ; i < MAX_SIZE; i++ ) {
		if (data->signal[i] == 0)
			break;
		else if (data->signal[i] == 10){
			gpio_direction_output(data->gpio, 1);
			__udelay(period);
		}
		else if (data->signal[i] == 5){
			gpio_direction_output(data->gpio, 0);
			__udelay(period);
		}
	}
	local_irq_enable();
	
	if(data->pwr_en != NULL )
	gpio_direction_output(data->pwr_en, 0);

	regulator_force_disable(regulator);
}


// #define IR_TEST ==> Not valuable(2011.09.02)
#ifdef IR_TEST
const unsigned int signal1[MAX_SIZE] = {38400,173,171,24,62,24,61,24,62,24,17,24,17,
	24,18,24,17,24,19,22,62,24,61,24,62,24,19,22,17,25,17,24,17,24,17,24,62,24,
	61,25,61,24,17,24,19,23,17,24,17,24,20,22, 17,24,17,24,17,25,61,24,62,24,61,24,62,24,61,24,1880, 0, };
const unsigned int signal2[MAX_SIZE] = {38400,173,171,24,62,24,61,24,62,24,17,24,17,24,18,24,17,24,18,23,62,24,61,24,62,24,18,23,17,25,17,24,17,24,17,24,62,24,61,25,17,24,61,24,18,24,17,24,17,24,18,24,17,24,17,24,62,24,17,24,62,24,61,24,62,24,61,24,1880,0, };
void ir_test(bool up)
{

	struct regulator *regulator;
	unsigned int period = 0;
	int i;
	unsigned int signal[MAX_SIZE];


	regulator = regulator_get(NULL, "8058_l11");
	regulator_set_voltage(regulator, 2800000, 2800000);
	regulator_enable(regulator);

	if (up)
	{
		pr_info("********* IrDA : %s(up)!!\n", __func__);
		memcpy(signal, signal1, sizeof(signal1));
	}
	else
	{
		pr_info("********* IrDA : %s(down)!!\n", __func__);
		memcpy(signal, signal2, sizeof(signal2));
	}

	period = MICRO_SEC / signal[0];

	pr_info("duty : %u\n", period);


	gpio_direction_output(IRDA_EN, 1);
	local_irq_disable();


	for ( i =1 ; i < MAX_SIZE; ) {
		if (signal[i] == 0)
			break;
		enable_high(signal[i++], period);
		__udelay(signal[i++]*period);
	}

	local_irq_enable();

	pr_info("********* IrDA : %s send complited!!\n", __func__);


	gpio_direction_output(IRDA_EN, 0);

	regulator_disable(regulator);


}
EXPORT_SYMBOL(ir_test);
#endif

static void ir_remocon_work(struct work_struct *work)
{
	struct ir_remocon_data *data = container_of(work,
                struct ir_remocon_data, work);

	ir_remocon_send(data);
}

static void ir_remocon_work_test(struct work_struct *work)
{
	struct ir_remocon_data *data = container_of(work,
                struct ir_remocon_data, work);

	ir_remocon_send_test(data);
}

static ssize_t remocon_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	int i;
	unsigned int _data;

	for ( i = 0; i < MAX_SIZE; i++) {
		if (sscanf(buf++,"%u", &_data) == 1) {
			data->signal[i] = _data;
//			pr_info("%u,", data->signal[i]);
			while (_data > 0) {
				buf++;
				_data /= 10;
			}
		} else {
			data->signal[i] = 0;
			break;
		}
	}

	data->gpio = IRDA_OUT;
	data->pwr_en = IRDA_EN;

	if (!work_pending(&data->work))
		schedule_work(&data->work);

	return size;
}

static ssize_t remocon_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	int i;
	char *bufp = buf;

	for ( i = 0; i < MAX_SIZE; i++) {
		if (data->signal[i] == 0)
			break;
		else	{
			bufp += sprintf(bufp, "%u,", data->signal[i]);
			pr_info("%u,", data->signal[i]);
		}
	}
	return strlen(buf);
}

static DEVICE_ATTR(ir_send, S_IRWXUGO, remocon_show, remocon_store);
static DEVICE_ATTR(ir_send_test, S_IRWXUGO, NULL, remocon_store);

static ssize_t check_ir_show(struct device *dev, struct device_attribute *attr, char *buf)
{
//	struct ir_remocon_data *data = dev_get_drvdata(dev);

	int _data = 1;

	return sprintf(buf, "%d\n", _data);
}

static DEVICE_ATTR(check_ir, S_IRWXUGO, check_ir_show, NULL);

static int __devinit ir_remocon_probe(struct platform_device *pdev)
{

	pr_info("********* IrDA : %s start!\n", __func__);

	struct ir_remocon_data *data = pdev->dev.platform_data;
	struct ir_remocon_data *data1 = pdev->dev.platform_data;	

	struct device *ir_remocon_dev;
	struct device *ir_remocon_dev_test;
	int error;

	data = kzalloc(sizeof(struct ir_remocon_data), GFP_KERNEL);
	if (NULL == data) {
		pr_err("Failed to allocate\n");
		error = -ENOMEM;
		goto err_free_mem;
	}

	data1 = kzalloc(sizeof(struct ir_remocon_data), GFP_KERNEL);
	if (NULL == data1) {
		pr_err("Failed to data1 allocate %s\n", __func__);
		error = -ENOMEM;
		goto err_free_mem;
	}
	

	mutex_init(&data->mutex);
	INIT_WORK(&data->work, ir_remocon_work);
	INIT_WORK(&data1->work, ir_remocon_work_test);

	error = gpio_init(data);
	if(error)
		pr_err("Failed to request gpio");

	ir_remocon_dev = device_create(sec_class, NULL, 0, data, "sec_ir");
	ir_remocon_dev_test = device_create(sec_class, NULL, 0, data1, "sec_ir_test");

	if (IS_ERR(ir_remocon_dev))
		pr_err("Failed to create device\n");

	if (IS_ERR(ir_remocon_dev_test))
		pr_err("Failed to create ir_remocon_dev_test device\n");

	if (device_create_file(ir_remocon_dev, &dev_attr_ir_send) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_ir_send.attr.name);

	if (device_create_file(ir_remocon_dev_test, &dev_attr_ir_send_test) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_ir_send_test.attr.name);


	if (device_create_file(ir_remocon_dev, &dev_attr_check_ir) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_check_ir.attr.name);

	return 0;

err_free_mem:
	kfree(data);
	kfree(data1);	
	return error;

}

static int __devexit ir_remocon_remove(struct platform_device *pdev)
{
	//    struct dock_keyboard_data *pdata = pdev->dev.platform_data;
	return 0;
}

#ifdef CONFIG_PM
static int ir_remocon_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}
static int ir_remocon_resume(struct platform_device *pdev)
{
	return 0;
}
#endif


static struct platform_driver ir_remocon_device_driver =
{
	.probe		= ir_remocon_probe,
	.remove	= __devexit_p(ir_remocon_remove),
#ifdef CONFIG_PM
	.suspend	= ir_remocon_suspend,
	.resume	= ir_remocon_resume,
#endif
	.driver		=
	{
		.name	= "ir_rc",
		.owner	= THIS_MODULE,
	}
};

static int __init ir_remocon_init(void)
{
	return platform_driver_register(&ir_remocon_device_driver);
}

static void __exit ir_remocon_exit(void)
{
	platform_driver_unregister(&ir_remocon_device_driver);
}

module_init(ir_remocon_init);
module_exit(ir_remocon_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SEC IR remote controller");
