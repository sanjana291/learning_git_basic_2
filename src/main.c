/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2026 Calixto Systems Pvt Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/usb/usbd.h>

#define LED0              DT_ALIAS(led0)
#define LED_TOGGLE_MS     1000
#define RX_RING_BUF_SIZE  1024
#define UART_FIFO_SIZE    64

LOG_MODULE_REGISTER(cdc_acm_echo, LOG_LEVEL_INF);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0, gpios);
static const struct device *const cdc_uart =
	DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

static uint8_t rx_buffer[RX_RING_BUF_SIZE];
static struct ring_buf ringbuf;
static bool rx_throttled;

K_SEM_DEFINE(dtr_sem, 0, 1);

static void print_baudrate(const struct device *dev)
{
	uint32_t baudrate;
	int ret;

	ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret != 0) {
		LOG_WRN("Failed to get baudrate, ret=%d", ret);
		return;
	}

	LOG_INF("Baudrate: %u", baudrate);
}

static void usb_msg_callback(struct usbd_context *const ctx,
			     const struct usbd_msg *msg)
{
	LOG_DBG("USB message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(ctx)) {
		switch (msg->type) {
		case USBD_MSG_VBUS_READY:
			if (usbd_enable(ctx) != 0) {
				LOG_ERR("Failed to enable USB device");
			}
			break;

		case USBD_MSG_VBUS_REMOVED:
			if (usbd_disable(ctx) != 0) {
				LOG_ERR("Failed to disable USB device");
			}
			break;

		default:
			break;
		}
	}

	switch (msg->type) {
	case USBD_MSG_CDC_ACM_CONTROL_LINE_STATE: {
		uint32_t dtr = 0U;

		if (uart_line_ctrl_get(msg->dev, UART_LINE_CTRL_DTR, &dtr) != 0) {
			LOG_WRN("Failed to get DTR state");
		} else if (dtr != 0U) {
			k_sem_give(&dtr_sem);
		}
		break;
	}

	case USBD_MSG_CDC_ACM_LINE_CODING:
		print_baudrate(msg->dev);
		break;

	default:
		break;
	}
}

static int usb_init(void)
{
	struct usbd_context *ctx;

	ctx = sample_usbd_init_device(usb_msg_callback);
	if (ctx == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(ctx)) {
		int ret = usbd_enable(ctx);

		if (ret != 0) {
			LOG_ERR("Failed to enable USB device, ret=%d", ret);
			return ret;
		}
	}

	LOG_INF("USB device initialized");

	return 0;
}

static void cdc_uart_irq_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (!rx_throttled && uart_irq_rx_ready(dev)) {
			uint8_t buffer[UART_FIFO_SIZE];
			size_t len;
			int recv_len;
			uint32_t stored_len;

			len = MIN(ring_buf_space_get(&ringbuf), sizeof(buffer));

			if (len == 0U) {
				uart_irq_rx_disable(dev);
				rx_throttled = true;
				continue;
			}

			recv_len = uart_fifo_read(dev, buffer, len);
			if (recv_len < 0) {
				LOG_ERR("Failed to read UART FIFO");
				continue;
			}

			stored_len = ring_buf_put(&ringbuf, buffer, recv_len);

			if (stored_len < (uint32_t)recv_len) {
				LOG_WRN("Dropped %u bytes",
					(uint32_t)recv_len - stored_len);
			}

			if (stored_len > 0U) {
				uart_irq_tx_enable(dev);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			uint8_t buffer[UART_FIFO_SIZE];
			uint32_t len;
			int sent_len;

			len = ring_buf_get(&ringbuf, buffer, sizeof(buffer));

			if (len == 0U) {
				uart_irq_tx_disable(dev);
				continue;
			}

			if (rx_throttled) {
				uart_irq_rx_enable(dev);
				rx_throttled = false;
			}

			sent_len = uart_fifo_fill(dev, buffer, len);

			if (sent_len < 0) {
				LOG_ERR("Failed to write UART FIFO");
			} else if ((uint32_t)sent_len < len) {
				LOG_WRN("Dropped %u bytes",
					len - (uint32_t)sent_len);
			}
		}
	}
}

int main(void)
{
	int ret;

	if (!device_is_ready(cdc_uart)) {
		LOG_ERR("CDC ACM UART is not ready");
		return 1;
	}

	ring_buf_init(&ringbuf, sizeof(rx_buffer), rx_buffer);

	ret = usb_init();
	if (ret != 0) {
		LOG_ERR("USB initialization failed");
		return 1;
	}

	LOG_INF("Waiting for host DTR...");

	ret = k_sem_take(&dtr_sem, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed waiting for DTR");
		return 1;
	}

	LOG_INF("Host connected");

	ret = uart_line_ctrl_set(cdc_uart, UART_LINE_CTRL_DCD, 1);
	if (ret != 0) {
		LOG_WRN("Failed to set DCD, ret=%d", ret);
	}

	ret = uart_line_ctrl_set(cdc_uart, UART_LINE_CTRL_DSR, 1);
	if (ret != 0) {
		LOG_WRN("Failed to set DSR, ret=%d", ret);
	}

	k_msleep(100);

	ret = uart_irq_callback_user_data_set(
		cdc_uart,
		cdc_uart_irq_handler,
		NULL);
	if (ret != 0) {
		LOG_ERR("Failed to set UART IRQ callback, ret=%d", ret);
		return 1;
	}

	uart_irq_rx_enable(cdc_uart);

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure LED, ret=%d", ret);
		return 1;
	}

	LOG_INF("CDC ACM echo started");

	while (1) {
		ret = gpio_pin_toggle_dt(&led0);
		if (ret != 0) {
			LOG_ERR("Failed to toggle LED, ret=%d", ret);
			return 1;
		}

		k_msleep(LED_TOGGLE_MS);
	}
}