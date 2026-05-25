#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BugBusters");
MODULE_DESCRIPTION("CDD - Sensor de dos señales via GPIO");

#define DEVICE_NAME "sensor_driver"
#define CLASS_NAME  "sensor_class"
#define BUF_SIZE    32

/* GPIOs disponibles */
#define GPIO_SIGNAL_0  4
#define GPIO_SIGNAL_1  17

/* Prueba Local */
// #define GPIO_BASE_PATH "/tmp/gpio"
/* /sys/class/gpio en QEMU */
#define GPIO_BASE_PATH "/sys/class/gpio"

static dev_t        first;
static struct cdev  c_dev;
static struct class *cl;

static int signal_select = 0;  /* 0=GPIO4, 1=GPIO17 */
static char data_buf[BUF_SIZE];

/* Lee el valor de un GPIO desde el filesystem */
static int read_gpio(int gpio_num)
{
    struct file *f;
    char path[64];
    char val_str[4];
    int  val = 0;

    snprintf(path, sizeof(path), GPIO_BASE_PATH "/gpio%d/value", gpio_num);

    f = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(f)) {
        printk(KERN_WARNING "sensor_driver: no se pudo abrir %s\n", path);
        return -1;
    }

    kernel_read(f, val_str, sizeof(val_str) - 1, &f->f_pos);
    val_str[1] = '\0';
    
    if (val_str[0] == '1')
        val = 1;
    else
        val = 0;

    filp_close(f, NULL);
    return val;
}

/* --- File operations --- */
static int my_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO "sensor_driver: open()\n");
    return 0;
}

static int my_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO "sensor_driver: close()\n");
    return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    int gpio_num;
    int gpio_val;
    int data_len;

    if (*off > 0)
        return 0; /* EOF */

    gpio_num = (signal_select == 0) ? GPIO_SIGNAL_0 : GPIO_SIGNAL_1;
    gpio_val = read_gpio(gpio_num);

    if (gpio_val < 0)
        snprintf(data_buf, BUF_SIZE, "GPIO%d:error\n", gpio_num);
    else
        snprintf(data_buf, BUF_SIZE, "GPIO%d:%d\n", gpio_num, gpio_val);

    data_len = strlen(data_buf);

    if (len < data_len)
        return -EINVAL;

    if (copy_to_user(buf, data_buf, data_len) != 0)
        return -EFAULT;

    *off += data_len;

    printk(KERN_INFO "sensor_driver: read() → %s", data_buf);
    return data_len;
}

static ssize_t my_write(struct file *f, const char __user *buf,
                        size_t len, loff_t *off)
{
    char kbuf[4];

    if (len > sizeof(kbuf) - 1)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, len) != 0)
        return -EFAULT;

    kbuf[len] = '\0';

    if (kbuf[0] == '0') {
        signal_select = 0;
        printk(KERN_INFO "sensor_driver: señal cambiada a GPIO%d\n", GPIO_SIGNAL_0);
    } else if (kbuf[0] == '1') {
        signal_select = 1;
        printk(KERN_INFO "sensor_driver: señal cambiada a GPIO%d\n", GPIO_SIGNAL_1);
    } else {
        printk(KERN_WARNING "sensor_driver: valor inválido '%c', usar 0 o 1\n", kbuf[0]);
        return -EINVAL;
    }

    return len;
}

static struct file_operations sensor_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_close,
    .read    = my_read,
    .write   = my_write,
};

/* --- Init / Exit --- */
static int __init sensor_init(void)
{
    int ret;
    struct device *dev_ret;

    if ((ret = alloc_chrdev_region(&first, 0, 1, DEVICE_NAME)) < 0)
        return ret;

    if (IS_ERR(cl = class_create(CLASS_NAME))) {
        unregister_chrdev_region(first, 1);
        return PTR_ERR(cl);
    }

    if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, DEVICE_NAME))) {
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return PTR_ERR(dev_ret);
    }

    cdev_init(&c_dev, &sensor_fops);
    if ((ret = cdev_add(&c_dev, first, 1)) < 0) {
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return ret;
    }

    printk(KERN_INFO "sensor_driver: cargado → /dev/%s\n", DEVICE_NAME);
    printk(KERN_INFO "sensor_driver: GPIO base path → %s\n", GPIO_BASE_PATH);
    return 0;
}

static void __exit sensor_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "sensor_driver: descargado\n");
}

module_init(sensor_init);
module_exit(sensor_exit);