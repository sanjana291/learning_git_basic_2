# USB CDC ACM

This application demonstrates USB CDC ACM functionality on the i.MX RT1176.

The board provides two USB controllers that can be used for the CDC ACM device:

* USB1
* USB2

The USB controller is selected using a Device Tree overlay during the build.

> **Important:** No USB controller is selected by default. The user must explicitly select either `usb1.overlay` or `usb2.overlay`.

## Supported Configurations

| Overlay        | USB Controller | USB PHY |
| -------------- | -------------- | ------- |
| `usb1.overlay` | USB1           | USBPHY1 |
| `usb2.overlay` | USB2           | USBPHY2 |

Only one USB overlay should be selected for a build.

---

## 1. Using USB1

To use USB1, build the application with `usb1.overlay`:

```bash
west build -b imxrt1176_som/mimxrt1176/cm7 .\calixto-zephyr-app\apps\07_cdc_acm -- -DEXTRA_DTC_OVERLAY_FILE="usb1.overlay"
```

Flash the application:

```bash
west flash
```

The `usb1.overlay` configuration enables:

```text
USB1
 └── USBPHY1
      └── Zephyr USB Device Controller
           └── CDC ACM UART
```

---

## 2. Using USB2

To use USB2, build the application with `usb2.overlay`:

```bash
west build -b imxrt1176_som/mimxrt1176/cm7 .\calixto-zephyr-app\apps\07_cdc_acm -- -DEXTRA_DTC_OVERLAY_FILE="usb2.overlay"
```

Flash the application:

```bash
west flash
```

The `usb2.overlay` configuration enables:

```text
USB2
 └── USBPHY2
      └── Zephyr USB Device Controller
           └── CDC ACM UART
```

---

## Overlay Configuration

### `usb1.overlay`

```dts
zephyr_udc0: &usb1 {
	status = "okay";
	phy_handle = <&usbphy1>;

	cdc_acm_uart0: cdc_acm_uart0 {
		compatible = "zephyr,cdc-acm-uart";
	};
};

&usbphy1 {
	status = "okay";
	tx-d-cal = <7>;
	tx-cal-45-dp-ohms = <6>;
	tx-cal-45-dm-ohms = <6>;
};
```

### `usb2.overlay`

```dts
zephyr_udc0: &usb2 {
	status = "okay";
	phy_handle = <&usbphy2>;

	cdc_acm_uart0: cdc_acm_uart0 {
		compatible = "zephyr,cdc-acm-uart";
	};
};

&usbphy2 {
	status = "okay";
	tx-d-cal = <7>;
	tx-cal-45-dp-ohms = <6>;
	tx-cal-45-dm-ohms = <6>;
};
```

## Build Directory

If switching between USB1 and USB2, use a pristine build to ensure the previous Device Tree configuration is not reused.

### USB1

```bash
west build -p always -b imxrt1176_som/mimxrt1176/cm7 .\calixto-zephyr-app\apps\07_cdc_acm -- -DEXTRA_DTC_OVERLAY_FILE="usb1.overlay"
```

### USB2

```bash
west build -p always -b imxrt1176_som/mimxrt1176/cm7 .\calixto-zephyr-app\apps\07_cdc_acm -- -DEXTRA_DTC_OVERLAY_FILE="usb2.overlay"
```

## Important

* There is **no default USB configuration**.
* The user must select either `usb1.overlay` or `usb2.overlay`.
* Do not specify both overlays in the same build.
* `usb1.overlay` uses USB1 + USBPHY1.
* `usb2.overlay` uses USB2 + USBPHY2.
* The selected controller becomes `zephyr_udc0`.
* The selected controller provides the CDC ACM UART interface.

## Quick Reference

### USB1

```bash
west build -b imxrt1176_som/mimxrt1176/cm7 .\calixto-zephyr-app\apps\07_cdc_acm -- -DEXTRA_DTC_OVERLAY_FILE="usb1.overlay"
```

### USB2

```bash
west build -b imxrt1176_som/mimxrt1176/cm7 .\calixto-zephyr-app\apps\07_cdc_acm -- -DEXTRA_DTC_OVERLAY_FILE="usb2.overlay"
```

### Flash

```bash
west flash
```
