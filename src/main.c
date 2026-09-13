// Tavoitteena päästä kurssi läpi eli 1-2

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>


// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Thread initializations
#define STACKSIZE 500
#define PRIORITY 5

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);

// Condition variables + Mutex
K_MUTEX_DEFINE(mutexled);
K_CONDVAR_DEFINE(redcon);
K_CONDVAR_DEFINE(yellowcon);
K_CONDVAR_DEFINE(greencon);

//Release signal
K_SEM_DEFINE(complete, 0, 1);

// FIFO data type
struct data_t {
	void *fifo_reserved;
	char msg[20];
};

int init_uart(void);
int init_led(void);
static void uart_task(void *, void *, void *);
static void dispatcher_task(void *, void *, void *);
static void red_task(void *, void *, void *);
static void yellow_task(void *, void *, void *);
static void green_task(void *, void *, void *);

// Thread initialization
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dis_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(red_thread, STACKSIZE, red_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_task, NULL, NULL, NULL, PRIORITY, 0, 0);

int init_uart(void) {
	if (!device_is_ready(uart_dev)) {
	return 1;
	} 
	return 0;
}

int init_led(void) {
	int ret;

	ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) return ret;
	gpio_pin_set_dt(&red, 0);

	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) return ret;
	gpio_pin_set_dt(&green, 0);

	ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) return ret;
	gpio_pin_set_dt(&blue, 0);

	return 0;
}

int main(void)
{
	if (init_uart() != 0 || init_led() != 0) {
		printk("Initialization failed!\n");
		return -1;
	}

	return 0;
}

// Uart task
static void uart_task(void *unused1, void *unused2, void *unused3)
{
	char rc = 0;
	char uart_msg[20];
	memset(uart_msg, 0, 20);
	int uart_msg_cnt = 0;

	while (true) {
		if (uart_poll_in(uart_dev, &rc) == 0) {
			if (rc != '\r' && rc != '\n') {
				if (uart_msg_cnt < 19) {
					uart_msg[uart_msg_cnt++] = rc;
				}
				} else if (uart_msg_cnt > 0) {

			printk("UART received: %s\n", uart_msg);

				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf != NULL) {
					snprintf(buf->msg, sizeof(buf->msg), "%s", uart_msg);

					k_fifo_put(&dispatcher_fifo, buf);
				}

			uart_msg_cnt = 0;
			memset(uart_msg, 0, 20);
			}
		}
		k_msleep(10);
	}
}

// dispatcher task

static void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {

		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence, rec_item->msg, 20);
		k_free(rec_item);

		printk("Dispatcher processing: %s\n", sequence);


		for (int i = 0; i < strlen(sequence); i++) {
			char color = sequence[i];

			k_mutex_lock(&mutexled, K_FOREVER);


			if (color == 'R' || color == 'r') {
				k_condvar_signal(&redcon);
			} else if (color == 'Y' || color == 'y') {
				k_condvar_signal(&yellowcon);
			} else if (color == 'G' || color == 'g') {
				k_condvar_signal(&greencon);
			} else {
				k_mutex_unlock(&mutexled);
				continue; 
			}

			k_mutex_unlock(&mutexled);

			k_sem_take(&complete, K_FOREVER);
		}
	}
}


//Led tasks
// red led
static void red_task(void *p1, void *p2, void *p3) {
	while (true) {
		k_mutex_lock(&mutexled, K_FOREVER);
		k_condvar_wait(&redcon, &mutexled, K_FOREVER);
		k_mutex_unlock(&mutexled);

		gpio_pin_set_dt(&red, 1);
		printk("[LED] Red ON\n");
		k_sleep(K_SECONDS(1));
		gpio_pin_set_dt(&red, 0);


		k_sem_give(&complete);
	}
}

// yellow led
static void yellow_task(void *p1, void *p2, void *p3) {
	while (true) {
		k_mutex_lock(&mutexled, K_FOREVER);
		k_condvar_wait(&yellowcon, &mutexled, K_FOREVER);
		k_mutex_unlock(&mutexled);


		gpio_pin_set_dt(&red, 1);
		gpio_pin_set_dt(&green, 1);
		printk("[LED] Yellow ON\n");
		k_sleep(K_SECONDS(1));
		gpio_pin_set_dt(&red, 0);
		gpio_pin_set_dt(&green, 0);

		k_sem_give(&complete);
	}
}

// green led
static void green_task(void *p1, void *p2, void *p3) {
while (true) {
		k_mutex_lock(&mutexled, K_FOREVER);
		k_condvar_wait(&greencon, &mutexled, K_FOREVER);
		k_mutex_unlock(&mutexled);

		gpio_pin_set_dt(&green, 1);
		printk("[LED] Green ON\n");
		k_sleep(K_SECONDS(1));
		gpio_pin_set_dt(&green, 0);

		k_sem_give(&complete);
	}
}