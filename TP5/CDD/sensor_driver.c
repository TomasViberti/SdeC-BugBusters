#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BugBusters");
MODULE_DESCRIPTION("CDD - Sensor de dos señales via GPIO");

#define DEVICE_NAME "sensor_driver"
#define CLASS_NAME  "sensor_driver_class"
#define BUF_SIZE    32

/* GPIOs disponibles */
#define GPIO_SIGNAL_0  4
#define GPIO_SIGNAL_1  17

static dev_t        first;
static struct cdev  c_dev;
static struct class *cl;
static struct gpio_device *gpio_dev;
static struct gpio_chip *gpio_chip;

static int signal_select = 0;  /* 0=GPIO4, 1=GPIO17 */
static char data_buf[BUF_SIZE];
static struct gpio_desc *gpio_desc_0;
static struct gpio_desc *gpio_desc_1;

/* Lee el valor de un GPIO usando la API de descriptores del kernel */
static int read_gpio(int gpio_num)
{
    struct gpio_desc *desc;

    if (gpio_num == GPIO_SIGNAL_0)
        desc = gpio_desc_0;
    else if (gpio_num == GPIO_SIGNAL_1)
        desc = gpio_desc_1;
    else
        desc = NULL;

    if (!desc) {
        printk(KERN_WARNING "sensor_driver: GPIO%d no soportado\n", gpio_num);
        return -1;
    }

    return gpiod_get_value_cansleep(desc);
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
    const char *const chip_labels[] = {
        "gpiochip0",
        "pinctrl-rp1",
        "pinctrl-bcm2711",
        "pinctrl-bcm2835",
    };
    size_t i;

    printk(KERN_INFO "sensor_driver: init() start\n");

    for (i = 0; i < ARRAY_SIZE(chip_labels); i++) {
        gpio_dev = gpio_device_find_by_label(chip_labels[i]);
        if (!IS_ERR_OR_NULL(gpio_dev)) {
            printk(KERN_INFO "sensor_driver: usando chip GPIO '%s'\n", chip_labels[i]);
            break;
        }
    }

    if (IS_ERR_OR_NULL(gpio_dev)) {
        printk(KERN_ERR "sensor_driver: no se encontro un gpio_device valido\n");
        return -ENODEV;
    }

    gpio_chip = gpio_device_get_chip(gpio_dev);
    if (!gpio_chip) {
        printk(KERN_ERR "sensor_driver: no se pudo obtener gpio_chip\n");
        gpio_device_put(gpio_dev);
        gpio_dev = NULL;
        return -ENODEV;
    }

    gpio_desc_0 = gpiochip_request_own_desc(gpio_chip, GPIO_SIGNAL_0,
                                            "sensor_driver_gpio4",
                                            GPIO_LOOKUP_FLAGS_DEFAULT,
                                            GPIOD_IN);
    if (IS_ERR_OR_NULL(gpio_desc_0)) {
        printk(KERN_ERR "sensor_driver: no se pudo reservar GPIO%d\n",
               GPIO_SIGNAL_0);
        gpio_device_put(gpio_dev);
        gpio_dev = NULL;
        return -ENODEV;
    }

    gpio_desc_1 = gpiochip_request_own_desc(gpio_chip, GPIO_SIGNAL_1,
                                            "sensor_driver_gpio17",
                                            GPIO_LOOKUP_FLAGS_DEFAULT,
                                            GPIOD_IN);
    if (IS_ERR_OR_NULL(gpio_desc_1)) {
        printk(KERN_ERR "sensor_driver: no se pudo reservar GPIO%d\n",
               GPIO_SIGNAL_1);
        gpiochip_free_own_desc(gpio_desc_0);
        gpio_desc_0 = NULL;
        gpio_device_put(gpio_dev);
        gpio_dev = NULL;
        return -ENODEV;
    }

    if ((ret = alloc_chrdev_region(&first, 0, 1, DEVICE_NAME)) < 0)
        goto err_free_gpios;

    if (IS_ERR(cl = class_create(CLASS_NAME))) {
        printk(KERN_ERR "sensor_driver: class_create(%s) fallo: %ld\n",
               CLASS_NAME, PTR_ERR(cl));
        unregister_chrdev_region(first, 1);
        ret = PTR_ERR(cl);
        goto err_free_gpios;
    }

    printk(KERN_INFO "sensor_driver: class_create(%s) OK\n", CLASS_NAME);

    if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, DEVICE_NAME))) {
        printk(KERN_ERR "sensor_driver: device_create(%s) fallo: %ld\n",
               DEVICE_NAME, PTR_ERR(dev_ret));
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        ret = PTR_ERR(dev_ret);
        goto err_free_gpios;
    }

    printk(KERN_INFO "sensor_driver: device_create(%s) OK\n", DEVICE_NAME);

    cdev_init(&c_dev, &sensor_fops);
    if ((ret = cdev_add(&c_dev, first, 1)) < 0) {
        printk(KERN_ERR "sensor_driver: cdev_add fallo: %d\n", ret);
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        goto err_free_gpios;
    }

    printk(KERN_INFO "sensor_driver: cdev_add OK\n");

    printk(KERN_INFO "sensor_driver: cargado → /dev/%s\n", DEVICE_NAME);
    printk(KERN_INFO "sensor_driver: usando GPIO descriptor API para %d y %d\n",
           GPIO_SIGNAL_0, GPIO_SIGNAL_1);
    return 0;

err_free_gpios:
    if (gpio_desc_1) {
        gpiochip_free_own_desc(gpio_desc_1);
        gpio_desc_1 = NULL;
    }
    if (gpio_desc_0) {
        gpiochip_free_own_desc(gpio_desc_0);
        gpio_desc_0 = NULL;
    }
    if (gpio_dev) {
        gpio_device_put(gpio_dev);
        gpio_dev = NULL;
        gpio_chip = NULL;
    }
    return ret;
}

static void __exit sensor_exit(void)
{
    if (gpio_desc_1) {
        gpiochip_free_own_desc(gpio_desc_1);
        gpio_desc_1 = NULL;
    }
    if (gpio_desc_0) {
        gpiochip_free_own_desc(gpio_desc_0);
        gpio_desc_0 = NULL;
    }
    if (gpio_dev) {
        gpio_device_put(gpio_dev);
        gpio_dev = NULL;
        gpio_chip = NULL;
    }
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "sensor_driver: descargado\n");
}

module_init(sensor_init);
module_exit(sensor_exit);
