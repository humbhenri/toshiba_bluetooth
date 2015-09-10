/*
 * Toshiba Bluetooth Enable Driver
 *
 * Copyright (C) 2009 Jes Sorensen <Jes.Sorensen@gmail.com>
 * Copyright (C) 2015 Azael Avalos <coproscefalo@gmail.com>
 *
 * Thanks to Matthew Garrett for background info on ACPI innards which
 * normal people aren't meant to understand :-)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Note the Toshiba Bluetooth RFKill switch seems to be a strange
 * fish. It only provides a BT event when the switch is flipped to
 * the 'on' position. When flipping it to 'off', the USB device is
 * simply pulled away underneath us, without any BT event being
 * delivered.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/acpi.h>

#define BT_KILLSWITCH_MASK	0x01
#define BT_PLUGGED_MASK		0x40
#define BT_POWER_MASK		0x80

MODULE_AUTHOR("Jes Sorensen <Jes.Sorensen@gmail.com>");
MODULE_DESCRIPTION("Toshiba Laptop ACPI Bluetooth Enable Driver");
MODULE_LICENSE("GPL");

static int toshiba_bluetooth_add(struct acpi_device *device);
static int toshiba_bluetooth_remove(struct acpi_device *device);
static void toshiba_bluetooth_notify(struct acpi_device *device, u32 event);

static const struct acpi_device_id toshiba_bluetooth_device_ids[] = {
	{ "TOS6205", 0},
	{ "", 0},
};
MODULE_DEVICE_TABLE(acpi, toshiba_bluetooth_device_ids);

#ifdef CONFIG_PM_SLEEP
static int toshiba_bluetooth_resume(struct device *dev);
#endif
static SIMPLE_DEV_PM_OPS(toshiba_bluetooth_pm, NULL, toshiba_bluetooth_resume);

static struct acpi_driver toshiba_bluetooth_driver = {
	.name =		"Toshiba BT",
	.class =	"Toshiba",
	.ids =		toshiba_bluetooth_device_ids,
	.ops =		{
				.add =		toshiba_bluetooth_add,
				.remove =	toshiba_bluetooth_remove,
				.notify =	toshiba_bluetooth_notify,
			},
	.owner = 	THIS_MODULE,
	.drv.pm =	&toshiba_bluetooth_pm,
};

static int toshiba_bluetooth_present(acpi_handle handle)
{
	acpi_status result;
	u64 bt_present;

	/*
	 * Some Toshiba laptops may have a fake TOS6205 device in
	 * their ACPI BIOS, so query the _STA method to see if there
	 * is really anything there.
	 */
	result = acpi_evaluate_integer(handle, "_STA", NULL, &bt_present);
	if (ACPI_FAILURE(result)) {
		pr_err("ACPI call to query Bluetooth presence failed");
		return -ENXIO;
	} else if (!bt_present) {
		pr_info("Bluetooth device not present\n");
		return -ENODEV;
	}

	return 0;
}

static int toshiba_bluetooth_status(acpi_handle handle)
{
	acpi_status result;
	u64 status;

	result = acpi_evaluate_integer(handle, "BTST", NULL, &status);
	if (ACPI_FAILURE(result)) {
		pr_err("Could not get Bluetooth device status\n");
		return -ENXIO;
	}

	pr_info("Bluetooth status %llu\n", status);

	return status;
}

static int toshiba_bluetooth_enable(acpi_handle handle)
{
	acpi_status result;

	result = acpi_evaluate_object(handle, "AUSB", NULL, NULL);
	if (ACPI_FAILURE(result)) {
		pr_err("Could not attach USB Bluetooth device\n");
		return -ENXIO;
	}

	result = acpi_evaluate_object(handle, "BTPO", NULL, NULL);
	if (ACPI_FAILURE(result)) {
		pr_err("Could not power ON Bluetooth device\n");
		return -ENXIO;
	}

	return 0;
}

static int toshiba_bluetooth_disable(acpi_handle handle)
{
	acpi_status result;

	result = acpi_evaluate_object(handle, "BTPF", NULL, NULL);
	if (ACPI_FAILURE(result)) {
		pr_err("Could not power OFF Bluetooth device\n");
		return -ENXIO;
	}

	result = acpi_evaluate_object(handle, "DUSB", NULL, NULL);
	if (ACPI_FAILURE(result)) {
		pr_err("Could not detach USB Bluetooth device\n");
		return -ENXIO;
	}

	return 0;
}

static void toshiba_bluetooth_notify(struct acpi_device *device, u32 event)
{
	int status;

	pr_info("Received event %x\n", event);

	/* Query the status of the Bluetooth device */
	status = toshiba_bluetooth_status(device->handle);
	if (status < 0)
		pr_err("Error while querying Bluetooth status\n");

	pr_info("Switch status %x\n", status & BT_KILLSWITCH_MASK);
	pr_info("Power status %x\n", status & BT_POWER_MASK);
	pr_info("Plug status %x\n", status & BT_PLUGGED_MASK);

	/* TODO: Add event handling here depending on Bluetooth status */
}

#ifdef CONFIG_PM_SLEEP
static int toshiba_bluetooth_resume(struct device *dev)
{
	return toshiba_bluetooth_enable(to_acpi_device(dev)->handle);
}
#endif

static int toshiba_bluetooth_add(struct acpi_device *device)
{
	int result;

	result = toshiba_bluetooth_present(device->handle);
	if (result)
		return result;

	pr_info("Toshiba ACPI Bluetooth device driver\n");

	/* Enable the BT device */
	result = toshiba_bluetooth_enable(device->handle);
	if (result)
		return result;

	/* Query the status of the BT device */
	result = toshiba_bluetooth_status(device->handle);
	if (result < 0)
		return result;

	pr_info("Switch status %x\n", result & BT_KILLSWITCH_MASK);
	pr_info("Power status %x\n", result & BT_POWER_MASK);
	pr_info("Plug status %x\n", result & BT_PLUGGED_MASK);

	return 0;
}

static int toshiba_bluetooth_remove(struct acpi_device *device)
{
	/* Clean up */
	return toshiba_bluetooth_disable(device->handle);
}

module_acpi_driver(toshiba_bluetooth_driver);