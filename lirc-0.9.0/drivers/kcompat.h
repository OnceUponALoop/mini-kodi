/*
 * Support header for building lirc drivers on kernels older than the
 * latest upstream kernel. As of this update, 2.6.18 is the oldest
 * kernel officially supported any longer. For older kernels, just
 * rewind the scm history a ways. We can't hope to significantly improve
 * things if we have to support crusty old kernels only a tiny minority
 * of people still run, some of these source files are/were really nasty
 * and hard to follow due to the proliferation of #if LINUX_VERSION_CODE
 * bits...
 */

#ifndef _KCOMPAT_H
#define _KCOMPAT_H

#include <linux/version.h>

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 34)
#define usb_alloc_coherent usb_buffer_alloc
#define usb_free_coherent usb_buffer_free
#endif

#define LIRC_THIS_MODULE(x)

#include <linux/device.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)

#define lirc_device_create(cs, parent, dev, drvdata, fmt, args...) \
	class_device_create(cs, NULL, dev, parent, fmt, ## args)

#else /* >= 2.6.26 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)

#define lirc_device_create(cs, parent, dev, drvdata, fmt, args...) \
	device_create(cs, parent, dev, fmt, ## args)

#else /* >= 2.6.27 */

#define lirc_device_create device_create

#endif /* >= 2.6.27 */

#endif /* >= 2.6.26 */

#define LIRC_DEVFS_PREFIX

typedef struct class lirc_class_t;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)

#define lirc_device_destroy class_device_destroy

#else

#define lirc_device_destroy device_destroy

#endif

#ifndef LIRC_DEVFS_PREFIX
#define LIRC_DEVFS_PREFIX "usb/"
#endif

#include <linux/moduleparam.h>

#include <linux/interrupt.h>
#ifndef IRQ_RETVAL
typedef void irqreturn_t;
#define IRQ_NONE
#define IRQ_HANDLED
#define IRQ_RETVAL(x)
#endif

#ifndef MOD_IN_USE
#ifdef CONFIG_MODULE_UNLOAD
#define MOD_IN_USE module_refcount(THIS_MODULE)
#else
#error "LIRC modules currently require"
#error "  'Loadable module support  --->  Module unloading'"
#error "to be enabled in the kernel"
#endif
#endif

/*************************** I2C specific *****************************/
#include <linux/i2c.h>

/* removed in 2.6.14 */
#ifndef I2C_ALGO_BIT
#   define I2C_ALGO_BIT 0
#endif

/* removed in 2.6.16 */
#ifndef I2C_DRIVERID_EXP3
#  define I2C_DRIVERID_EXP3 0xf003
#endif

/*************************** USB specific *****************************/
#include <linux/usb.h>

/* removed in 2.6.14 */
#ifndef URB_ASYNC_UNLINK
#define URB_ASYNC_UNLINK 0
#endif

/*************************** pm_wakeup.h ******************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
static inline void device_set_wakeup_capable(struct device *dev, int val) {}
#endif /* kernel < 2.6.27 */

/*************************** interrupt.h ******************************/
/* added in 2.6.18, old defines removed in 2.6.24 */
#ifndef IRQF_DISABLED
#define IRQF_DISABLED SA_INTERRUPT
#endif
#ifndef IRQF_SHARED
#define IRQF_SHARED SA_SHIRQ
#endif

/****************************** bitops.h **********************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#endif

/****************************** kernel.h **********************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
#define DIV_ROUND_CLOSEST(x, divisor)(                  \
{                                                       \
        typeof(divisor) __divisor = divisor;            \
        (((x) + ((__divisor) / 2)) / (__divisor));      \
}                                                       \
)
#endif

#endif /* _KCOMPAT_H */
