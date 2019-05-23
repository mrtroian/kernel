#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/wait.h>

#define MODULE_NAME "cdm_module"
#define DEVICE_NAME "cdm_device"
#define MINOR_DEVICES 3
#define BUFFSIZE	1024
#define TIMEOUT		10000
#define IOCTL_BFCL	_IO(1,0)
#define IOCTL_UNDO	_IO(2,0)
#define IOCTL_FRSP	_IOR(3,0,int*)
#define IOCTL_ECHO	_IO(0x54, 0x01)
#define BUFFER(n)	(buffers[n])
#define READ_P(n)	(read_a[n])
#define CURR_P(n)	(curr[n])
#define ITER(m, n)	(buffers[m][n])
#define DEVICES(n)	(cdm_devices[n])
#define MSG_LEN(n)	(device_meta[n] & 0x00000fff)
#define MSG_DEL(n)	(device_meta[n] &= 0xfffff000)
#define REG_DEV(n)	(files[n])
#define DEV_NUM(n)	(get_device_number(n))
#define IDENTIFY(d)	(d->f_inode->i_rdev)
#define MSG_SET(m, n)	(device_meta[m] |= (n & 0xfff))
#define BUFF_EMPTY(n) (FREE_SPACE(n) == BUFFSIZE)
#define FREE_SPACE(n) (BUFFSIZE - (int)(CURR_P(n) - BUFFER(n)))
#define WAIT_FOR_READ(cond)	(wait_event_interruptible_timeout(rqueue, cond, TIMEOUT));
#define WAIT_FOR_WRITE(cond)	(wait_event_interruptible_timeout(wqueue, cond, TIMEOUT));


MODULE_AUTHOR("Kyrylo Troian");
MODULE_LICENSE("GPL v2");

typedef struct file			file_t;
typedef struct inode		node_t;
typedef struct cdm_device_s	cdm_t;

static int		undo(int devnum);
static int		buffcl(int devnum);
static int		get_device_number(file_t *filep);
static int		dev_open(node_t *inodep, file_t *filep);
static int		dev_release(node_t *inodep, file_t *filep);
static long		dev_ioctl(file_t *filep, unsigned int cmd, unsigned long arg);
static ssize_t	dev_read(file_t *filep, char *buff, size_t length, loff_t *offset);
static ssize_t	dev_write(file_t *filep, const char *buff, size_t length, loff_t *offset);


static int		major_number;
static int		device_meta[MINOR_DEVICES] = { 0 };
static char		buffers[MINOR_DEVICES][BUFFSIZE];
static char		*read_a[MINOR_DEVICES];
static char		*curr[MINOR_DEVICES];
static char		*sub_dev[MINOR_DEVICES] = { "cdm.0", "cdm.1", "cdm.2" };
static dev_t	files[MINOR_DEVICES] = { 0 };
static struct	class *deviceClass = NULL;
static struct	device *cdm_devices[MINOR_DEVICES];
static DECLARE_WAIT_QUEUE_HEAD(wqueue);
static DECLARE_WAIT_QUEUE_HEAD(rqueue);
static DEFINE_SPINLOCK(lock);

static struct file_operations fops =
{
	.open			= dev_open,
	.release 		= dev_release,
	.read			= dev_read,
	.write			= dev_write,
	.unlocked_ioctl	= dev_ioctl,
};


static int dev_open(node_t *inodep, file_t *filep)
{
	int devnum;

	devnum = DEV_NUM(filep);
	READ_P(devnum) = BUFFER(devnum);
	pr_info("cdm.%d: Device has been opened.\n", devnum);

	return 0;
}

static int dev_release(node_t *inodep, file_t *filep)
{
	int devnum;

	devnum = DEV_NUM(filep);
	pr_info("cdm.%d: Device closed.\n", devnum);

	return 0;
}

static ssize_t dev_write(file_t *filep, const char *buff, size_t length, loff_t *offset)
{
	int i;
	int devnum;

	i = 0;
	devnum = DEV_NUM(filep);
	spin_lock(&lock);

	while (length--) {
		if (!(FREE_SPACE(devnum) > 0)) {
			wake_up_interruptible(&rqueue);
			spin_unlock(&lock);
			WAIT_FOR_WRITE(FREE_SPACE(devnum) == BUFFSIZE);
			spin_lock(&lock);
		}
		*CURR_P(devnum)++ = buff[i++];
	}
	MSG_DEL(devnum);
	MSG_SET(devnum, i);
	pr_info("cdm.%d: %d bytes written.\n", devnum, i);
	READ_P(devnum) = BUFFER(devnum);

	spin_unlock(&lock);
	if (i > 0) {
		wake_up_interruptible(&rqueue);
	}
	return i;
}

static ssize_t dev_read(file_t *filep, char *buff, size_t length, loff_t *offset)
{
	int msglen;
	int devnum;
	int error;

	error = 0;
	msglen = 0;
	devnum = DEV_NUM(filep);
	spin_lock(&lock);

	if (BUFF_EMPTY(devnum)) {
		spin_unlock(&lock);
		WAIT_FOR_READ(!BUFF_EMPTY(devnum));
		spin_lock(&lock);
	}
	msglen = CURR_P(devnum) - BUFFER(devnum);
	error = copy_to_user(buff, READ_P(devnum), msglen);

	if (error != 0) {
		pr_err("cdm.%d: Error occured on reading.\n", devnum);
		spin_unlock(&lock);
		return -EFAULT;
	}
	READ_P(devnum) += msglen;
	buff += msglen;
	pr_info("cdm.%d: %d bytes read.\n", devnum, msglen);
	spin_unlock(&lock);
	wake_up_interruptible(&wqueue);

	return msglen;
}

static long dev_ioctl(file_t *filep, unsigned int cmd, unsigned long arg)
{
	int devnum;
	int val;

	devnum = DEV_NUM(filep);
	val = 0;

	if (cmd == IOCTL_UNDO) {
		undo(devnum);
		return 0;
	} else if (cmd == IOCTL_BFCL) {
		buffcl(devnum);
		return 0;
	} else if (cmd == IOCTL_FRSP) {
		val = FREE_SPACE(devnum);
		copy_to_user((int*) arg, &val, sizeof(val));
		return 0;
	}
	pr_err("cdm.%d: unknown IOCTL command: %u.", devnum, cmd);

	return -1;
}

static int get_device_number(file_t *filep)
{
	int i = 0;

	while (i < MINOR_DEVICES) {
		if (REG_DEV(i) == IDENTIFY(filep)) {
			return i;
		}
		i++;
	}
	pr_err("%s: Failed to identify device.\n", MODULE_NAME);

	return -1;
}

static int buffcl(int devnum)
{
	int i = 0;

	while (i < BUFFSIZE)
		ITER(devnum, i++) = 0;
	READ_P(devnum) = BUFFER(devnum);
	CURR_P(devnum) = BUFFER(devnum);
	MSG_DEL(devnum);
	pr_info("cdm.%d: buffer cleared.\n", devnum);
	wake_up_interruptible(&rqueue);
	wake_up_interruptible(&wqueue);

	return i;
}

static int undo(int devnum)
{
	char *to = CURR_P(devnum) - MSG_LEN(devnum);

	if (to < BUFFER(devnum))
		return -1;

	while (to <= CURR_P(devnum))
		*to++ = 0;
	CURR_P(devnum) -= MSG_LEN(devnum);
	MSG_DEL(devnum);

	return 0;
}

static int __init cdm_init(void)
{
	int devnum;

	devnum = 0;
	spin_lock_init(&lock);
	pr_info("%s: Module loaded in memory at 0x%lx\n", MODULE_NAME, (unsigned long) &cdm_init);
	major_number = register_chrdev(0, MODULE_NAME, &fops);

	if (major_number < 0) {
		pr_err("%s: Failed to register a major number.\n", MODULE_NAME);
		return major_number;
	}
	deviceClass = class_create(THIS_MODULE, MODULE_NAME);

	if (IS_ERR(deviceClass)) {
		pr_err("%s: Failed to create a device class.\n", MODULE_NAME);
		unregister_chrdev(major_number, DEVICE_NAME);
		return -1;
	}

	while (devnum < MINOR_DEVICES) {
		REG_DEV(devnum) = MKDEV(major_number, devnum);
		DEVICES(devnum) = device_create(deviceClass, NULL, REG_DEV(devnum), NULL, sub_dev[devnum]);

		if (IS_ERR(DEVICES(devnum))) {
			device_destroy(deviceClass, REG_DEV(devnum));
			class_destroy(deviceClass);
			unregister_chrdev(major_number, DEVICE_NAME);
			pr_err("%s: Failed to create device.\n", MODULE_NAME);
			return -1;
		}
		buffcl(devnum);
		devnum++;
	}

	return 0;
}

static void __exit cdm_exit(void)
{
	int devnum;

	devnum = 0;

	while (devnum < MINOR_DEVICES) {
		device_destroy(deviceClass, REG_DEV(devnum));
		devnum++;
	}
	class_unregister(deviceClass);
	class_destroy(deviceClass);
	unregister_chrdev(major_number, DEVICE_NAME);
	pr_info("%s: Module removed from 0x%lx\n", MODULE_NAME, (unsigned long) &cdm_init);
}

module_init(cdm_init);
module_exit(cdm_exit);