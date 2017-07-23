/*
 * LIRC base driver
 *
 * by by Artur Lipowski <alipowski@interia.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
#include <linux/autoconf.h>
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/completion.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/unistd.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
#include <linux/kthread.h>
#endif
#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include "drivers/kcompat.h"
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
#include <media/lirc.h>
#include <media/lirc_dev.h>
#else
#include "drivers/lirc.h"
#include "lirc_dev.h"
#endif

static int debug;
#define dprintk(fmt, args...)					\
	do {							\
		if (debug)					\
			printk(KERN_DEBUG fmt, ## args);	\
	} while (0)

#define IRCTL_DEV_NAME	"BaseRemoteCtl"
#define NOPLUG		-1
#define LOGHEAD		"lirc_dev (%s[%d]): "

static dev_t lirc_base_dev;

struct irctl {
	struct lirc_driver d;
	int attached;
	int open;

	struct mutex irctl_lock;
	struct lirc_buffer *buf;
	unsigned int chunk_size;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
	int tpid;
	struct completion *t_notify;
	struct completion *t_notify2;
	int shutdown;
#else
	struct task_struct *task;
#endif
	long jiffies_to_wait;
};

static DEFINE_MUTEX(lirc_dev_lock);

static struct irctl *irctls[MAX_IRCTL_DEVICES];
static struct cdev cdevs[MAX_IRCTL_DEVICES];

/* Only used for sysfs but defined to void otherwise */
static struct class *lirc_class;

/*  helper function
 *  initializes the irctl structure
 */
static void lirc_irctl_init(struct irctl *ir)
{
	mutex_init(&ir->irctl_lock);
	ir->d.minor = NOPLUG;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
	ir->tpid = -1;
#endif
}

static void lirc_irctl_cleanup(struct irctl *ir)
{
	dprintk(LOGHEAD "cleaning up\n", ir->d.name, ir->d.minor);

	device_destroy(lirc_class, MKDEV(MAJOR(lirc_base_dev), ir->d.minor));

	if (ir->buf != ir->d.rbuf) {
		lirc_buffer_free(ir->buf);
		kfree(ir->buf);
	}
	ir->buf = NULL;
}

/*  helper function
 *  reads key codes from driver and puts them into buffer
 *  returns 0 on success
 */
static int lirc_add_to_buf(struct irctl *ir)
{
	if (ir->d.add_to_buf) {
		int res = -ENODATA;
		int got_data = 0;

		/*
		 * service the device as long as it is returning
		 * data and we have space
		 */
get_data:
		res = ir->d.add_to_buf(ir->d.data, ir->buf);
		if (res == 0) {
			got_data++;
			goto get_data;
		}

		if (res == -ENODEV)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
			ir->shutdown = 1;
#else
			kthread_stop(ir->task);
#endif

		return got_data ? 0 : res;
	}

	return 0;
}

/* main function of the polling thread */
static int lirc_thread(void *irctl)
{
	struct irctl *ir = irctl;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
	/*
	 * This thread doesn't need any user-level access, so get rid
	 * of all our resources
	 */
	daemonize("lirc_dev");

	if (ir->t_notify != NULL)
		complete(ir->t_notify);

#endif
	dprintk(LOGHEAD "poll thread started\n", ir->d.name, ir->d.minor);

	do {
		if (ir->open) {
			if (ir->jiffies_to_wait) {
				set_current_state(TASK_INTERRUPTIBLE);
				schedule_timeout(ir->jiffies_to_wait);
			}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
			if (ir->shutdown)
#else
			if (kthread_should_stop())
#endif
				break;
			if (!lirc_add_to_buf(ir))
				wake_up_interruptible(&ir->buf->wait_poll);
		} else {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
	} while (!ir->shutdown);

	if (ir->t_notify2 != NULL)
		wait_for_completion(ir->t_notify2);

	ir->tpid = -1;
	if (ir->t_notify != NULL)
		complete(ir->t_notify);
#else
	} while (!kthread_should_stop());
#endif

	dprintk(LOGHEAD "poll thread ended\n", ir->d.name, ir->d.minor);

	return 0;
}

static struct file_operations lirc_dev_fops = {
	.owner		= THIS_MODULE,
	.read		= lirc_dev_fop_read,
	.write		= lirc_dev_fop_write,
	.poll		= lirc_dev_fop_poll,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
	.ioctl		= lirc_dev_fop_ioctl,
#else
	.unlocked_ioctl	= lirc_dev_fop_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= lirc_dev_fop_ioctl,
#endif
#endif
	.open		= lirc_dev_fop_open,
	.release	= lirc_dev_fop_close,
};

static int lirc_cdev_add(struct irctl *ir)
{
	int retval;
	struct lirc_driver *d = &ir->d;
	struct cdev *cdev = &cdevs[d->minor];
 
	if (d->fops) {
		cdev_init(cdev, d->fops);
		cdev->owner = d->owner;
	} else {
		cdev_init(cdev, &lirc_dev_fops);
		cdev->owner = THIS_MODULE;
	}
	retval = kobject_set_name(&cdev->kobj, "lirc%d", d->minor);
	if (retval)
		return retval;

	retval = cdev_add(cdev, MKDEV(MAJOR(lirc_base_dev), d->minor), 1);
	if (retval)
		kobject_put(&cdev->kobj);

	return retval;
}

int lirc_register_driver(struct lirc_driver *d)
{
	struct irctl *ir;
	int minor;
	int bytes_in_key;
	unsigned int chunk_size;
	unsigned int buffer_size;
	int err;
	DECLARE_COMPLETION(tn);

	if (!d) {
		printk(KERN_ERR "lirc_dev: lirc_register_driver: "
		       "driver pointer must be not NULL!\n");
		err = -EBADRQC;
		goto out;
	}

	if (!d->dev) {
		printk(KERN_ERR "%s: dev pointer not filled in!\n", __func__);
		err = -EINVAL;
		goto out;
	}

	if (MAX_IRCTL_DEVICES <= d->minor) {
		printk(KERN_ERR "lirc_dev: lirc_register_driver: "
		       "\"minor\" must be between 0 and %d (%d)!\n",
		       MAX_IRCTL_DEVICES-1, d->minor);
		err = -EBADRQC;
		goto out;
	}

	if (1 > d->code_length || (BUFLEN * 8) < d->code_length) {
		printk(KERN_ERR "lirc_dev: lirc_register_driver: "
		       "code length in bits for minor (%d) "
		       "must be less than %d!\n",
		       d->minor, BUFLEN * 8);
		err = -EBADRQC;
		goto out;
	}

	printk(KERN_INFO "lirc_dev: lirc_register_driver: sample_rate: %d\n",
		d->sample_rate);
	if (d->sample_rate) {
		if (2 > d->sample_rate || HZ < d->sample_rate) {
			printk(KERN_ERR "lirc_dev: lirc_register_driver: "
			       "sample_rate must be between 2 and %d!\n", HZ);
			err = -EBADRQC;
			goto out;
		}
		if (!d->add_to_buf) {
			printk(KERN_ERR "lirc_dev: lirc_register_driver: "
			       "add_to_buf cannot be NULL when "
			       "sample_rate is set\n");
			err = -EBADRQC;
			goto out;
		}
	} else if (!(d->fops && d->fops->read) && !d->rbuf) {
		printk(KERN_ERR "lirc_dev: lirc_register_driver: "
		       "fops->read and rbuf "
		       "cannot all be NULL!\n");
		err = -EBADRQC;
		goto out;
	} else if (!d->rbuf) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
		if (!(d->fops && d->fops->read && d->fops->poll &&
		      d->fops->ioctl)) {
#else
		if (!(d->fops && d->fops->read && d->fops->poll &&
		      d->fops->unlocked_ioctl)) {
#endif
			printk(KERN_ERR "lirc_dev: lirc_register_driver: "
			       "neither read, poll nor ioctl can be NULL!\n");
			err = -EBADRQC;
			goto out;
		}
	}

	mutex_lock(&lirc_dev_lock);

	minor = d->minor;

	if (minor < 0) {
		/* find first free slot for driver */
		for (minor = 0; minor < MAX_IRCTL_DEVICES; minor++)
			if (!irctls[minor])
				break;
		if (MAX_IRCTL_DEVICES == minor) {
			printk(KERN_ERR "lirc_dev: lirc_register_driver: "
			       "no free slots for drivers!\n");
			err = -ENOMEM;
			goto out_lock;
		}
	} else if (irctls[minor]) {
		printk(KERN_ERR "lirc_dev: lirc_register_driver: "
		       "minor (%d) just registered!\n", minor);
		err = -EBUSY;
		goto out_lock;
	}

	ir = kzalloc(sizeof(struct irctl), GFP_KERNEL);
	if (!ir) {
		err = -ENOMEM;
		goto out_lock;
	}
	lirc_irctl_init(ir);
	irctls[minor] = ir;
	d->minor = minor;

	if (d->sample_rate) {
		ir->jiffies_to_wait = HZ / d->sample_rate;
	} else {
		/* it means - wait for external event in task queue */
		ir->jiffies_to_wait = 0;
	}

	/* some safety check 8-) */
	d->name[sizeof(d->name)-1] = '\0';

	bytes_in_key = BITS_TO_LONGS(d->code_length) +
			(d->code_length % 8 ? 1 : 0);
	buffer_size = d->buffer_size ? d->buffer_size : BUFLEN / bytes_in_key;
	chunk_size  = d->chunk_size  ? d->chunk_size  : bytes_in_key;

	if (d->rbuf) {
		ir->buf = d->rbuf;
	} else {
		ir->buf = kmalloc(sizeof(struct lirc_buffer), GFP_KERNEL);
		if (!ir->buf) {
			err = -ENOMEM;
			goto out_lock;
		}
		err = lirc_buffer_init(ir->buf, chunk_size, buffer_size);
		if (err) {
			kfree(ir->buf);
			goto out_lock;
		}
	}
	ir->chunk_size = ir->buf->chunk_size;

	if (d->features == 0)
		d->features = LIRC_CAN_REC_LIRCCODE;

	ir->d = *d;

	device_create(lirc_class, ir->d.dev,
		      MKDEV(MAJOR(lirc_base_dev), ir->d.minor), NULL,
		      "lirc%u", ir->d.minor);

	if (d->sample_rate) {
		/* try to fire up polling thread */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
		ir->t_notify = &tn;
		ir->tpid = kernel_thread(lirc_thread, (void *)ir, 0);
		if (ir->tpid < 0) {
#else
		ir->task = kthread_run(lirc_thread, (void *)ir, "lirc_dev");
		if (IS_ERR(ir->task)) {
#endif
			printk(KERN_ERR "lirc_dev: lirc_register_driver: "
			       "cannot run poll thread for minor = %d\n",
			       d->minor);
			err = -ECHILD;
			goto out_sysfs;
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
		wait_for_completion(&tn);
		ir->t_notify = NULL;
#endif
	}

	err = lirc_cdev_add(ir);
	if (err)
		goto out_sysfs;

	ir->attached = 1;
	mutex_unlock(&lirc_dev_lock);

	dprintk("lirc_dev: driver %s registered at minor number = %d\n",
		ir->d.name, ir->d.minor);
	return minor;

out_sysfs:
	device_destroy(lirc_class, MKDEV(MAJOR(lirc_base_dev), ir->d.minor));
out_lock:
	mutex_unlock(&lirc_dev_lock);
out:
	return err;
}
EXPORT_SYMBOL(lirc_register_driver);

int lirc_unregister_driver(int minor)
{
	struct irctl *ir;
	struct cdev *cdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
	DECLARE_COMPLETION(tn);
	DECLARE_COMPLETION(tn2);
#endif

	if (minor < 0 || minor >= MAX_IRCTL_DEVICES) {
		printk(KERN_ERR "lirc_dev: lirc_unregister_driver: "
		       "\"minor\" must be between 0 and %d!\n",
		       MAX_IRCTL_DEVICES-1);
		return -EBADRQC;
	}

	ir = irctls[minor];

	if (!ir) {
		printk(KERN_ERR "lirc_dev: %s: failed to get irctl struct for "
		       "minor %d!\n", __func__, minor);
		return -ENOENT;
	}

	cdev = &cdevs[minor];

	mutex_lock(&lirc_dev_lock);

	if (ir->d.minor != minor) {
		printk(KERN_ERR "lirc_dev: lirc_unregister_driver: "
		       "minor (%d) device not registered!", minor);
		mutex_unlock(&lirc_dev_lock);
		return -ENOENT;
	}

	/* end up polling thread */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
	if (ir->tpid >= 0) {
		struct task_struct *p;

		ir->t_notify = &tn;
		ir->t_notify2 = &tn2;
		ir->shutdown = 1;

		p = find_task_by_pid(ir->tpid);
		wake_up_process(p);

		complete(&tn2);
		wait_for_completion(&tn);
		ir->t_notify = NULL;
		ir->t_notify2 = NULL;
	}
#else /* kernel >= 2.6.23 */
	if (ir->task)
		kthread_stop(ir->task);
#endif

	dprintk("lirc_dev: driver %s unregistered from minor number = %d\n",
		ir->d.name, ir->d.minor);

	ir->attached = 0;
	if (ir->open) {
		dprintk(LOGHEAD "releasing opened driver\n",
			ir->d.name, ir->d.minor);
		wake_up_interruptible(&ir->buf->wait_poll);
		mutex_lock(&ir->irctl_lock);
		ir->d.set_use_dec(ir->d.data);
		module_put(cdev->owner);
		mutex_unlock(&ir->irctl_lock);
	} else {
		lirc_irctl_cleanup(ir);
		cdev_del(cdev);
		kfree(ir);
		irctls[minor] = NULL;
	}

	mutex_unlock(&lirc_dev_lock);

	return 0;
}
EXPORT_SYMBOL(lirc_unregister_driver);

int lirc_dev_fop_open(struct inode *inode, struct file *file)
{
	struct irctl *ir;
	struct cdev *cdev;
	int retval = 0;

	if (iminor(inode) >= MAX_IRCTL_DEVICES || !irctls[iminor(inode)]) {
		dprintk("lirc_dev [%d]: open result = -ENODEV\n",
			iminor(inode));
		return -ENODEV;
	}

	if (mutex_lock_interruptible(&lirc_dev_lock))
		return -ERESTARTSYS;

	ir = irctls[iminor(inode)];
	if (!ir) {
		retval = -ENODEV;
		goto error;
	}

	dprintk(LOGHEAD "open called\n", ir->d.name, ir->d.minor);

	if (ir->d.minor == NOPLUG) {
		retval = -ENODEV;
		goto error;
	}

	if (ir->open) {
		retval = -EBUSY;
		goto error;
	}

	cdev = &cdevs[iminor(inode)];
	if (try_module_get(cdev->owner)) {
		ir->open++;
		retval = ir->d.set_use_inc(ir->d.data);

		if (retval) {
			module_put(cdev->owner);
			ir->open--;
		} else {
			lirc_buffer_clear(ir->buf);
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
		if (ir->tpid >= 0) {
			struct task_struct *p;

			p = find_task_by_pid(ir->tpid);
			wake_up_process(p);
		}
#else
		if (ir->task)
			wake_up_process(ir->task);
#endif
	} else {
		retval = -ENODEV;
	}

error:
	if (ir)
		dprintk(LOGHEAD "open result = %d\n", ir->d.name, ir->d.minor,
			retval);

	mutex_unlock(&lirc_dev_lock);

	nonseekable_open(inode, file);

	return retval;
}
EXPORT_SYMBOL(lirc_dev_fop_open);

int lirc_dev_fop_close(struct inode *inode, struct file *file)
{
	struct irctl *ir = irctls[iminor(inode)];
	struct cdev *cdev = &cdevs[iminor(inode)];

	if (!ir) {
		printk(KERN_ERR "%s: called with invalid irctl\n", __func__);
		return -EINVAL;
	}

	dprintk(LOGHEAD "close called\n", ir->d.name, ir->d.minor);

	WARN_ON(mutex_lock_interruptible(&lirc_dev_lock));

	ir->open--;
	if (ir->attached) {
		ir->d.set_use_dec(ir->d.data);
		module_put(cdev->owner);
	} else {
		lirc_irctl_cleanup(ir);
		cdev_del(cdev);
		irctls[ir->d.minor] = NULL;
		kfree(ir);
	}

	mutex_unlock(&lirc_dev_lock);

	return 0;
}
EXPORT_SYMBOL(lirc_dev_fop_close);

unsigned int lirc_dev_fop_poll(struct file *file, poll_table *wait)
{
	struct irctl *ir = irctls[iminor(file->f_dentry->d_inode)];
	unsigned int ret;

	if (!ir) {
		printk(KERN_ERR "%s: called with invalid irctl\n", __func__);
		return POLLERR;
	}

	dprintk(LOGHEAD "poll called\n", ir->d.name, ir->d.minor);

	if (!ir->attached)
		return POLLERR;

	poll_wait(file, &ir->buf->wait_poll, wait);

	if (ir->buf)
		if (lirc_buffer_empty(ir->buf))
			ret = 0;
		else
			ret = POLLIN | POLLRDNORM;
	else
		ret = POLLERR;

	dprintk(LOGHEAD "poll result = %s\n",
		ir->d.name, ir->d.minor,
		lirc_buffer_empty(ir->buf) ? "0" : "POLLIN|POLLRDNORM");

	return ret;
}
EXPORT_SYMBOL(lirc_dev_fop_poll);

/*
 *
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
int lirc_dev_fop_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
#else
long lirc_dev_fop_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	__u32 mode;
	int result = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
	struct irctl *ir = irctls[iminor(inode)];
#else
	struct irctl *ir = irctls[iminor(file->f_dentry->d_inode)];
#endif
	if (!ir) {
		printk(KERN_ERR "lirc_dev: %s: no irctl found!\n", __func__);
		return -ENODEV;
	}

	dprintk(LOGHEAD "ioctl called (0x%x)\n",
		ir->d.name, ir->d.minor, cmd);

	if (ir->d.minor == NOPLUG || !ir->attached) {
		dprintk(LOGHEAD "ioctl result = -ENODEV\n",
			ir->d.name, ir->d.minor);
		return -ENODEV;
	}

	mutex_lock(&ir->irctl_lock);

	switch (cmd) {
	case LIRC_GET_FEATURES:
		result = put_user(ir->d.features, (__u32 *)arg);
		break;
	case LIRC_GET_REC_MODE:
		if (!(ir->d.features & LIRC_CAN_REC_MASK)) {
			result = -ENOSYS;
			break;
		}

		result = put_user(LIRC_REC2MODE
				  (ir->d.features & LIRC_CAN_REC_MASK),
				  (__u32 *)arg);
		break;
	case LIRC_SET_REC_MODE:
		if (!(ir->d.features & LIRC_CAN_REC_MASK)) {
			result = -ENOSYS;
			break;
		}

		result = get_user(mode, (__u32 *)arg);
		if (!result && !(LIRC_MODE2REC(mode) & ir->d.features))
			result = -EINVAL;
		/*
		 * FIXME: We should actually set the mode somehow but
		 * for now, lirc_serial doesn't support mode changing either
		 */
		break;
	case LIRC_GET_LENGTH:
		result = put_user(ir->d.code_length, (__u32 *) arg);
		break;
	case LIRC_GET_MIN_TIMEOUT:
		if (!(ir->d.features & LIRC_CAN_SET_REC_TIMEOUT) ||
		    ir->d.min_timeout == 0) {
			result = -ENOSYS;
			break;
		}

		result = put_user(ir->d.min_timeout, (__u32 *) arg);
		break;
	case LIRC_GET_MAX_TIMEOUT:
		if (!(ir->d.features & LIRC_CAN_SET_REC_TIMEOUT) ||
		    ir->d.max_timeout == 0) {
			result = -ENOSYS;
			break;
		}

		result = put_user(ir->d.max_timeout, (__u32 *) arg);
		break;
	default:
		result = -EINVAL;
	}

	dprintk(LOGHEAD "ioctl result = %d\n",
		ir->d.name, ir->d.minor, result);

	mutex_unlock(&ir->irctl_lock);

	return result;
}
EXPORT_SYMBOL(lirc_dev_fop_ioctl);

ssize_t lirc_dev_fop_read(struct file *file, char __user *buffer,
			  size_t length, loff_t *ppos)
{
	struct irctl *ir = irctls[iminor(file->f_dentry->d_inode)];
	unsigned char *buf;
	int ret = 0, written = 0;
	DECLARE_WAITQUEUE(wait, current);

	if (!ir) {
		printk(KERN_ERR "%s: called with invalid irctl\n", __func__);
		return -ENODEV;
	}

	dprintk(LOGHEAD "read called\n", ir->d.name, ir->d.minor);

	buf = kzalloc(ir->chunk_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (mutex_lock_interruptible(&ir->irctl_lock)) {
		ret = -ERESTARTSYS;
		goto out_unlocked;
	}
	if (!ir->attached) {
		ret = -ENODEV;
		goto out_locked;
	}

	if (length % ir->chunk_size) {
		ret = -EINVAL;
		goto out_locked;
	}

	/*
	 * we add ourselves to the task queue before buffer check
	 * to avoid losing scan code (in case when queue is awaken somewhere
	 * between while condition checking and scheduling)
	 */
	add_wait_queue(&ir->buf->wait_poll, &wait);
	set_current_state(TASK_INTERRUPTIBLE);

	/*
	 * while we didn't provide 'length' bytes, device is opened in blocking
	 * mode and 'copy_to_user' is happy, wait for data.
	 */
	while (written < length && ret == 0) {
		if (lirc_buffer_empty(ir->buf)) {
			/* According to the read(2) man page, 'written' can be
			 * returned as less than 'length', instead of blocking
			 * again, returning -EWOULDBLOCK, or returning
			 * -ERESTARTSYS */
			if (written)
				break;
			if (file->f_flags & O_NONBLOCK) {
				ret = -EWOULDBLOCK;
				break;
			}
			if (signal_pending(current)) {
				ret = -ERESTARTSYS;
				break;
			}

			mutex_unlock(&ir->irctl_lock);
			schedule();
			set_current_state(TASK_INTERRUPTIBLE);

			if (mutex_lock_interruptible(&ir->irctl_lock)) {
				ret = -ERESTARTSYS;
				remove_wait_queue(&ir->buf->wait_poll, &wait);
				set_current_state(TASK_RUNNING);
				goto out_unlocked;
			}

			if (!ir->attached) {
				ret = -ENODEV;
				break;
			}
		} else {
			lirc_buffer_read(ir->buf, buf);
			ret = copy_to_user((void *)buffer+written, buf,
					   ir->buf->chunk_size);
			if (!ret)
				written += ir->buf->chunk_size;
			else
				ret = -EFAULT;
		}
	}

	remove_wait_queue(&ir->buf->wait_poll, &wait);
	set_current_state(TASK_RUNNING);

out_locked:
	mutex_unlock(&ir->irctl_lock);

out_unlocked:
	kfree(buf);
	dprintk(LOGHEAD "read result = %s (%d)\n",
		ir->d.name, ir->d.minor, ret ? "<fail>" : "OK", ret);

	return ret ? ret : written;
}
EXPORT_SYMBOL(lirc_dev_fop_read);


void *lirc_get_pdata(struct file *file)
{
	void *data = NULL;

	if (file && file->f_dentry && file->f_dentry->d_inode &&
	    file->f_dentry->d_inode->i_rdev) {
		struct irctl *ir;
		ir = irctls[iminor(file->f_dentry->d_inode)];
		data = ir->d.data;
	}

	return data;
}
EXPORT_SYMBOL(lirc_get_pdata);


ssize_t lirc_dev_fop_write(struct file *file, const char __user *buffer,
			   size_t length, loff_t *ppos)
{
	struct irctl *ir = irctls[iminor(file->f_dentry->d_inode)];

	if (!ir) {
		printk(KERN_ERR "%s: called with invalid irctl\n", __func__);
		return -ENODEV;
	}

	dprintk(LOGHEAD "write called\n", ir->d.name, ir->d.minor);

	if (!ir->attached)
		return -ENODEV;

	return -EINVAL;
}
EXPORT_SYMBOL(lirc_dev_fop_write);


static int __init lirc_dev_init(void)
{
	int retval;

	lirc_class = class_create(THIS_MODULE, "lirc");
	if (IS_ERR(lirc_class)) {
		retval = PTR_ERR(lirc_class);
		printk(KERN_ERR "lirc_dev: class_create failed\n");
		goto error;
	}

	retval = alloc_chrdev_region(&lirc_base_dev, 0, MAX_IRCTL_DEVICES,
				     IRCTL_DEV_NAME);
	if (retval) {
		class_destroy(lirc_class);
		printk(KERN_ERR "lirc_dev: alloc_chrdev_region failed\n");
		goto error;
	}

	printk(KERN_INFO "lirc_dev: IR Remote Control driver registered, "
	       "major %d \n", MAJOR(lirc_base_dev));

error:
	return retval;
}

static void __exit lirc_dev_exit(void)
{
	class_destroy(lirc_class);
	unregister_chrdev_region(lirc_base_dev, MAX_IRCTL_DEVICES);
	dprintk("lirc_dev: module unloaded\n");
}

module_init(lirc_dev_init);
module_exit(lirc_dev_exit);

MODULE_DESCRIPTION("LIRC base driver module");
MODULE_AUTHOR("Artur Lipowski");
MODULE_LICENSE("GPL");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Enable debugging messages");
