#ifndef CTS_CONFIG_H
#define CTS_CONFIG_H

/** Driver version */
#define CFG_CTS_DRIVER_MAJOR_VERSION        2
#define CFG_CTS_DRIVER_MINOR_VERSION        0
#define CFG_CTS_DRIVER_PATCH_VERSION        2
#define CFG_CTS_DRIVER_VERSION              "v2.0.3"

#define CFG_CTS_HAS_RESET_PIN

//#define CONFIG_CTS_I2C_HOST
#ifndef CONFIG_CTS_I2C_HOST
#define CFG_CTS_SPI_SPEED_KHZ               9600
#endif

#ifdef CONFIG_PROC_FS
#define CFG_CTS_TOOL_PROC_FILENAME          "cts_tool"
#endif

#ifdef CONFIG_SYSFS
#define CONFIG_CTS_SYSFS
#endif

#define CFG_CTS_MAX_TOUCH_NUM               (10)

#define CFG_CTS_GESTURE

/* Platform configurations */
#define CFG_CTS_MAX_I2C_XFER_SIZE           (128)

#define CFG_CTS_MAX_SPI_XFER_SIZE           (1400u)


#define CFG_CTS_DEVICE_NAME                 "chipone-tddi"
#define CFG_CTS_DRIVER_NAME                 "chipone,icnl9916"

#define CFG_CTS_OF_DEVICE_ID_NAME           "chipone-tddi"
#define CFG_CTS_OF_INT_GPIO_NAME            "chipone,irq-gpio"
#define CFG_CTS_OF_RST_GPIO_NAME            "chipone,rst-gpio"
#endif /* CTS_CONFIG_H */

