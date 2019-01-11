#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "pattern_ioctl.h"
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

#define SPIDEV_MAJOR			153	/* assigned */
#define N_SPI_MINORS			32  	/* ... up to 256 */

#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
				| SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)


static DECLARE_BITMAP(minors, N_SPI_MINORS);
uint8_t tx_buff[2];
Pattern P;

//SPI data structure
struct spidev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	unsigned		users;
	u8			*tx_buffer;
	u8			*rx_buffer;
	u32			speed_hz;
};

struct spidev_data *spidev_data_per;


static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");



struct spi_transfer	t = {
			.tx_buf		= tx_buff,
			.len		= 2,
			.speed_hz	= 500000,
			.cs_change = 1,
			.bits_per_word = 8,
			.delay_usecs = 0,
		};
	struct spi_message	m;



static void spidev_complete(void *arg)
{
	complete(arg);
}

int spi_led_transfer(uint8_t tx_buffer_addr, uint8_t tx_buffer_data)
{
	tx_buff[0] = tx_buffer_addr;
	tx_buff[1] = tx_buffer_data;
	
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	gpio_set_value(15,0);
	spi_sync(spidev_data_per->spi, &m);
	gpio_set_value(15,1);
	
	return 0;
} 

// Sync function

static ssize_t spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = spidev_complete;
	message->context = &done;

	spin_lock_irq(&spidev->spi_lock);
	if (spidev->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(spidev->spi, message);
	spin_unlock_irq(&spidev->spi_lock);

	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}
	return status;
}

static inline ssize_t spidev_sync_write(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= spidev->tx_buffer,
			.len		= len,
			.speed_hz	= spidev->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}


//Open file operation

static int spidev_open(struct inode *inodep, struct file *filep) {

	int i=1;
	//initialize GPIO pins for LED matrix SPI transfer

	//IO11
	gpio_request(72,"sys");
	gpio_request(44,"sys");
	gpio_request(24,"sys");

	gpio_set_value_cansleep(72,0);
	gpio_direction_output(44,1);
	gpio_direction_output(24,0);

	//IO12

	gpio_request(42,"sys");
	gpio_request(15,"sys");

	gpio_direction_output(42,0);
	gpio_direction_output(15,0);

	//IO13
	gpio_request(46,"sys");
	gpio_request(30,"sys");

	gpio_direction_output(46,1);
	gpio_direction_output(30,0);

	printk("All GPIOs required for SPI LED transfer initialized\n");
	
	// Setting up LED module functionality
	
	// Decode mode control regist
	spi_led_transfer(0x09,0x00);
	
	// Intensity
	spi_led_transfer(0x0A,0x09);
	
	// Scan limit
	spi_led_transfer(0x0B,0x07);
	
	// Shutdown control regist normal mode
	spi_led_transfer(0x0C,0x01);
	
	// Display test
	
	spi_led_transfer(0x0F,0x01);
	
	msleep(1000);
	
	spi_led_transfer(0x0F,0x00);
	
	// Clearing Display
	for(i = 1; i<9; i++)
	{
	spi_led_transfer(i,0x00);
	}
	return 0;

}

// IOCTL file operation 

static long spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	Pattern p;
	copy_from_user(&p, (Pattern *)arg, sizeof(Pattern));   // copying the input values from user space to kernel module 
	switch (cmd)
        {
        		case CONFIG:
        		{	
        			int i,j;
        			printk("IOCTL commands to pass patterns\n");
        			
					for (i=0; i<=9; i++)                                   // copying the 10 patterns in the per device structure's led array
					{
						for(j=0; j<=7; j++)
						{
							P.led[i][j] = p.led[i][j];
						}
					}
						
					break;
        		}
        		
            default:
            printk("\nWrong command, exiting driver");
        		return 0;
        }
    return 0;
    
}

// Write file operation

static ssize_t spidev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int P_1, P_2, P_3;
	int i=0;
	int rest = 0;
	char str[20];
	copy_from_user(str, (char *)buf, sizeof(str));   // copying the input values from user space to kernel module 
   printk("%s", str);
   sscanf(str, "%d %d %d %d", &P_1, &P_2, &P_3, &rest);
  // printk("From SPI_Driver: Printing Pattern numbers:%d %d %d\n", P_1, rest, P_2);
   
	// Printing Patterns
	
	for(i = 1; i<9; i++)
	{
		spi_led_transfer(i,P.led[P_1][i-1]);
	}
	msleep(rest);
	for(i = 1; i<9; i++)
	{
		spi_led_transfer(i,P.led[P_2][i-1]);
	}
	msleep(rest);
	for(i = 1; i<9; i++)
	{
		spi_led_transfer(i,P.led[P_3][i-1]);
	}
	msleep(rest);
	
	return 0;
}

// Release file operation

static int spidev_release(struct inode *inode, struct file *filp)
{
	int i =1;
	for(i = 1; i<9; i++)
	{
	spi_led_transfer(i,0x00);
	}

   // Freeing GPIOs
   
   // IO11	
	gpio_free(72);
	gpio_free(44);
	gpio_free(24);

	//IO12
	gpio_free(42);
	gpio_free(15);

	//IO13
	gpio_free(46);
	gpio_free(30);

	printk("SPI LED driver successfully released\n");
	return 0;
}


static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	.write =	spidev_write,
	//.read =		spidev_read,
	.unlocked_ioctl = spidev_ioctl,
	
	.open =		spidev_open,
	.release =	spidev_release,
	//.llseek =	no_llseek,
};

static struct class *spidev_class;


// PROBE
static int spidev_probe(struct spi_device *spi)
{
	//struct spidev_data	*spidev;
	int			status;
	unsigned long		minor;

	/* Allocate driver data */
	spidev_data_per = kzalloc(sizeof(*spidev_data_per), GFP_KERNEL);
	if (!spidev_data_per)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev_data_per->spi = spi;
	spin_lock_init(&spidev_data_per->spi_lock);
	mutex_init(&spidev_data_per->buf_lock);

	INIT_LIST_HEAD(&spidev_data_per->device_entry);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev_data_per->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(spidev_class, &spi->dev, spidev_data_per->devt,
				    spidev_data_per, "spidev%d.%d",
				    spi->master->bus_num, spi->chip_select);
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&spidev_data_per->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	spidev_data_per->speed_hz = spi->max_speed_hz;

	if (status == 0)
		spi_set_drvdata(spi, spidev_data_per);
	else
		kfree(spidev_data_per);

	return status;
}


// REMOVE
static int spidev_remove(struct spi_device *spi)
{
	spidev_data_per = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev_data_per->spi_lock);
	spidev_data_per->spi = NULL;
	spin_unlock_irq(&spidev_data_per->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&spidev_data_per->device_entry);
	device_destroy(spidev_class, spidev_data_per->devt);
	clear_bit(MINOR(spidev_data_per->devt), minors);
	if (spidev_data_per->users == 0)
		kfree(spidev_data_per);
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "rohm,dh2228fv" },
	{},
};


static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spidev",
		.owner =	THIS_MODULE,
		.of_match_table = of_match_ptr(spidev_dt_ids),
	},
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};


MODULE_DEVICE_TABLE(of, spidev_dt_ids);

//------------------------------------------------------------------------------------------------------------//

static int __init spidev_init(void)
{
	int status;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
	if (status < 0)
		return status;

	spidev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spidev_class)) {
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
		return PTR_ERR(spidev_class);
	}

	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
	}
	printk("SPI LED driver module installed\n");
	return status;
}
module_init(spidev_init);

static void __exit spidev_exit(void)
{
	spi_unregister_driver(&spidev_spi_driver);
	class_destroy(spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
	printk("SPI LED driver module removed\n");
}
module_exit(spidev_exit);

MODULE_AUTHOR("Oindrila & Niharika");
MODULE_DESCRIPTION("User mode SPI LED device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev");


