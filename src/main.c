//Tavoitteena päästä kurssi läpi eli 1-2


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Global variable
// Must be outside any function!!
int led_state = 0;

int init_led(void);
void red_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);

// Thread initialization
#define STACKSIZE 500
#define PRIORITY 5

K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

// Main program
int main(void)
{
	init_led();
	return 0;
}

// Initialize leds
int init_led(void) {
	int ret;

        //red
	ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) return ret;
	gpio_pin_set_dt(&red, 0);

        //green
	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) return ret;
	gpio_pin_set_dt(&green, 0);

	//blue
	ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) return ret;
	gpio_pin_set_dt(&blue, 0);

	printk("Leds initialized ok\n");
	return 0;
}

// Task to handle red led
void red_led_task(void *p1, void *p2, void *p3) {
	while (true) {
		if (led_state == 0) {
		
			gpio_pin_set_dt(&red, 1);
			gpio_pin_set_dt(&green, 0);
			gpio_pin_set_dt(&blue, 0);
			printk("Red ON\n");

			k_sleep(K_SECONDS(1));

			led_state = 1;
		}
		k_msleep(100);
	}
}

// Task to handle yellow led
void yellow_led_task(void *p1, void *p2, void *p3) {
	while (true) {
		if (led_state == 1) {

			gpio_pin_set_dt(&red, 1);
			gpio_pin_set_dt(&green, 1);
			gpio_pin_set_dt(&blue, 0);
			printk("Yellow ON\n");

			k_sleep(K_SECONDS(1));

			led_state = 2;
		}
		k_msleep(100);
	}
}

// Task to handle green led
void green_led_task(void *p1, void *p2, void *p3) {
	while (true) {
		if (led_state == 2) {

			gpio_pin_set_dt(&red, 0);
			gpio_pin_set_dt(&green, 1);
			gpio_pin_set_dt(&blue, 0);
			printk("Green ON\n");

			k_sleep(K_SECONDS(1));

			led_state = 0;
		}
		k_msleep(100);
	}
}