// SPDX-License-Identifier: GPL-2.0

#include <linux/acpi.h>
#include <linux/i2c.h>

/* From Intel's ipu4-acpi */
struct sensor_bios_data_packed {
	u8 version;
	u8 sku;
	u8 guid_csi2[16];
	u8 devfunction;
	u8 bus;
	u32 dphylinkenfuses;
	u32 clockdiv;
	u8 link;
	u8 lanes;
	u32 csiparams[10];
	u32 maxlanespeed;
	u8 sensorcalibfileidx;
	u8 sensorcalibfileidxInMBZ[3];
	u8 romtype;
	u8 vcmtype;
	u8 platforminfo;
	u8 platformsubinfo;
	u8 flash;
	u8 privacyled;
	u8 degree;
	u8 mipilinkdefined;
	u32 mclkspeed;
	u8 controllogicid;
	u8 reserved1[3];
	u8 mclkport;
	u8 reserved2[13];
} __attribute__((__packed__));

/* From coreboot */
struct intel_ssdb {
	uint8_t version;			/* Current version */
	uint8_t sensor_card_sku;		/* CRD Board type */
	uint8_t csi2_data_stream_interface[16];	/* CSI2 data stream GUID */
	uint16_t bdf_value;			/* Bus number of the host
						controller */
	uint32_t dphy_link_en_fuses;		/* Host controller's fuses
						information used to verify if
						link is fused out or not */
	uint32_t lanes_clock_division;		/* Lanes/clock divisions per
						sensor */
	uint8_t link_used;			/* Link used by this sensor
						stream */
	uint8_t lanes_used;			/* Number of lanes connected for
						the sensor */
	uint32_t csi_rx_dly_cnt_termen_clane;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_settle_clane;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_termen_dlane0;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_settle_dlane0;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_termen_dlane1;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_settle_dlane1;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_termen_dlane2;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_settle_dlane2;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_termen_dlane3;	/* MIPI timing information */
	uint32_t csi_rx_dly_cnt_settle_dlane3;	/* MIPI timing information */
	uint32_t max_lane_speed;		/* Maximum lane speed for
						the sensor */
	uint8_t sensor_cal_file_idx;		/* Legacy field for sensor
						calibration file index */
	uint8_t sensor_cal_file_idx_mbz[3];	/* Legacy field for sensor
						calibration file index */
	uint8_t rom_type;			/* NVM type of the camera
						module */
	uint8_t vcm_type;			/* VCM type of the camera
						module */
	uint8_t platform;			/* Platform information */
	uint8_t platform_sub;			/* Platform sub-categories */
	uint8_t flash_support;			/* Enable/disable flash
						support */
	uint8_t privacy_led;			/* Privacy LED support */
	uint8_t degree;				/* Camera Orientation */
	uint8_t mipi_define;			/* MIPI info defined in ACPI or
						sensor driver */
	uint32_t mclk_speed;			/* Clock info for sensor */
	uint8_t control_logic_id;		/* PMIC device node used for
						the camera sensor */
	uint8_t mipi_data_format;		/* MIPI data format */
	uint8_t silicon_version;		/* Silicon version */
	uint8_t customer_id;			/* Customer ID */
	uint8_t mclk_port;
	uint8_t reserved[13];			/* Pads SSDB out so the binary blob in ACPI is
						   the same size as seen on other firmwares.*/
} __attribute__((__packed__));

/* From old chromiumos' ACPI info reading implementation */
struct intel_cldb {
	u8 version;
	/*
	 * control logic type
	 * 0: UNKNOWN
	 * 1: DISCRETE(CRD-D)
	 * 2: PMIC TPS68470
	 * 3: PMIC uP6641
	 */
	u8 control_logic_type;
	u8 control_logic_id; /* PMIC device node used for the camera sensor */
	u8 sensor_card_sku;
	u8 reserved[28];
} __attribute__((__packed__));

void dump_ssdb(struct device *dev, struct intel_ssdb *data)
{
	dev_info(dev, "link_used: %d\n", data->link_used);
	dev_info(dev, "lanes_used: %d\n", data->lanes_used);
	dev_info(dev, "vcm_type: %d\n", data->vcm_type);
	dev_info(dev, "flash_support: %d\n", data->flash_support);
	dev_info(dev, "degree: %d\n", data->degree);
	dev_info(dev, "mclk_port: %d\n", data->mclk_port);
	dev_info(dev, "mclk_speed: %d\n", data->mclk_speed);

	dev_info(dev, "/* Additional data */\n");
	dev_info(dev, "lanes_clock_division: %d\n", data->lanes_clock_division);
	dev_info(dev, "rom_type: %d\n", data->rom_type);
	dev_info(dev, "privacy_led: %d\n", data->privacy_led);
	dev_info(dev, "mipi_define: %d\n", data->mipi_define);
	dev_info(dev, "control_logic_id: %d\n", data->control_logic_id);

	dev_info(dev, "\n");
}

void dump_cldb(struct device *dev, struct intel_cldb *data)
{
	dev_info(dev, "version: %d\n", data->version);
	dev_info(dev, "control_logic_type: %d\n", data->control_logic_type);
	dev_info(dev, "control_logic_id: %d\n", data->control_logic_id);
	dev_info(dev, "sensor_card_sku: %d\n", data->sensor_card_sku);
	dev_info(dev, "\n");
}

static int read_acpi_block(struct device *dev, char *id, void *data, u32 size)
{
	union acpi_object *obj;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	struct acpi_handle *dev_handle = ACPI_HANDLE(dev);
	int status;
	u32 buffer_length;

	status = acpi_evaluate_object(dev_handle, id, NULL, &buffer);
	if (!ACPI_SUCCESS(status))
		return -ENODEV;

	obj = (union acpi_object *)buffer.pointer;
	if (!obj || obj->type != ACPI_TYPE_BUFFER) {
		dev_err(dev, "Could't read acpi buffer\n");
		status = -ENODEV;
		goto err;
	}

	if (obj->buffer.length > size) {
		dev_err(dev, "Given buffer is too small\n");
		status = -ENODEV;
		goto err;
	}

	memcpy(data, obj->buffer.pointer, min(size, obj->buffer.length));
	buffer_length = obj->buffer.length;
	kfree(buffer.pointer);

	return buffer_length;
err:
	kfree(buffer.pointer);
	return status;
}

static int get_acpi_data(struct device *dev)
{
	struct intel_ssdb sensor_data;
	int ret;
	
	ret = read_acpi_block(dev, "SSDB", &sensor_data, sizeof(sensor_data));
	if (ret < 0)
		return ret;

	dump_ssdb(dev, &sensor_data);
	dev_info(dev, "-------------------- %s done --------------------\n",
		 dev_name(dev));

	return 0;
}

static int get_acpi_data_probe(struct i2c_client *client)
{
	get_acpi_data(&client->dev);

	return 0;
}

static int get_acpi_data_remove(struct i2c_client *client)
{
	return 0;
}

static const struct acpi_device_id get_acpi_data_acpi_ids[] = {
	{"INT33BE"}, // ov5693
	{"INT3479"}, // ov5670
	{"INT347A"}, // ov8865
	{"INT347E"}, // ov7251
	{"OVTI2680"}, // ov2680
	{"OVTI5648"}, // ov5648
	{"OVTI9734"}, // ov9734
	{},
};
MODULE_DEVICE_TABLE(acpi, get_acpi_data_acpi_ids);

static struct i2c_driver get_acpi_data_i2c_driver = {
	.driver = {
		.acpi_match_table = ACPI_PTR(get_acpi_data_acpi_ids),
		.name  = "get_acpi_data",
	},
	.probe_new  = get_acpi_data_probe,
	.remove = get_acpi_data_remove,
};

module_i2c_driver(get_acpi_data_i2c_driver);

MODULE_LICENSE("GPL v2");
