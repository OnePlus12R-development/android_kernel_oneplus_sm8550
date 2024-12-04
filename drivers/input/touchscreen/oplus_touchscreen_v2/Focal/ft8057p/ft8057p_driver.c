// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/string.h>

#include "ft8057p_core.h"

struct chip_data_ft8057p *g_fts_data = NULL;

struct upgrade_setting_nf upgrade_setting_list[] = {
	{0x87, 0x19, 0, (64 * 1024),  (128 * 1024), 0x00, 0x02, 8,  1, 1, 1, 0, 0},
	{0x86, 0x22, 0, (64 * 1024),  (128 * 1024), 0x00, 0x02, 8,  1, 1, 0, 0, 0},
	{0x87, 0x56, 0, (88 * 1024),  32766,        0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x80, 0x09, 0, (88 * 1024),  32766,        0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x86, 0x32, 0, (64 * 1024),  (128 * 1024), 0xA5, 0x01, 12, 0, 1, 0, 0, 0},
	{0x86, 0x42, 0, (64 * 1024),  (128 * 1024), 0xA5, 0x01, 12, 0, 1, 0, 0, 0},
	{0x87, 0x20, 0, (88 * 1024),  (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x87, 0x22, 0, (88 * 1024),  (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x82, 0x01, 0, (96 * 1024),  (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 0, 0},
	{0xF0, 0xC6, 0, (84 * 1024),  (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x56, 0x62, 0, (128 * 1024), (128 * 1024), 0xA5, 0x01, 8,  0, 4, 0, 0, 5},
	{0x82, 0x05, 0, (120 * 1024), (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 0, 0},
	{0x80, 0x57, 0, (84 * 1024),  (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x80, 0xC7, 0, (84 * 1024),  (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x23, 0x89, 0, (88 * 1024),  (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x87, 0x25, 0, (88 * 1024),  (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 1, 0},
	{0x82, 0x06, 0, (120 * 1024), (128 * 1024), 0xA5, 0x01, 8,  0, 2, 0, 0, 0},
};


/*******Part0:LOG TAG Declear********************/

#ifdef TPD_DEVICE
#undef TPD_DEVICE
#define TPD_DEVICE "focaltech,nf_ft8057p"
#else
#define TPD_DEVICE "focaltech,nf_ft8057p"
#endif


#define FTS_REG_UPGRADE                             0xFC
#define FTS_UPGRADE_AA                              0xAA
#define FTS_UPGRADE_55                              0x55
#define FTS_DELAY_UPGRADE_AA                        10
#define FTS_DELAY_UPGRADE_RESET                     80
#define FTS_UPGRADE_LOOP                            10

#define FTS_ROMBOOT_CMD_SET_PRAM_ADDR               0xAD
#define FTS_ROMBOOT_CMD_SET_PRAM_ADDR_LEN           4
#define FTS_ROMBOOT_CMD_WRITE                       0xAE
#define FTS_ROMBOOT_CMD_START_APP                   0x08
#define FTS_DELAY_PRAMBOOT_START                    100
#define FTS_ROMBOOT_CMD_ECC                         0xCC
#define FTS_ROMBOOT_CMD_ECC_NEW_LEN                 7
#define FTS_ECC_FINISH_TIMEOUT                      100
#define FTS_ROMBOOT_CMD_ECC_FINISH                  0xCE
#define FTS_ROMBOOT_CMD_ECC_READ                    0xCD
#define FTS_PRAM_SADDR                              0x000000
#define FTS_DRAM_SADDR                              0xD00000
#define FTS_DELAY_READ_ID                           20

#define FTS_CMD_RESET                               0x07
#define FTS_CMD_START                               0x55
#define FTS_CMD_START_DELAY                         12
#define FTS_CMD_READ_ID                             0x90
#define FTS_CMD_DATA_LEN                            0x7A
#define FTS_CMD_ERASE_APP                           0x61
#define FTS_RETRIES_REASE                           50
#define FTS_RETRIES_DELAY_REASE                     400
#define FTS_REASE_APP_DELAY                         1350
#define FTS_CMD_ECC_INIT                            0x64
#define FTS_CMD_ECC_CAL                             0x65
#define FTS_RETRIES_ECC_CAL                         10
#define FTS_RETRIES_DELAY_ECC_CAL                   50
#define FTS_CMD_ECC_READ                            0x66
#define FTS_CMD_FLASH_STATUS                        0x6A
#define FTS_CMD_WRITE                               0xBF
#define FTS_CMD_SET_WFLASH_ADDR                     0xAB
#define FTS_CMD_SET_RFLASH_ADDR                     0xAC
#define FTS_RETRIES_WRITE                           100
#define FTS_RETRIES_DELAY_WRITE                     1

#define FTS_CMD_FLASH_STATUS_NOP                    0x0000
#define FTS_CMD_FLASH_STATUS_ECC_OK                 0xF055
#define FTS_CMD_FLASH_STATUS_ERASE_OK               0xF0AA
#define FTS_CMD_FLASH_STATUS_WRITE_OK               0x1000

#define POINT_REPORT_CHECK_WAIT_TIME                200    /* unit:ms */
#define PRC_INTR_INTERVALS                          100    /* unit:ms */

#define FTS_MIN_LEN                                 0x120
#define FTS_MAX_LEN_FILE                            (256 * 1024)
#define FTS_MAX_LEN_APP                             (64 * 1024)
#define FTS_MAX_LEN_SECTOR                          (4 * 1024)
#define FTS_CMD_WRITE_LEN                           6
#define FTS_READ_BOOT_ID_TIMEOUT                    3
#define FTS_FLASH_PACKET_LENGTH_SPI_LOW             (4 * 1024 - 4)
#define FTS_FLASH_PACKET_LENGTH_SPI                 (32 * 1024 - 16)
#define FTS_APP_INFO_OFFSET                         0x100


#define BYTE_OFF_0(x)           (u8)((x) & 0xFF)
#define BYTE_OFF_8(x)           (u8)(((x) >> 8) & 0xFF)
#define BYTE_OFF_16(x)          (u8)(((x) >> 16) & 0xFF)
#define BYTE_OFF_24(x)          (u8)(((x) >> 24) & 0xFF)


#define AL2_FCS_COEF                ((1 << 15) + (1 << 10) + (1 << 3))

enum GESTURE_ID {
	GESTURE_RIGHT2LEFT_SWIP = 0x20,
	GESTURE_LEFT2RIGHT_SWIP = 0x21,
	GESTURE_DOWN2UP_SWIP = 0x22,
	GESTURE_UP2DOWN_SWIP = 0x23,
	GESTURE_DOUBLE_TAP = 0x24,
	GESTURE_DOUBLE_SWIP = 0x25,
	GESTURE_RIGHT_VEE = 0x51,
	GESTURE_LEFT_VEE = 0x52,
	GESTURE_DOWN_VEE = 0x53,
	GESTURE_UP_VEE = 0x54,
	GESTURE_O_CLOCKWISE = 0x57,
	GESTURE_O_ANTICLOCK = 0x30,
	GESTURE_W = 0x31,
	GESTURE_M = 0x32,
	GESTURE_FINGER_PRINT = 0x26,
	GESTURE_SINGLE_TAP = 0x27,
	GESTURE_HEART_ANTICLOCK = 0x55,
	GESTURE_HEART_CLOCKWISE = 0x59,
};

static void focal_esd_check_enable(void *chip_data, bool enable);
static int fts_hw_reset(struct chip_data_ft8057p *ts_data, u32 delayms);
static int fts_enter_gesture_mode(struct touchpanel_data *ts, bool gesture_mode);




/*************************************************************
 *******************FocalTech SPI protocols*******************
 *************************************************************/

#define SPI_RETRY_NUMBER            3
#define CS_HIGH_DELAY               150 /* unit: us */
#define SPI_BUF_LENGTH              4096

#define DATA_CRC_EN                 0x20
#define WRITE_CMD                   0x00
#define READ_CMD                    (0x80 | DATA_CRC_EN)

#define SPI_DUMMY_BYTE              3
#define SPI_HEADER_LENGTH           6   /*CRC*/




/* spi interface */
static int fts_spi_transfer(struct spi_device *spi, u8 *tx_buf, u8 *rx_buf, u32 len)
{
	int ret = 0;
	struct spi_message msg;
	struct spi_transfer xfer = {
		.tx_buf = tx_buf,
		.rx_buf = rx_buf,
		.len    = len,
	};

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(spi, &msg);
	if (ret) {
		TPD_INFO("spi_sync fail,ret:%d", ret);
		return ret;
	}

	return ret;
}

static void fts_spi_buf_show(u8 *data, int datalen)
{
	int i = 0;
	int count = 0;
	int size = 0;
	char *tmpbuf = NULL;

	if (!data || (datalen <= 0)) {
		TPD_INFO("data/datalen is invalid");
		return;
	}

	size = (datalen > 256) ? 256 : datalen;
	tmpbuf = kzalloc(1024, GFP_KERNEL);
	if (!tmpbuf) {
		TPD_INFO("tmpbuf zalloc fail");
		return;
	}

	for (i = 0; i < size; i++)
		count += snprintf(tmpbuf + count, 1024 - count, "%02X ", data[i]);

	TPD_INFO("%s", tmpbuf);
	if (tmpbuf) {
		kfree(tmpbuf);
		tmpbuf = NULL;
	}
}

static void crckermit(u8 *data, u32 len, u16 *crc_out)
{
	u32 i = 0;
	u32 j = 0;
	u16 crc = 0xFFFF;

	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (j = 0; j < 8; j++) {
			if (crc & 0x01)
				crc = (crc >> 1) ^ 0x8408;
			else
				crc = (crc >> 1);
		}
	}

	*crc_out = crc;
}

static int rdata_check(u8 *rdata, u32 rlen)
{
	u16 crc_calc = 0;
	u16 crc_read = 0;

	crckermit(rdata, rlen - 2, &crc_calc);
	crc_read = (u16)(rdata[rlen - 1] << 8) + rdata[rlen - 2];
	if (crc_calc != crc_read) {
		fts_spi_buf_show(rdata, rlen);
		return -EIO;
	}

	return 0;
}

int fts_write(u8 *writebuf, u32 writelen)
{
	int ret = 0;
	int i = 0;
	struct chip_data_ft8057p *ts_data = g_fts_data;
	u8 *txbuf = NULL;
	u8 *rxbuf = NULL;
	u32 txlen = 0;
	u32 txlen_need = writelen + SPI_HEADER_LENGTH + SPI_DUMMY_BYTE;
	u32 datalen = writelen - 1;

	if (!ts_data || !ts_data->ft_spi) {
		TPD_INFO("ts_data/ft_spi is invalid");
		return -EINVAL;
	}

	if (!writebuf || !writelen) {
		TPD_INFO("writebuf/len is invalid");
		return -EINVAL;
	}

	mutex_lock(&ts_data->bus_lock);
	if (txlen_need > SPI_BUF_LENGTH) {
		txbuf = kzalloc(txlen_need, GFP_KERNEL);
		if (NULL == txbuf) {
			TPD_INFO("txbuf malloc fail");
			ret = -ENOMEM;
			goto err_write;
		}

		rxbuf = kzalloc(txlen_need, GFP_KERNEL);
		if (NULL == rxbuf) {
			TPD_INFO("rxbuf malloc fail");
			ret = -ENOMEM;
			goto err_write;
		}
	} else {
		txbuf = ts_data->bus_tx_buf;
		rxbuf = ts_data->bus_rx_buf;
		memset(txbuf, 0x0, SPI_BUF_LENGTH);
		memset(rxbuf, 0x0, SPI_BUF_LENGTH);
	}

	txbuf[txlen++] = writebuf[0];
	txbuf[txlen++] = WRITE_CMD;
	txbuf[txlen++] = (datalen >> 8) & 0xFF;
	txbuf[txlen++] = datalen & 0xFF;
	if (datalen > 0) {
		txlen = txlen + SPI_DUMMY_BYTE;
		memcpy(&txbuf[txlen], &writebuf[1], datalen);
		txlen = txlen + datalen;
	}

	for (i = 0; i < SPI_RETRY_NUMBER; i++) {
		ret = fts_spi_transfer(ts_data->ft_spi, txbuf, rxbuf, txlen);
		if ((0 == ret) && ((rxbuf[3] & 0xA0) == 0)) {
			break;
		} else {
			TPD_INFO("data write(addr:%x),status:%x,retry:%d,ret:%d",
			         writebuf[0], rxbuf[3], i, ret);
			ret = -EIO;
			udelay(CS_HIGH_DELAY);
		}
	}
	if (ret < 0) {
		TPD_INFO("data write(addr:%x) fail,status:%x,ret:%d",
		         writebuf[0], rxbuf[3], ret);
	}

err_write:
	if (txlen_need > SPI_BUF_LENGTH) {
		if (txbuf) {
			kfree(txbuf);
			txbuf = NULL;
		}

		if (rxbuf) {
			kfree(rxbuf);
			rxbuf = NULL;
		}
	}

	udelay(CS_HIGH_DELAY);
	mutex_unlock(&ts_data->bus_lock);
	return ret;
}

int fts_write_reg(u8 addr, u8 value)
{
	u8 writebuf[2] = { 0 };

	writebuf[0] = addr;
	writebuf[1] = value;
	return fts_write(writebuf, 2);
}

int fts_read(u8 *cmd, u32 cmdlen, u8 *data, u32 datalen)
{
	int ret = 0;
	int i = 0;
	struct chip_data_ft8057p *ts_data = g_fts_data;
	u8 *txbuf = NULL;
	u8 *rxbuf = NULL;
	u32 txlen = 0;
	u32 txlen_need = datalen + SPI_HEADER_LENGTH + SPI_DUMMY_BYTE;
	u8 ctrl = READ_CMD;
	u32 dp = 0;

	if (!ts_data || !ts_data->ft_spi) {
		TPD_INFO("ts_data/ft_spi is invalid");
		return -EINVAL;
	}

	if (!cmd || !cmdlen || !data || !datalen) {
		TPD_INFO("cmd/cmdlen/data/datalen is invalid");
		return -EINVAL;
	}

	mutex_lock(&ts_data->bus_lock);
	if (txlen_need > SPI_BUF_LENGTH) {
		txbuf = kzalloc(txlen_need, GFP_KERNEL);
		if (NULL == txbuf) {
			TPD_INFO("txbuf malloc fail");
			ret = -ENOMEM;
			goto err_read;
		}

		rxbuf = kzalloc(txlen_need, GFP_KERNEL);
		if (NULL == rxbuf) {
			TPD_INFO("rxbuf malloc fail");
			ret = -ENOMEM;
			goto err_read;
		}
	} else {
		txbuf = ts_data->bus_tx_buf;
		rxbuf = ts_data->bus_rx_buf;
		memset(txbuf, 0x0, SPI_BUF_LENGTH);
		memset(rxbuf, 0x0, SPI_BUF_LENGTH);
	}

	txbuf[txlen++] = cmd[0];
	txbuf[txlen++] = ctrl;
	txbuf[txlen++] = (datalen >> 8) & 0xFF;
	txbuf[txlen++] = datalen & 0xFF;
	dp = txlen + SPI_DUMMY_BYTE;
	txlen = dp + datalen;
	if (ctrl & DATA_CRC_EN) {
		txlen = txlen + 2;
	}

	for (i = 0; i < SPI_RETRY_NUMBER; i++) {
		ret = fts_spi_transfer(ts_data->ft_spi, txbuf, rxbuf, txlen);
		if ((0 == ret) && ((rxbuf[3] & 0xA0) == 0)) {
			memcpy(data, &rxbuf[dp], datalen);
			/* crc check */
			if (ctrl & DATA_CRC_EN) {
				ret = rdata_check(&rxbuf[dp], txlen - dp);
				if (ret < 0) {
					TPD_INFO("data read(addr:%x) crc abnormal,retry:%d",
					         cmd[0], i);
					udelay(CS_HIGH_DELAY);
					continue;
				}
			}
			break;
		} else {
			TPD_INFO("data read(addr:%x) status:%x,retry:%d,ret:%d",
			         cmd[0], rxbuf[3], i, ret);
			ret = -EIO;
			udelay(CS_HIGH_DELAY);
		}
	}

	if (ret < 0) {
		TPD_INFO("data read(addr:%x) %s,status:%x,ret:%d", cmd[0],
		         (i >= SPI_RETRY_NUMBER) ? "crc abnormal" : "fail",
		         rxbuf[3], ret);
	}

err_read:
	if (txlen_need > SPI_BUF_LENGTH) {
		if (txbuf) {
			kfree(txbuf);
			txbuf = NULL;
		}

		if (rxbuf) {
			kfree(rxbuf);
			rxbuf = NULL;
		}
	}

	udelay(CS_HIGH_DELAY);
	mutex_unlock(&ts_data->bus_lock);
	return ret;
}

int fts_read_reg(u8 addr, u8 *value)
{
	return fts_read(&addr, 1, value, 1);
}

static int fts_spi_transfer_direct(u8 *writebuf, u32 writelen, u8 *readbuf, u32 readlen)
{
	int ret = 0;
	struct chip_data_ft8057p *ts_data = g_fts_data;
	u8 *txbuf = NULL;
	u8 *rxbuf = NULL;
	bool read_cmd = (readbuf && readlen) ? 1 : 0;
	u32 txlen = (read_cmd) ? readlen : writelen;

	if (!writebuf || !writelen) {
		TPD_INFO("writebuf/len is invalid");
		return -EINVAL;
	}

	mutex_lock(&ts_data->bus_lock);
	if (txlen > SPI_BUF_LENGTH) {
		txbuf = kzalloc(txlen, GFP_KERNEL);
		if (NULL == txbuf) {
			TPD_INFO("txbuf malloc fail");
			ret = -ENOMEM;
			goto err_spi_dir;
		}

		rxbuf = kzalloc(txlen, GFP_KERNEL);
		if (NULL == rxbuf) {
			TPD_INFO("rxbuf malloc fail");
			ret = -ENOMEM;
			goto err_spi_dir;
		}
	} else {
		txbuf = ts_data->bus_tx_buf;
		rxbuf = ts_data->bus_rx_buf;
		memset(txbuf, 0x0, SPI_BUF_LENGTH);
		memset(rxbuf, 0x0, SPI_BUF_LENGTH);
	}

	memcpy(txbuf, writebuf, writelen);
	ret = fts_spi_transfer(ts_data->ft_spi, txbuf, rxbuf, txlen);
	if (ret < 0) {
		TPD_INFO("data read(addr:%x) fail,status:%x,ret:%d", txbuf[0], rxbuf[3], ret);
		goto err_spi_dir;
	}

	if (read_cmd) {
		memcpy(readbuf, rxbuf, txlen);
	}

	ret = 0;
err_spi_dir:
	if (txlen > SPI_BUF_LENGTH) {
		if (txbuf) {
			kfree(txbuf);
			txbuf = NULL;
		}

		if (rxbuf) {
			kfree(rxbuf);
			rxbuf = NULL;
		}
	}

	udelay(CS_HIGH_DELAY);
	mutex_unlock(&ts_data->bus_lock);
	return ret;
}

int fts_spi_write_direct(u8 *writebuf, u32 writelen)
{
	int ret = 0;
	u8 *readbuf = NULL;

	ret = fts_spi_transfer_direct(writebuf, writelen, readbuf, 0);
	if (ret < 0)
		return ret;
	else
		return 0;
}

int fts_spi_read_direct(u8 *writebuf, u32 writelen, u8 *readbuf, u32 readlen)
{
	int ret = 0;

	ret = fts_spi_transfer_direct(writebuf, writelen, readbuf, readlen);
	if (ret < 0)
		return ret;
	else
		return 0;
}

static int fts_bus_init(struct chip_data_ft8057p *ts_data)
{
	ts_data->bus_tx_buf = kzalloc(SPI_BUF_LENGTH, GFP_KERNEL);
	if (NULL == ts_data->bus_tx_buf) {
		TPD_INFO("failed to allocate memory for bus_tx_buf");
		return -ENOMEM;
	}

	ts_data->bus_rx_buf = kzalloc(SPI_BUF_LENGTH, GFP_KERNEL);
	if (NULL == ts_data->bus_rx_buf) {
		kfree(ts_data->bus_tx_buf);
		ts_data->bus_tx_buf = NULL;
		TPD_INFO("failed to allocate memory for bus_rx_buf");
		return -ENOMEM;
	}

	mutex_init(&ts_data->bus_lock);
	return 0;
}

static int fts_bus_exit(struct chip_data_ft8057p *ts_data)
{
	if (ts_data && ts_data->bus_tx_buf) {
		kfree(ts_data->bus_tx_buf);
		ts_data->bus_tx_buf = NULL;
	}

	if (ts_data && ts_data->bus_rx_buf) {
		kfree(ts_data->bus_rx_buf);
		ts_data->bus_rx_buf = NULL;
	}
	return 0;
}



/*********************************************************
 *              proc/ftxxxx-debug                        *
 *********************************************************/
#define PROC_READ_REGISTER                      1
#define PROC_WRITE_REGISTER                     2
#define PROC_WRITE_DATA                         6
#define PROC_READ_DATA                          7
#define PROC_SET_TEST_FLAG                      8
#define PROC_HW_RESET                           11
#define PROC_READ_STATUS                        12
#define PROC_SET_BOOT_MODE                      13
#define PROC_ENTER_TEST_ENVIRONMENT             14
#define PROC_WRITE_DATA_DIRECT                  16
#define PROC_READ_DATA_DIRECT                   17
#define PROC_CONFIGURE                          18
#define PROC_CONFIGURE_INTR                     20
#define PROC_GET_DRIVER_INFO                    21
#define PROC_NAME                               "ftxxxx-debug"
#define PROC_BUF_SIZE                           256

static ssize_t fts_debug_write(struct file *filp, const char __user *buff, size_t count, loff_t *ppos)
{
	u8 *writebuf = NULL;
	u8 tmpbuf[PROC_BUF_SIZE] = { 0 };
	int buflen = count;
	int writelen = 0;
	int ret = 0;
	char tmp[PROC_BUF_SIZE];
	struct chip_data_ft8057p *ts_data = PDE_DATA(file_inode(filp));
	struct ftxxxx_proc *proc = &ts_data->proc;

	if (buflen < 1) {
		TPD_INFO("apk proc wirte count(%d) fail", buflen);
		return -EINVAL;
	}

	if (buflen > PROC_BUF_SIZE) {
		writebuf = (u8 *)kzalloc(buflen * sizeof(u8), GFP_KERNEL);
		if (NULL == writebuf) {
			TPD_INFO("apk proc wirte buf zalloc fail");
			return -ENOMEM;
		}
	} else {
		writebuf = tmpbuf;
	}

	if (copy_from_user(writebuf, buff, buflen)) {
		TPD_INFO("[APK]: copy from user error!!");
		ret = -EFAULT;
		goto proc_write_err;
	}

	proc->opmode = writebuf[0];
	if (buflen == 1) {
		ret = buflen;
		goto proc_write_err;
	}

	switch (proc->opmode) {
	case PROC_SET_TEST_FLAG:
		TPD_INFO("[APK]: PROC_SET_TEST_FLAG = %x", writebuf[1]);
		if (writebuf[1] == 0) {
			focal_esd_check_enable(ts_data, true);
		} else {
			focal_esd_check_enable(ts_data, false);
		}
		break;

	case PROC_READ_REGISTER:
		proc->cmd[0] = writebuf[1];
		break;

	case PROC_WRITE_REGISTER:
		ret = fts_write_reg(writebuf[1], writebuf[2]);
		if (ret < 0) {
			TPD_INFO("PROC_WRITE_REGISTER write error");
			goto proc_write_err;
		}
		break;

	case PROC_READ_DATA:
		writelen = buflen - 1;
		if (writelen >= FTS_MAX_COMMMAND_LENGTH) {
			TPD_INFO("cmd(PROC_READ_DATA) length(%d) fail", writelen);
			goto proc_write_err;
		}
		memcpy(proc->cmd, writebuf + 1, writelen);
		proc->cmd_len = writelen;
		break;

	case PROC_WRITE_DATA:
		writelen = buflen - 1;
		ret = fts_write(writebuf + 1, writelen);
		if (ret < 0) {
			TPD_INFO("PROC_WRITE_DATA write error");
			goto proc_write_err;
		}
		break;

	case PROC_HW_RESET:
		if (buflen < PROC_BUF_SIZE) {
			snprintf(tmp, PROC_BUF_SIZE, "%s", writebuf + 1);
			tmp[buflen - 1] = '\0';
			if (strncmp(tmp, "focal_driver", 12) == 0) {
				TPD_INFO("APK execute HW Reset");
				fts_hw_reset(ts_data, 0);
			}
		}
		break;

	case PROC_READ_DATA_DIRECT:
		writelen = buflen - 1;
		if (writelen >= FTS_MAX_COMMMAND_LENGTH) {
			TPD_INFO("cmd(PROC_READ_DATA_DIRECT) length(%d) fail", writelen);
			goto proc_write_err;
		}
		memcpy(proc->cmd, writebuf + 1, writelen);
		proc->cmd_len = writelen;
		break;

	case PROC_WRITE_DATA_DIRECT:
		writelen = buflen - 1;
		ret = fts_spi_transfer_direct(writebuf + 1, writelen, NULL, 0);
		if (ret < 0) {
			TPD_INFO("PROC_WRITE_DATA_DIRECT write error");
			goto proc_write_err;
		}
		break;

	case PROC_CONFIGURE:
		ts_data->ft_spi->mode = writebuf[1];
		ts_data->ft_spi->bits_per_word = writebuf[2];
		ts_data->ft_spi->max_speed_hz = *(u32 *)(writebuf + 4);
		TPD_INFO("spi,mode=%d,bits=%d,speed=%d", ts_data->ft_spi->mode,
		         ts_data->ft_spi->bits_per_word, ts_data->ft_spi->max_speed_hz);
		ret = spi_setup(ts_data->ft_spi);
		if (ret) {
			TPD_INFO("spi setup fail");
			goto proc_write_err;
		}
		break;

	case PROC_CONFIGURE_INTR:
		if (writebuf[1] == 0)
			disable_irq_nosync(ts_data->ts->irq);
		else
			enable_irq(ts_data->ts->irq);
		break;

	default:
		break;
	}

	ret = buflen;
proc_write_err:
	if ((buflen > PROC_BUF_SIZE) && writebuf) {
		kfree(writebuf);
		writebuf = NULL;
	}

	return ret;
}

static ssize_t fts_debug_read(struct file *filp, char __user *buff, size_t count, loff_t *ppos)
{
	int ret = 0;
	int num_read_chars = 0;
	int buflen = count;
	u8 *readbuf = NULL;
	u8 tmpbuf[PROC_BUF_SIZE] = { 0 };
	struct chip_data_ft8057p *ts_data = PDE_DATA(file_inode(filp));
	struct ftxxxx_proc *proc = &ts_data->proc;

	if (buflen <= 0) {
		TPD_INFO("apk proc read count(%d) fail", buflen);
		return -EINVAL;
	}

	if (buflen > PROC_BUF_SIZE) {
		readbuf = (u8 *)kzalloc(buflen * sizeof(u8), GFP_KERNEL);
		if (NULL == readbuf) {
			TPD_INFO("apk proc wirte buf zalloc fail");
			return -ENOMEM;
		}
	} else {
		readbuf = tmpbuf;
	}

	switch (proc->opmode) {
	case PROC_READ_REGISTER:
		num_read_chars = 1;
		ret = fts_read_reg(proc->cmd[0], &readbuf[0]);
		if (ret < 0) {
			TPD_INFO("PROC_READ_REGISTER read error");
			goto proc_read_err;
		}
		break;

	case PROC_READ_DATA:
		num_read_chars = buflen;
		ret = fts_read(proc->cmd, proc->cmd_len, readbuf, num_read_chars);
		if (ret < 0) {
			TPD_INFO("PROC_READ_DATA read error");
			goto proc_read_err;
		}
		break;

	case PROC_READ_DATA_DIRECT:
		num_read_chars = buflen;
		ret = fts_spi_transfer_direct(proc->cmd, proc->cmd_len, readbuf, num_read_chars);
		if (ret < 0) {
			TPD_INFO("PROC_READ_DATA_DIRECT read error");
			goto proc_read_err;
		}
		break;

	case PROC_GET_DRIVER_INFO:
		if (buflen >= 64) {
			num_read_chars = buflen;
			readbuf[0] = 3;
			snprintf(&readbuf[32], buflen - 32, "Focaltech V3.4 20211214");
		}
		break;

	default:
		break;
	}

	ret = num_read_chars;
proc_read_err:
	if ((num_read_chars > 0) && copy_to_user(buff, readbuf, num_read_chars)) {
		TPD_INFO("copy to user error");
		ret = -EFAULT;
	}

	if ((buflen > PROC_BUF_SIZE) && readbuf) {
		kfree(readbuf);
		readbuf = NULL;
	}

	return ret;
}


static int fts_ta_open(struct inode *inode, struct file *file)
{
	struct chip_data_ft8057p *ts_data = PDE_DATA(inode);

	if (ts_data->touch_analysis_support) {
		TPD_INFO("fts_ta open");
		ts_data->ta_buf = kzalloc(FTS_MAX_TOUCH_BUF, GFP_KERNEL);
		if (!ts_data->ta_buf) {
			TPD_INFO("kzalloc for ta_buf fails");
			return -ENOMEM;
		}
	}
	return 0;
}

static int fts_ta_release(struct inode *inode, struct file *file)
{
	struct chip_data_ft8057p *ts_data = PDE_DATA(inode);

	if (ts_data->touch_analysis_support) {
		TPD_INFO("fts_ta close");
		ts_data->ta_flag = 0;
		if (ts_data->ta_buf) {
			kfree(ts_data->ta_buf);
			ts_data->ta_buf = NULL;
		}
	}
	return 0;
}

static ssize_t fts_ta_read(struct file *filp, char __user *buff, size_t count, loff_t *ppos)
{
	int read_num = (int)count;
	struct chip_data_ft8057p *ts_data = PDE_DATA(file_inode(filp));

	if (!ts_data->touch_analysis_support || !ts_data->ta_buf) {
		TPD_INFO("touch_analysis is disabled, or ta_buf is NULL");
		return -EINVAL;
	}

	if (!(filp->f_flags & O_NONBLOCK)) {
		ts_data->ta_flag = 1;
		wait_event_interruptible(ts_data->ts_waitqueue, !ts_data->ta_flag);
	}

	read_num = (ts_data->ta_size < read_num) ? ts_data->ta_size : read_num;
	if ((read_num > 0) && (copy_to_user(buff, ts_data->ta_buf, read_num))) {
		TPD_INFO("copy to user error");
		return -EFAULT;
	}

	return read_num;
}

static const struct file_operations fts_procta_fops = {
	.open = fts_ta_open,
	.release = fts_ta_release,
	.read = fts_ta_read,
};

DECLARE_PROC_OPS(fts_proc_fops, simple_open, fts_debug_read, fts_debug_write, NULL);
static int fts_create_apk_debug_channel(struct chip_data_ft8057p *ts_data)
{
	struct ftxxxx_proc *proc = &ts_data->proc;

	proc->proc_entry = proc_create_data(PROC_NAME, 0777, NULL, &fts_proc_fops, ts_data);
	if (NULL == proc->proc_entry) {
		TPD_INFO("create proc entry fail");
		return -ENOMEM;
	}

	ts_data->proc_ta.proc_entry = proc_create_data("fts_ta", 0777, NULL, \
	                              &fts_proc_fops, ts_data);
	if (!ts_data->proc_ta.proc_entry) {
		TPD_INFO("create proc_ta entry fail");
		return -ENOMEM;
	}
	TPD_INFO("Create proc entry success!");
	return 0;
}

static void fts_release_apk_debug_channel(struct chip_data_ft8057p *ts_data)
{
	struct ftxxxx_proc *proc = &ts_data->proc;

	if (proc->proc_entry) {
		proc_remove(proc->proc_entry);
	}

	if (ts_data->proc_ta.proc_entry)
		proc_remove(ts_data->proc_ta.proc_entry);
}


static ssize_t fts_prc_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct touchpanel_data *ts = dev_get_drvdata(dev);
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)ts->chip_data;

	mutex_lock(&ts->mutex);
	if (buf[0] == '1') {
		TPD_INFO("enable prc");
		ts_data->prc_support = 1;
	} else if (buf[0] == '0') {
		TPD_INFO("disable prc");
		cancel_delayed_work_sync(&ts_data->prc_work);
		ts_data->prc_support = 0;
	}
	mutex_unlock(&ts->mutex);

	return count;
}

static ssize_t fts_prc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;
	struct touchpanel_data *ts = dev_get_drvdata(dev);
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)ts->chip_data;

	count = snprintf(buf, PAGE_SIZE, "PRC: %s\n", \
	                 ts_data->prc_support ? "Enable" : "Disable");

	return count;
}


/* fts_touch_size node */
static ssize_t fts_touchsize_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count = 0;
	struct touchpanel_data *ts = dev_get_drvdata(dev);
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)ts->chip_data;

	count += snprintf(buf + count, PAGE_SIZE, "touch size:%d\n", ts_data->touch_size);

	return count;
}

static ssize_t fts_touchsize_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value = 0;
	struct touchpanel_data *ts = dev_get_drvdata(dev);
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)ts->chip_data;

	sscanf(buf, "%d", &value);
	if ((value > 2) && (value < FTS_MAX_TOUCH_BUF)) {
		TPD_INFO("touch size:%d->%d", ts_data->touch_size, value);
		ts_data->touch_size = value;
	} else
		TPD_INFO("touch size:%d invalid", value);

	return count;
}

/* fts_ta_mode node */
static ssize_t fts_tamode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count = 0;
	struct touchpanel_data *ts = dev_get_drvdata(dev);
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)ts->chip_data;

	count += snprintf(buf + count, PAGE_SIZE, "touch analysis:%s\n", \
	                  ts_data->touch_analysis_support ? "Enable" : "Disable");

	return count;
}

static ssize_t fts_tamode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value = 0;
	struct touchpanel_data *ts = dev_get_drvdata(dev);
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)ts->chip_data;

	sscanf(buf, "%d", &value);
	ts_data->touch_analysis_support = !!value;
	TPD_INFO("set touch analysis:%d", ts_data->touch_analysis_support);

	return count;
}

static DEVICE_ATTR(fts_prc, S_IRUGO | S_IWUSR, fts_prc_show, fts_prc_store);
static DEVICE_ATTR(fts_touch_size, S_IRUGO | S_IWUSR, fts_touchsize_show, fts_touchsize_store);
static DEVICE_ATTR(fts_ta_mode, S_IRUGO | S_IWUSR, fts_tamode_show, fts_tamode_store);

/* add your attr in here*/
static struct attribute *fts_attributes[] = {
	&dev_attr_fts_prc.attr,
	&dev_attr_fts_touch_size.attr,
	&dev_attr_fts_ta_mode.attr,
	NULL
};

static struct attribute_group fts_attribute_group = {
	.attrs = fts_attributes
};

static int fts_create_sysfs(struct chip_data_ft8057p *ts_data)
{
	int ret = 0;

	ret = sysfs_create_group(&ts_data->ts->dev->kobj, &fts_attribute_group);
	if (ret) {
		TPD_INFO("[EX]: sysfs_create_group() failed!!");
		sysfs_remove_group(&ts_data->ts->dev->kobj, &fts_attribute_group);
		return -ENOMEM;
	} else {
		TPD_INFO("[EX]: sysfs_create_group() succeeded!!");
	}

	return ret;
}

static int fts_remove_sysfs(struct chip_data_ft8057p *ts_data)
{
	sysfs_remove_group(&ts_data->ts->dev->kobj, &fts_attribute_group);
	return 0;
}




/*******Part1:Call Back Function implement*******/

static int fts_rstgpio_set(struct hw_resource *hw_res, bool on)
{
	if (gpio_is_valid(hw_res->reset_gpio)) {
		TPD_INFO("Set the reset_gpio \n");
		gpio_direction_output(hw_res->reset_gpio, on);

	} else {
		TPD_INFO("reset is invalid!!\n");
	}

	return 0;
}

/*
 * return success: 0; fail : negative
 */
static int fts_hw_reset(struct chip_data_ft8057p *ts_data, u32 delayms)
{
	TPD_INFO("%s.\n", __func__);
	fts_rstgpio_set(ts_data->hw_res, false); /* reset gpio*/
	msleep(5);
	fts_rstgpio_set(ts_data->hw_res, true); /* reset gpio*/

	if (delayms) {
		msleep(delayms);
	}

	return 0;
}


static int fts_fwupg_hardware_reset_to_boot(void)
{
	fts_reset_proc(0);
	return 0;
}


static int fts_check_bootid(void)
{
	int ret = 0;
	u8 cmd = 0;
	u8 id[2] = { 0 };

	cmd = FTS_CMD_READ_ID;
	ret = fts_read(&cmd, 1, id, 2);
	if (ret < 0) {
		TPD_INFO("read boot id(0x%02x 0x%02x) fail", id[0], id[1]);
		return ret;
	}

	TPD_INFO("read boot id:0x%02x 0x%02x", id[0], id[1]);
	if ((FTS_VAL_BT_ID == id[0]) && (FTS_VAL_BT_ID2 == id[1])) {
		return 0;
	}
	return -EIO;
}


static int fts_power_control(void *chip_data, bool enable)
{
	/*For IDC, power on sequences are done in LCD driver*/
	return 0;
}

static int focal_dump_reg_state(void *chip_data, char *buf)
{
	int count = 0;
	u8 regvalue = 0;

	/*power mode 0:active 1:monitor 3:sleep*/
	fts_read_reg(FTS_REG_POWER_MODE, &regvalue);
	count += sprintf(buf + count, "Power Mode:0x%02x\n", regvalue);

	/*FW version*/
	fts_read_reg(FTS_REG_FW_VER, &regvalue);
	count += sprintf(buf + count, "FW Ver:0x%02x\n", regvalue);

	/*Vendor ID*/
	fts_read_reg(FTS_REG_VENDOR_ID, &regvalue);
	count += sprintf(buf + count, "Vendor ID:0x%02x\n", regvalue);

	/* 1 Gesture mode,0 Normal mode*/
	fts_read_reg(FTS_REG_GESTURE_EN, &regvalue);
	count += sprintf(buf + count, "Gesture Mode:0x%02x\n", regvalue);

	/* 3 charge in*/
	fts_read_reg(FTS_REG_CHARGER_MODE_EN, &regvalue);
	count += sprintf(buf + count, "charge stat:0x%02x\n", regvalue);

	/*Interrupt counter*/
	fts_read_reg(FTS_REG_INT_CNT, &regvalue);
	count += sprintf(buf + count, "INT count:0x%02x\n", regvalue);

	/*Flow work counter*/
	fts_read_reg(FTS_REG_FLOW_WORK_CNT, &regvalue);
	count += sprintf(buf + count, "ESD count:0x%02x\n", regvalue);

	return count;
}

static int focal_get_fw_version(void *chip_data)
{
	u8 fw_ver = 0;

	fts_read_reg(FTS_REG_FW_VER, &fw_ver);
	return (int)fw_ver;
}

static void focal_esd_check_enable(void *chip_data, bool enable)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	ts_data->esd_check_enabled = enable;
}

static bool focal_get_esd_check_flag(void *chip_data)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	return ts_data->esd_check_need_stop;
}

static int fts_esd_handle(void *chip_data)
{
	int ret = -1;
	int i = 0;
	static int flow_work_cnt_last = 0;
	static int err_cnt = 0;
	static int i2c_err = 0;
	u8 val = 0xFF;
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;

	if (!ts_data->esd_check_enabled) {
		goto NORMAL_END;
	}

	ret = fts_read_reg(0x00, &val);

	if ((ret & 0x70) == 0x40) { /*work in factory mode*/
		goto NORMAL_END;
	}

	for (i = 0; i < 3; i++) {
		ret = fts_read_reg(FTS_REG_CHIP_ID, &val);

		if (val != FTS_VAL_CHIP_ID) {
			TPD_INFO("%s: read chip_id failed!(ret:%x)\n", __func__, ret);
			msleep(10);
			i2c_err++;

		} else {
			i2c_err = 0;
			break;
		}
	}

	ret = fts_read_reg(FTS_REG_FLOW_WORK_CNT, &val);

	if (ret < 0) {
		TPD_INFO("%s: read FTS_REG_FLOW_WORK_CNT failed!\n", __func__);
		i2c_err++;
	}

	if (flow_work_cnt_last == val) {
		err_cnt++;

	} else {
		err_cnt = 0;
	}

	flow_work_cnt_last = ret;

	if ((err_cnt >= 5) || (i2c_err >= 3)) {
		TPD_INFO("esd check failed, start reset!\n");
		disable_irq_nosync(ts_data->ts->irq);
		tp_touch_btnkey_release(ts_data->tp_index);
		fts_hw_reset(ts_data, RESET_TO_NORMAL_TIME);
		enable_irq(ts_data->ts->irq);
		flow_work_cnt_last = 0;
		err_cnt = 0;
		i2c_err = 0;
	}

NORMAL_END:
	return 0;
}


static void fts_release_all_finger(struct touchpanel_data *ts)
{
#ifdef TYPE_B_PROTOCOL
	int i = 0;

	if (!ts->touch_count || !ts->irq_slot)
		return;

	mutex_lock(&ts->report_mutex);
	for (i = 0; i < ts->max_num; i++) {
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
	}
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, 0);
	input_sync(ts->input_dev);
	mutex_unlock(&ts->report_mutex);
	TPD_INFO("fts_release_all_finger");
	ts->view_area_touched = 0; /*realse all touch point,must clear this flag*/
	ts->touch_count = 0;
	ts->irq_slot = 0;
#endif
}

static void fts_prc_func(struct work_struct *work)
{
	struct chip_data_ft8057p *ts_data = container_of(work,
	                                   struct chip_data_ft8057p, prc_work.work);
	unsigned long cur_jiffies = jiffies;
	unsigned long intr_timeout = msecs_to_jiffies(PRC_INTR_INTERVALS);

	if (ts_data->prc_support && !ts_data->ts->is_suspended) {
		intr_timeout += ts_data->intr_jiffies;
		if (time_after(cur_jiffies, intr_timeout)) {
			if (ts_data->ts->touch_count && ts_data->ts->irq_slot) {
				fts_release_all_finger(ts_data->ts);
				TPD_INFO("prc trigger interval:%dms",
				         jiffies_to_msecs(cur_jiffies - ts_data->intr_jiffies));
			}
			ts_data->prc_mode = 0;
		} else {
			queue_delayed_work(ts_data->ts_workqueue, &ts_data->prc_work,
			                   msecs_to_jiffies(POINT_REPORT_CHECK_WAIT_TIME));
			ts_data->prc_mode = 1;
		}
	} else {
		ts_data->prc_mode = 0;
	}
}

static void fts_prc_queue_work(struct chip_data_ft8057p *ts_data)
{
	ts_data->intr_jiffies = jiffies;
	if (ts_data->prc_support && !ts_data->prc_mode && !ts_data->ts->is_suspended) {
		queue_delayed_work(ts_data->ts_workqueue, &ts_data->prc_work,
		                   msecs_to_jiffies(POINT_REPORT_CHECK_WAIT_TIME));
		ts_data->prc_mode = 1;
	}
}



static int fts_point_report_check_init(struct chip_data_ft8057p *ts_data)
{
	TPD_INFO("point check init");

	if (ts_data->ts_workqueue) {
		INIT_DELAYED_WORK(&ts_data->prc_work, fts_prc_func);
	} else {
		TPD_INFO("fts workqueue is NULL, can't run point report check function");
		return -EINVAL;
	}

	ts_data->prc_support = 1;
	return 0;
}

static int fts_point_report_check_exit(struct chip_data_ft8057p *ts_data)
{
	TPD_INFO("point check exit");
	cancel_delayed_work_sync(&ts_data->prc_work);
	return 0;
}

static int fts_dpram_write_pe(u32 saddr, const u8 *buf, u32 len, bool wpram)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	u8 *cmd = NULL;
	u32 addr = 0;
	u32 offset = 0;
	u32 remainder = 0;
	u32 packet_number = 0;
	u32 packet_len = 0;
	u32 packet_size = FTS_FLASH_PACKET_LENGTH_SPI;
	bool fd_support = true;
	struct chip_data_ft8057p *fts_data = g_fts_data;

	TPD_INFO("dpram write");
	if (!fts_data || !fts_data->setting_nf) {
		TPD_INFO("upgrade/ts_data/setting_nf is null");
		return -EINVAL;
	}

	if (!buf) {
		TPD_INFO("fw buf is null");
		return -EINVAL;
	}

	if ((len < FTS_MIN_LEN) || (len > fts_data->setting_nf->app2_offset)) {
		TPD_INFO("fw length(%d) fail", len);
		return -EINVAL;
	}

	cmd = vmalloc(packet_size + FTS_CMD_WRITE_LEN + 1);
	if (NULL == cmd) {
		TPD_INFO("malloc memory for pram write buffer fail");
		return -ENOMEM;
	}
	memset(cmd, 0, packet_size + FTS_CMD_WRITE_LEN + 1);

	packet_number = len / packet_size;
	remainder = len % packet_size;
	if (remainder > 0)
		packet_number++;
	packet_len = packet_size;
	TPD_INFO("write data, num:%d remainder:%d", packet_number, remainder);

	cmd[0] = FTS_ROMBOOT_CMD_WRITE;
	for (i = 0; i < packet_number; i++) {
		offset = i * packet_size;
		addr = saddr + offset;
		cmd[1] = BYTE_OFF_16(addr);
		cmd[2] = BYTE_OFF_8(addr);
		cmd[3] = BYTE_OFF_0(addr);

		/* last packet */
		if ((i == (packet_number - 1)) && remainder)
			packet_len = remainder;
		cmd[4] = BYTE_OFF_8(packet_len);
		cmd[5] = BYTE_OFF_0(packet_len);

		for (j = 0; j < packet_len; j++) {
			cmd[FTS_CMD_WRITE_LEN + j] = buf[offset + j];
		}

		ret = fts_write(&cmd[0], FTS_CMD_WRITE_LEN + packet_len);
		if (ret < 0) {
			TPD_INFO("write fw to pram(%d) fail", i);
			goto write_pram_err;
		}

		if (!fd_support)
			mdelay(3);
	}

write_pram_err:
	if (cmd) {
		vfree(cmd);
		cmd = NULL;
	}
	return ret;
}

static int fts_dpram_write(u32 saddr, const u8 *buf, u32 len, bool wpram)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	u8 *cmd = NULL;
	u32 addr = 0;
	u32 baseaddr = wpram ? FTS_PRAM_SADDR : FTS_DRAM_SADDR;
	u32 offset = 0;
	u32 remainder = 0;
	u32 packet_number = 0;
	u32 packet_len = 0;
	u32 packet_size = FTS_FLASH_PACKET_LENGTH_SPI;
	struct chip_data_ft8057p *fts_data = g_fts_data;

	TPD_INFO("dpram write");
	if (!fts_data  || !fts_data->setting_nf) {
		TPD_INFO("upgrade/ts_data/setting_nf is null");
		return -EINVAL;
	}

	if (!buf) {
		TPD_INFO("fw buf is null");
		return -EINVAL;
	}

	if ((len < FTS_MIN_LEN) || (len > fts_data->setting_nf->app2_offset)) {
		TPD_INFO("fw length(%d) fail", len);
		return -EINVAL;
	}

	cmd = vmalloc(packet_size + FTS_CMD_WRITE_LEN + 1);
	if (NULL == cmd) {
		TPD_INFO("malloc memory for pram write buffer fail");
		return -ENOMEM;
	}
	memset(cmd, 0, packet_size + FTS_CMD_WRITE_LEN + 1);

	packet_number = len / packet_size;
	remainder = len % packet_size;
	if (remainder > 0)
		packet_number++;
	packet_len = packet_size;
	TPD_INFO("write data, num:%d remainder:%d", packet_number, remainder);

	for (i = 0; i < packet_number; i++) {
		offset = i * packet_size;
		addr = saddr + offset + baseaddr;
		/* last packet */
		if ((i == (packet_number - 1)) && remainder)
			packet_len = remainder;

		/* set pram address */
		cmd[0] = FTS_ROMBOOT_CMD_SET_PRAM_ADDR;
		cmd[1] = BYTE_OFF_16(addr);
		cmd[2] = BYTE_OFF_8(addr);
		cmd[3] = BYTE_OFF_0(addr);
		ret = fts_write(&cmd[0], FTS_ROMBOOT_CMD_SET_PRAM_ADDR_LEN);
		if (ret < 0) {
			TPD_INFO("set pram(%d) addr(%d) fail", i, addr);
			goto write_pram_err;
		}

		/* write pram data */
		cmd[0] = FTS_ROMBOOT_CMD_WRITE;
		for (j = 0; j < packet_len; j++) {
			cmd[1 + j] = buf[offset + j];
		}
		ret = fts_write(&cmd[0], 1 + packet_len);
		if (ret < 0) {
			TPD_INFO("write fw to pram(%d) fail", i);
			goto write_pram_err;
		}
	}

write_pram_err:
	if (cmd) {
		vfree(cmd);
		cmd = NULL;
	}
	return ret;
}

static int fts_ecc_cal_tp(u32 ecc_saddr, u32 ecc_len, u16 *ecc_value)
{
	int ret = 0;
	int i = 0;
	u8 cmd[FTS_ROMBOOT_CMD_ECC_NEW_LEN] = { 0 };
	u8 value[2] = { 0 };
	struct chip_data_ft8057p *fts_data = g_fts_data;

	TPD_INFO("ecc calc in tp");
	if (!fts_data || !fts_data->setting_nf) {
		TPD_INFO("upgrade/ts_data/setting_nf is null");
		return -EINVAL;
	}

	cmd[0] = FTS_ROMBOOT_CMD_ECC;
	cmd[1] = BYTE_OFF_16(ecc_saddr);
	cmd[2] = BYTE_OFF_8(ecc_saddr);
	cmd[3] = BYTE_OFF_0(ecc_saddr);
	cmd[4] = BYTE_OFF_16(ecc_len);
	cmd[5] = BYTE_OFF_8(ecc_len);
	cmd[6] = BYTE_OFF_0(ecc_len);

	/* make boot to calculate ecc in pram */
	ret = fts_write(cmd, FTS_ROMBOOT_CMD_ECC_NEW_LEN);
	if (ret < 0) {
		TPD_INFO("ecc calc cmd fail");
		return ret;
	}
	mdelay(2);

	/* wait boot calculate ecc finish */
	if (fts_data->setting_nf->ecc_delay) {
		mdelay(fts_data->setting_nf->ecc_delay);
	} else {
		cmd[0] = FTS_ROMBOOT_CMD_ECC_FINISH;
		for (i = 0; i < FTS_ECC_FINISH_TIMEOUT; i++) {
			ret = fts_read(cmd, 1, value, 1);
			if (ret < 0) {
				TPD_INFO("ecc finish cmd fail");
				return ret;
			}
			if (fts_data->setting_nf->eccok_val == value[0])
				break;
			mdelay(1);
		}
		if (i >= FTS_ECC_FINISH_TIMEOUT) {
			TPD_INFO("wait ecc finish timeout,ecc_finish=%x", value[0]);
			return -EIO;
		}
	}

	/* get ecc value calculate in boot */
	cmd[0] = FTS_ROMBOOT_CMD_ECC_READ;
	ret = fts_read(cmd, 1, value, 2);
	if (ret < 0) {
		TPD_INFO("ecc read cmd fail");
		return ret;
	}

	*ecc_value = ((u16)(value[0] << 8) + value[1]) & 0x0000FFFF;
	return 0;
}

static int fts_ecc_cal_host(const u8 *data, u32 data_len, u16 *ecc_value)
{
	u16 ecc = 0;
	u32 i = 0;
	u32 j = 0;
	u16 al2_fcs_coef = AL2_FCS_COEF;

	for (i = 0; i < data_len; i += 2) {
		ecc ^= ((data[i] << 8) | (data[i + 1]));
		for (j = 0; j < 16; j ++) {
			if (ecc & 0x01)
				ecc = (u16)((ecc >> 1) ^ al2_fcs_coef);
			else
				ecc >>= 1;
		}
	}

	*ecc_value = ecc & 0x0000FFFF;
	return 0;
}

static int fts_ecc_check(const u8 *buf, u32 len, u32 ecc_saddr)
{
	int ret = 0;
	int i = 0;
	u16 ecc_in_host = 0;
	u16 ecc_in_tp = 0;
	int packet_length = 0;
	int packet_number = 0;
	int packet_remainder = 0;
	int offset = 0;
	u32 packet_size = FTS_MAX_LEN_FILE;
	struct chip_data_ft8057p *fts_data = g_fts_data;

	TPD_INFO("ecc check");
	if (!fts_data || !fts_data->setting_nf) {
		TPD_INFO("upgrade/ts_data/setting_nf is null");
		return -EINVAL;
	}

	if (fts_data->setting_nf->ecclen_max) {
		packet_size = fts_data->setting_nf->ecclen_max;
	}

	packet_number = len / packet_size;
	packet_remainder = len % packet_size;
	if (packet_remainder)
		packet_number++;
	packet_length = packet_size;

	for (i = 0; i < packet_number; i++) {
		/* last packet */
		if ((i == (packet_number - 1)) && packet_remainder)
			packet_length = packet_remainder;

		ret = fts_ecc_cal_host(buf + offset, packet_length, &ecc_in_host);
		if (ret < 0) {
			TPD_INFO("ecc in host calc fail");
			return ret;
		}

		ret = fts_ecc_cal_tp(ecc_saddr + offset, packet_length, &ecc_in_tp);
		if (ret < 0) {
			TPD_INFO("ecc in tp calc fail");
			return ret;
		}

		TPD_DEBUG("ecc in tp:%04x,host:%04x,i:%d", ecc_in_tp, ecc_in_host, i);
		if (ecc_in_tp != ecc_in_host) {
			TPD_INFO("ecc_in_tp(%x) != ecc_in_host(%x), ecc check fail",
					ecc_in_tp, ecc_in_host);
			return -EIO;
		}

		offset += packet_length;
	}

	return 0;
}

static int fts_pram_write_ecc(const u8 *buf, u32 len)
{
	int ret = 0;
	u32 pram_app_size = 0;
	u16 code_len = 0;
	u16 code_len_n = 0;
	u32 pram_start_addr = 0;
	struct chip_data_ft8057p *fts_data = g_fts_data;

	TPD_INFO("begin to write pram app(bin len:%d)", len);
	if (!fts_data || !fts_data->setting_nf) {
		TPD_INFO("upgrade/setting_nf is null");
		return -EINVAL;
	}

	/* get pram app length */
	code_len = ((u16)buf[FTS_APP_INFO_OFFSET + 0] << 8)
				+ buf[FTS_APP_INFO_OFFSET + 1];
	code_len_n = ((u16)buf[FTS_APP_INFO_OFFSET + 2] << 8)
				+ buf[FTS_APP_INFO_OFFSET + 3];
	if ((code_len + code_len_n) != 0xFFFF) {
		TPD_INFO("pram code len(%x %x) fail", code_len, code_len_n);
		return -EINVAL;
	}

	pram_app_size = ((u32)code_len) * fts_data->setting_nf->length_coefficient;
	TPD_INFO("pram app length in fact:%d", pram_app_size);

	/* write pram */
	if (fts_data->setting_nf->spi_pe)
		ret = fts_dpram_write_pe(pram_start_addr, buf, pram_app_size, true);
	else
		ret = fts_dpram_write(pram_start_addr, buf, pram_app_size, true);
	if (ret < 0) {
		TPD_INFO("write pram fail");
		return ret;
	}

	/* check ecc */
	ret = fts_ecc_check(buf, pram_app_size, pram_start_addr);
	if (ret < 0) {
		TPD_INFO("pram ecc check fail");
		return ret;
	}

	TPD_INFO("pram app write successfully");
	return 0;
}

static int fts_dram_write_ecc(const u8 *buf, u32 len)
{
	int ret = 0;
	u32 dram_size = 0;
	u32 pram_app_size = 0;
	u32 dram_start_addr = 0;
	u16 const_len = 0;
	u16 const_len_n = 0;
	const u8 *dram_buf = NULL;
	struct chip_data_ft8057p *fts_data = g_fts_data;

	TPD_INFO("begin to write dram data(bin len:%d)", len);
	if (!fts_data || !fts_data->setting_nf) {
		TPD_INFO("upgrade/setting_nf is null");
		return -EINVAL;
	}

	/* get dram data length */
	const_len = ((u16)buf[FTS_APP_INFO_OFFSET + 0x8] << 8)
				+ buf[FTS_APP_INFO_OFFSET + 0x9];
	const_len_n = ((u16)buf[FTS_APP_INFO_OFFSET + 0x0A] << 8)
				+ buf[FTS_APP_INFO_OFFSET + 0x0B];
	if (((const_len + const_len_n) != 0xFFFF) || (const_len == 0)) {
		TPD_INFO("no support dram,const len(%x %x)", const_len, const_len_n);
		return 0;
	}

	dram_size = ((u32)const_len) * fts_data->setting_nf->length_coefficient;
	pram_app_size = ((u32)(((u16)buf[FTS_APP_INFO_OFFSET + 0] << 8)
						+ buf[FTS_APP_INFO_OFFSET + 1]));
	pram_app_size = pram_app_size * fts_data->setting_nf->length_coefficient;

	dram_buf = buf + pram_app_size;
	TPD_INFO("dram buf length in fact:%d,offset:%d", dram_size, pram_app_size);
	/* write pram */
	ret = fts_dpram_write(dram_start_addr, dram_buf, dram_size, false);
	if (ret < 0) {
		TPD_INFO("write dram fail");
		return ret;
	}

	/* check ecc */
	ret = fts_ecc_check(dram_buf, dram_size, dram_start_addr);
	if (ret < 0) {
		TPD_INFO("dram ecc check fail");
		return ret;
	}

	TPD_INFO("dram data write successfully");
	return 0;
}

int fts_enter_into_boot(void)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	u8 cmd[2] = { 0 };


	TPD_INFO("enter into boot environment");
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
		/* hardware tp reset to boot */
		fts_fwupg_hardware_reset_to_boot();
		msleep(8+i);

		/* enter into boot & check boot id*/
		for (j = 0; j < 3; j++) {
			cmd[0] = 0x55;
			ret = fts_write(cmd, 1);
			if (ret >= 0) {
				msleep(8);
				ret = fts_check_bootid();
				if (0 == ret) {
					TPD_INFO("boot id check pass, retry=%d", i);
					return 0;
				}
			}
		}
	}

	return -EIO;
}




static int ft8057p_pram_start(void)
{
	u8 cmd = FTS_ROMBOOT_CMD_START_APP;
	int ret = 0;

	TPD_INFO("remap to start pramboot");

	ret = fts_write(&cmd, 1);
	if (ret < 0) {
		TPD_INFO("write start pram cmd fail");
		return ret;
	}
	msleep(FTS_DELAY_PRAMBOOT_START);

	return 0;
}


/*
 * description: download fw to IC and run
 *
 * param - buf: const, fw data buffer
 *         len: length of fw
 *
 * return 0 if success, otherwise return error code
 */
static int fts_fw_write_start(struct chip_data_ft8057p *ts_data, const u8 *buf, u32 len, bool need_reset)
{
	int ret = 0;
	struct chip_data_ft8057p *fts_data = ts_data;

	TPD_INFO("begin to write and start fw(bin len:%d)", len);
	if (!fts_data ||  !fts_data->setting_nf) {
		TPD_INFO("upgrade/ts_data/setting_nf is null");
		return -EINVAL;
	}

	fts_data->fw_is_running = false;

	if (need_reset) {
		/* enter into boot environment */
		ret = fts_enter_into_boot();
		if (ret < 0) {
			TPD_INFO("enter into boot environment fail");
			return ret;
		}
	}

	/* write pram */
	ret = fts_pram_write_ecc(buf, len);
	if (ret < 0) {
		TPD_INFO("write pram fail");
		return ret;
	}

	if (fts_data->setting_nf->drwr_support) {
		/* write dram */
		ret = fts_dram_write_ecc(buf, len);
		if (ret < 0) {
			TPD_INFO("write dram fail");
			return ret;
		}
	}

	/* remap pram and run fw */
	ret = ft8057p_pram_start();
	if (ret < 0) {
		TPD_INFO("pram start fail");
		return ret;
	}

	fts_data->fw_is_running = true;
	TPD_INFO("fw download successfully");
	return 0;
}

#if NULL_DATA
static int fts_fwupg_enter_into_boot(struct chip_data_ft8057p *ts_data)
{
	int ret = 0;
	int i = 0;
	u8 cmd = 0;
	u8 id[2] = { 0 };

	do {
		/*reset to boot*/
		ret = fts_write_reg(FTS_REG_UPGRADE, FTS_UPGRADE_AA);

		if (ret < 0) {
			TPD_INFO("write FC=0xAA fail");
			return ret;
		}

		msleep(FTS_DELAY_UPGRADE_AA);

		ret = fts_write_reg(FTS_REG_UPGRADE, FTS_UPGRADE_55);

		if (ret < 0) {
			TPD_INFO("write FC=0x55 fail");
			return ret;
		}

		msleep(FTS_DELAY_UPGRADE_RESET);

		/*read boot id*/
		cmd = FTS_CMD_START;
		ret = fts_write(&cmd, 1);

		if (ret < 0) {
			TPD_INFO("write 0x55 fail");
			return ret;
		}

		cmd = FTS_CMD_READ_ID;
		ret = fts_read(&cmd, 1, id, 2);

		if (ret < 0) {
			TPD_INFO("read boot id fail");
			return ret;
		}

		TPD_INFO("read boot id:0x%02x%02x", id[0], id[1]);

		if ((id[0] == FTS_VAL_BT_ID) && (id[1] == FTS_VAL_BT_ID2)) {
			break;
		}
	} while (i++ < FTS_UPGRADE_LOOP);

	return 0;
}
#endif


static fw_check_state fts_fw_check(void *chip_data,
                                   struct resolution_info *resolution_info, struct panel_info *panel_data)
{
	u8 cmd = 0;
	u8 id[2] = { 0 };
	char dev_version[MAX_DEVICE_VERSION_LENGTH] = {0};
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;

	fts_read_reg(FTS_REG_CHIP_ID, &id[0]);
	fts_read_reg(FTS_REG_CHIP_ID2, &id[1]);

	if ((id[0] != FTS_VAL_CHIP_ID) || (id[1] != FTS_VAL_CHIP_ID2)) {
		cmd = 0x55;
		fts_write(&cmd, 1);
		msleep(12);
		cmd = 0x90;
		fts_read(&cmd, 1, id, 2);
		TPD_INFO("boot id:0x%02x%02x, fw abnormal", id[0], id[1]);
		return FW_ABNORMAL;
	}

	/*fw check normal need update tp_fw  && device info*/
	fts_read_reg(FTS_REG_FW_VER, &ts_data->fwver);
	panel_data->tp_fw = ts_data->fwver;
	TPD_INFO("FW VER:%d", panel_data->tp_fw);

	if (panel_data->manufacture_info.version) {
		sprintf(dev_version, "%04x", panel_data->tp_fw);
		strlcpy(&(panel_data->manufacture_info.version[7]), dev_version, 5);
	}

	return FW_NORMAL;
}

int fts_reset_proc(int hdelayms)
{
	TPD_INFO("tp reset");
	fts_rstgpio_set(g_fts_data->hw_res, false); /* reset gpio*/
	msleep(5);
	fts_rstgpio_set(g_fts_data->hw_res, true); /* reset gpio*/

	if (hdelayms) {
		msleep(hdelayms);
	}

	return 0;
}


int fts_fwupg_init(struct chip_data_ft8057p *ts_data)
{
	int i = 0;
	struct upgrade_setting_nf *setting = &upgrade_setting_list[0];
	int setting_count =
		sizeof(upgrade_setting_list) / sizeof(upgrade_setting_list[0]);

	TPD_INFO("fw upgrade init function");
	if (!ts_data || !ts_data->ts_workqueue) {
		TPD_INFO("ts_data/workqueue is NULL, can't run upgrade function");
		return -EINVAL;
	}

	if (0 == setting_count) {
		TPD_INFO("no upgrade settings in tp driver, init fail");
		return -ENODATA;
	}

	if (1 == setting_count) {
		ts_data->setting_nf = setting;
	} else {
		for (i = 0; i < setting_count; i++) {
			setting = &upgrade_setting_list[i];
			if ((setting->rom_idh == FTS_VAL_BT_ID)
				&& (setting->rom_idl == FTS_VAL_BT_ID2)) {
				TPD_INFO("match upgrade setting,type(ID):0x%02x%02x",
						setting->rom_idh, setting->rom_idl);
				ts_data->setting_nf = setting;
			}
		}
	}

	if (NULL == ts_data->setting_nf) {
		TPD_INFO("no upgrade settings match, can't upgrade");
		return -ENODATA;
	}

	return 0;
}


static int fts_fw_download(struct chip_data_ft8057p *ts_data, u8 *buf, u32 len, bool need_reset)
{
	int ret = 0;
	int i = 0;

	TPD_INFO("fw upgrade download function");
	for (i = 0; i < 3; i++) {
		TPD_INFO("fw download times:%d", i + 1);
		ret = fts_fw_write_start(ts_data, buf, len, need_reset);
		if (0 == ret)
			break;
	}
	if (i >= 3) {
		TPD_INFO("fw download fail");
		return -EIO;
	}

	return ret;
}


#define OFFSET_FW_DATA_FW_VER 0x010E
static fw_update_state fts_fw_update(void *chip_data, const struct firmware *fw,
                                     bool force)
{
	int ret = 0;
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	u8 *buf;
	u32 len = 0;

	TPD_INFO("%s: called", __func__);
	if (!fw) {
		TPD_INFO("fw is null");
		return FW_UPDATE_ERROR;
	}

	buf = (u8 *)fw->data;
	len = (int)fw->size;

	if ((len < 0x120) || (len > (256 * 1024))) {
		TPD_INFO("fw_len(%d) is invalid", len);
		return FW_UPDATE_ERROR;
	}

	focal_esd_check_enable(ts_data, false);
	ret = fts_fw_download(ts_data, buf, len, true);
	focal_esd_check_enable(ts_data, true);
	if (ret < 0) {
		TPD_INFO("fw update fail");
		return FW_UPDATE_ERROR;
	}

	return FW_UPDATE_SUCCESS;
}

static int fts_enter_factory_work_mode(struct chip_data_ft8057p *ts_data,
                                       u8 mode_val)
{
	int ret = 0;
	int retry = 20;
	u8 regval = 0;

	TPD_INFO("%s:enter %s mode", __func__, (mode_val == 0x40) ? "factory" : "work");
	ret = fts_write_reg(DEVIDE_MODE_ADDR, mode_val);

	if (ret < 0) {
		TPD_INFO("%s:write mode(val:0x%x) fail", __func__, mode_val);
		return ret;
	}

	while (--retry) {
		fts_read_reg(DEVIDE_MODE_ADDR, &regval);

		if (regval == mode_val) {
			break;
		}

		msleep(20);
	}

	if (!retry) {
		TPD_INFO("%s:enter mode(val:0x%x) timeout", __func__, mode_val);
		return -EIO;
	}

	msleep(FACTORY_TEST_DELAY);
	return 0;
}

static int fts_start_scan(struct chip_data_ft8057p *ts_data)
{
	int ret = 0;
	int retry = 50;
	u8 regval = 0;
	u8 scanval = FTS_FACTORY_MODE_VALUE | (1 << 7);

	TPD_INFO("%s: start to scan a frame", __func__);
	ret = fts_write_reg(DEVIDE_MODE_ADDR, scanval);

	if (ret < 0) {
		TPD_INFO("%s:start to scan a frame fail", __func__);
		return ret;
	}

	while (--retry) {
		fts_read_reg(DEVIDE_MODE_ADDR, &regval);

		if (regval == FTS_FACTORY_MODE_VALUE) {
			break;
		}

		msleep(20);
	}

	if (!retry) {
		TPD_INFO("%s:scan a frame timeout", __func__);
		return -EIO;
	}

	return 0;
}

static int fts_get_rawdata(struct chip_data_ft8057p *ts_data, int *raw,
                           bool is_diff)
{
	int ret = 0;
	int i = 0;
	int byte_num = ts_data->hw_res->tx_num * ts_data->hw_res->rx_num * 2;
	int size = 0;
	int packet_len = 0;
	int offset = 0;
	u8 raw_addr = 0;
	u8 regval = 0;
	u8 *buf = NULL;

	TPD_INFO("%s:call", __func__);
	/*kzalloc buffer*/
	buf = kzalloc(byte_num, GFP_KERNEL);

	if (!buf) {
		TPD_INFO("%s:kzalloc for raw byte buf fail", __func__);
		return -ENOMEM;
	}

	ret = fts_enter_factory_work_mode(ts_data, FTS_FACTORY_MODE_VALUE);

	if (ret < 0) {
		TPD_INFO("%s:enter factory mode fail", __func__);
		goto raw_err;
	}

	if (is_diff) {
		fts_read_reg(FACTORY_REG_DATA_SELECT, &regval);
		ret = fts_write_reg(FACTORY_REG_DATA_SELECT, 0x01);

		if (ret < 0) {
			TPD_INFO("%s:write 0x01 to reg0x06 fail", __func__);
			goto reg_restore;
		}
	}

	ret = fts_start_scan(ts_data);

	if (ret < 0) {
		TPD_INFO("%s:scan a frame fail", __func__);
		goto reg_restore;
	}

	ret = fts_write_reg(FACTORY_REG_LINE_ADDR, 0xAA);

	if (ret < 0) {
		TPD_INFO("%s:write 0xAA to reg0x01 fail", __func__);
		goto reg_restore;
	}

	raw_addr = FACTORY_REG_RAWDATA_ADDR_MC_SC;
	ret = fts_read(&raw_addr, 1, buf, MAX_PACKET_SIZE);
	size = byte_num - MAX_PACKET_SIZE;
	offset = MAX_PACKET_SIZE;

	while (size > 0) {
		if (size >= MAX_PACKET_SIZE) {
			packet_len = MAX_PACKET_SIZE;

		} else {
			packet_len = size;
		}

		ret = fts_read(&raw_addr, 1, buf + offset, packet_len);

		if (ret < 0) {
			TPD_INFO("%s:read raw data(packet:%d) fail", __func__,
			         offset / MAX_PACKET_SIZE);
			goto reg_restore;
		}

		size -= packet_len;
		offset += packet_len;
	}

	for (i = 0; i < byte_num; i = i + 2) {
		raw[i >> 1] = (int)(short)((buf[i] << 8) + buf[i + 1]);
	}

reg_restore:

	if (is_diff) {
		ret = fts_write_reg(FACTORY_REG_DATA_SELECT, regval);

		if (ret < 0) {
			TPD_INFO("%s:restore reg0x06 fail", __func__);
		}
	}

raw_err:
	kfree(buf);
	ret = fts_enter_factory_work_mode(ts_data, FTS_WORK_MODE_VALUE);

	if (ret < 0) {
		TPD_INFO("%s:enter work mode fail", __func__);
	}

	return ret;
}

static void fts_delta_read(struct seq_file *s, void *chip_data)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	int *raw = NULL;
	int tx_num = ts_data->hw_res->tx_num;
	int rx_num = ts_data->hw_res->rx_num;

	TPD_INFO("%s:start to read diff data", __func__);
	focal_esd_check_enable(ts_data, false);   /*no allowed esd check*/

	raw = kzalloc(tx_num * rx_num * sizeof(int), GFP_KERNEL);

	if (!raw) {
		seq_printf(s, "kzalloc for raw fail\n");
		goto raw_fail;
	}

	ret = fts_write_reg(FTS_REG_AUTOCLB_ADDR, 0x01);

	if (ret < 0) {
		TPD_INFO("%s, write 0x01 to reg 0xee failed \n", __func__);
	}

	ret = fts_get_rawdata(ts_data, raw, true);

	if (ret < 0) {
		seq_printf(s, "get diff data fail\n");
		goto raw_fail;
	}

	for (i = 0; i < tx_num; i++) {
		seq_printf(s, "\n[%2d]", i + 1);

		for (j = 0; j < rx_num; j++) {
			seq_printf(s, " %5d,", raw[i * rx_num + j]);
		}
	}

	seq_printf(s, "\n");

raw_fail:
	fts_write_reg(FTS_REG_AUTOCLB_ADDR, 0x00);
	focal_esd_check_enable(ts_data, true);
	kfree(raw);
}

static void fts_baseline_read(struct seq_file *s, void *chip_data)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	int *raw = NULL;
	int tx_num = ts_data->hw_res->tx_num;
	int rx_num = ts_data->hw_res->rx_num;

	TPD_INFO("%s:start to read raw data", __func__);
	focal_esd_check_enable(ts_data, false);

	raw = kzalloc(tx_num * rx_num * sizeof(int), GFP_KERNEL);

	if (!raw) {
		seq_printf(s, "kzalloc for raw fail\n");
		goto raw_fail;
	}

	ret = fts_write_reg(FTS_REG_AUTOCLB_ADDR, 0x01);

	if (ret < 0) {
		TPD_INFO("%s, write 0x01 to reg 0xee failed \n", __func__);
	}

	ret = fts_get_rawdata(ts_data, raw, false);

	if (ret < 0) {
		seq_printf(s, "get raw data fail\n");
		goto raw_fail;
	}

	for (i = 0; i < tx_num; i++) {
		seq_printf(s, "\n[%2d]", i + 1);

		for (j = 0; j < rx_num; j++) {
			seq_printf(s, " %5d,", raw[i * rx_num + j]);
		}
	}

	seq_printf(s, "\n");

raw_fail:
	fts_write_reg(FTS_REG_AUTOCLB_ADDR, 0x00);
	focal_esd_check_enable(ts_data, true);
	kfree(raw);
}

static void fts_main_register_read(struct seq_file *s, void *chip_data)
{
	u8 regvalue = 0;

	/*TP FW version*/
	fts_read_reg(FTS_REG_FW_VER, &regvalue);
	seq_printf(s, "TP FW Ver:0x%02x\n", regvalue);

	/*Vendor ID*/
	fts_read_reg(FTS_REG_VENDOR_ID, &regvalue);
	seq_printf(s, "Vendor ID:0x%02x\n", regvalue);

	/*Gesture enable*/
	fts_read_reg(FTS_REG_GESTURE_EN, &regvalue);
	seq_printf(s, "Gesture Mode:0x%02x\n", regvalue);

	/*charge in*/
	fts_read_reg(FTS_REG_CHARGER_MODE_EN, &regvalue);
	seq_printf(s, "charge state:0x%02x\n", regvalue);

	/*edge limit*/
	fts_read_reg(FTS_REG_EDGE_LIMIT, &regvalue);
	seq_printf(s, "edge Mode:0x%02x\n", regvalue);

	/*game mode*/
	fts_read_reg(FTS_REG_GAME_MODE_EN, &regvalue);
	seq_printf(s, "Game Mode:0x%02x\n", regvalue);

	/*FOD mode*/
	fts_read_reg(FTS_REG_FOD_EN, &regvalue);
	seq_printf(s, "FOD Mode:0x%02x\n", regvalue);

	/*Interrupt counter*/
	fts_read_reg(FTS_REG_INT_CNT, &regvalue);
	seq_printf(s, "INT count:0x%02x\n", regvalue);

	/*Flow work counter*/
	fts_read_reg(FTS_REG_FLOW_WORK_CNT, &regvalue);
	seq_printf(s, "ESD count:0x%02x\n", regvalue);

	/*Panel ID*/
	fts_read_reg(FTS_REG_MODULE_ID, &regvalue);
	seq_printf(s, "PANEL ID:0x%02x\n", regvalue);

	return;
}

static int fts_enable_black_gesture(struct chip_data_ft8057p *ts_data,
                                    bool enable)
{
	struct touchpanel_data *ts = ts_data->ts;
	int i = 0;
	u8 state = 0xFF;
	fts_enter_gesture_mode(ts, enable);
	for (i = 0; i < 5; i++) {
		fts_write_reg(0xD1, 0xFF);
		fts_write_reg(0xD2, 0xFF);
		fts_write_reg(0xD5, 0xFF);
		fts_write_reg(0xD6, 0xFF);
		fts_write_reg(0xD7, 0xFF);
		fts_write_reg(0xD8, 0xFF);
		fts_write_reg(FTS_REG_GESTURE_EN, enable);
		msleep(1);
		fts_read_reg(FTS_REG_GESTURE_EN, &state);
		if (state == 1)
			break;
	}

	if (i >= 5)
		TPD_INFO("make IC enter into gesture(suspend) fail,state:%x", state);
	else
		TPD_INFO("Enter into gesture(suspend) successfully");

	return 0;
}


static int fts_enable_edge_limit(struct chip_data_ft8057p *ts_data, int enable)
{
	u8 edge_mode = 0;

	/*0:Horizontal, 1:Vertical*/
	if (enable == VERTICAL_SCREEN) {
		edge_mode = 0;

	} else if (enable == LANDSCAPE_SCREEN_90) {
		edge_mode = 1;

	} else if (enable == LANDSCAPE_SCREEN_270) {
		edge_mode = 2;
	}

	TPD_INFO("MODE_EDGE, write 0x8C=%d", edge_mode);
	return fts_write_reg(FTS_REG_EDGE_LIMIT, edge_mode);
}

static int fts_enable_charge_mode(struct chip_data_ft8057p *ts_data, bool enable)
{
	TPD_INFO("MODE_CHARGE, write 0x8B=%d", enable);
	return fts_write_reg(FTS_REG_CHARGER_MODE_EN, enable);
}

static int fts_enable_game_mode(struct chip_data_ft8057p *ts_data, bool enable)
{
	TPD_INFO("MODE_GAME, write 0xC3=%d", enable);
	return fts_write_reg(FTS_REG_GAME_MODE_EN, enable);
}

static int fts_enable_headset_mode(struct chip_data_ft8057p *ts_data,
                                   bool enable)
{
	TPD_INFO("MODE_HEADSET, write 0xC3=%d \n", enable);
	return fts_write_reg(FTS_REG_HEADSET_MODE_EN, enable);
}

static int fts_mode_switch(void *chip_data, work_mode mode, int flag)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	int ret = 0;
	struct touchpanel_data *ts = ts_data->ts;

	switch (mode) {
	case MODE_NORMAL:
		TPD_INFO("MODE_NORMAL");
		if (ts_data->ts->is_suspended == 0) {
			fts_enter_gesture_mode(ts, 0);
                }
		break;

	case MODE_SLEEP:
		TPD_INFO("MODE_SLEEP, write 0xA5=3");
		ret = fts_write_reg(FTS_REG_POWER_MODE, 0x03);

		if (ret < 0) {
			TPD_INFO("%s: enter into sleep failed.\n", __func__);
			goto mode_err;
		}

		break;

	case MODE_GESTURE:
		TPD_INFO("MODE_GESTURE, Melo, ts->is_suspended = %d \n",
		         ts_data->ts->is_suspended);

		if (ts_data->ts->is_suspended) {                             /* do not pull up reset when doing resume*/
			if (ts_data->last_mode == MODE_SLEEP) {
				fts_hw_reset(ts_data, RESET_TO_NORMAL_TIME);
			}
		}

		ret = fts_enable_black_gesture(ts_data, flag);

		if (ret < 0) {
			TPD_INFO("%s: enable gesture failed.\n", __func__);
			goto mode_err;
		}

		break;

	/*    case MODE_GLOVE:*/
	/*        break;*/

	case MODE_EDGE:
		ret = fts_enable_edge_limit(ts_data, flag);

		if (ret < 0) {
			TPD_INFO("%s: enable edg limit failed.\n", __func__);
			goto mode_err;
		}

		break;

	case MODE_FACE_DETECT:
		break;

	case MODE_CHARGE:
		ret = fts_enable_charge_mode(ts_data, flag);

		if (ret < 0) {
			TPD_INFO("%s: enable charge mode failed.\n", __func__);
			goto mode_err;
		}

		break;

	case MODE_GAME:
		ret = fts_enable_game_mode(ts_data, flag);

		if (ret < 0) {
			TPD_INFO("%s: enable game mode failed.\n", __func__);
			goto mode_err;
		}

		break;

	case MODE_HEADSET:
		ret = fts_enable_headset_mode(ts_data, flag);

		if (ret < 0) {
			TPD_INFO("%s: enable headset mode failed.\n", __func__);
			goto mode_err;
		}

		break;

	default:
		TPD_INFO("%s: Wrong mode.\n", __func__);
		goto mode_err;
	}

	ts_data->last_mode = mode;
	return 0;
mode_err:
	return ret;
}



/*
 * return success: 0; fail : negative
 */
static int fts_reset(void *chip_data)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;

	TPD_INFO("%s:call\n", __func__);
	fts_hw_reset(ts_data, RESET_TO_NORMAL_TIME);

	return 0;
}

static int  fts_reset_gpio_control(void *chip_data, bool enable)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	return fts_rstgpio_set(ts_data->hw_res, enable);
}

static int fts_get_vendor(void *chip_data, struct panel_info *panel_data)
{
	int len = 0;
	/*char manu_temp[MAX_DEVICE_MANU_LENGTH] = FOCAL_PREFIX;*/

	len = strlen(panel_data->fw_name);
	if ((len > 3) && (panel_data->fw_name[len - 3] == 'i') && \
	    (panel_data->fw_name[len - 2] == 'm') && (panel_data->fw_name[len - 1] == 'g')) {
		/*panel_data->fw_name[len-3] = 'b';*/
		/*panel_data->fw_name[len-2] = 'i';*/
		/*panel_data->fw_name[len-1] = 'n';*/
		TPD_INFO("tp_type = %d, panel_data->fw_name = %s\n", panel_data->tp_type, panel_data->fw_name);
	}
	/*strlcat(manu_temp, panel_data->manufacture_info.manufacture, MAX_DEVICE_MANU_LENGTH);*/
	/*strncpy(panel_data->manufacture_info.manufacture, manu_temp, MAX_DEVICE_MANU_LENGTH);*/
	TPD_INFO("tp_type = %d, panel_data->fw_name = %s\n", panel_data->tp_type, panel_data->fw_name);

	return 0;
}

static int fts_get_chip_info(void *chip_data)
{
	u8 cmd = 0x90;
	u8 id[2] = { 0 };
	int cnt = 0;
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;

	for (cnt = 0; cnt < 3; cnt++) {
		/* hardware tp reset to boot */
		fts_hw_reset(ts_data, 0);
		mdelay(FTS_CMD_START_DELAY);

		/* check boot id*/
		cmd = FTS_CMD_START;
		fts_write(&cmd, 1);
		mdelay(FTS_CMD_START_DELAY);
		cmd = FTS_CMD_READ_ID;
		fts_read(&cmd, 1, id, 2);
		TPD_INFO("read boot id:0x%02x%02x", id[0], id[1]);
		if (id[0] == 0x80) {
			return 0;
		}
	}

	return 0;
}

static int fts_ftm_process(void *chip_data)
{
	int ret = 0;

	ret = fts_mode_switch(chip_data, MODE_SLEEP, true);

	if (ret < 0) {
		TPD_INFO("%s:switch mode to MODE_SLEEP fail", __func__);
		return ret;
	}

	ret = fts_power_control(chip_data, false);

	if (ret < 0) {
		TPD_INFO("%s:power on fail", __func__);
		return ret;
	}

	return 0;
}

static void fts_read_fod_info(struct chip_data_ft8057p *ts_data)
{
	int ret = 0;
	u8 cmd = FTS_REG_FOD_INFO;
	u8 val[FTS_REG_FOD_INFO_LEN] = { 0 };

	ret = fts_read(&cmd, 1, val, FTS_REG_FOD_INFO_LEN);

	if (ret < 0) {
		TPD_INFO("%s:read FOD info fail", __func__);
		return;
	}

	TPD_DEBUG("%s:FOD info buffer:%x %x %x %x %x %x %x %x %x", __func__, val[0],
	          val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8]);
	ts_data->fod_info.fp_id = val[0];
	ts_data->fod_info.event_type = val[1];

	if (val[8] == 0) {
		ts_data->fod_info.fp_down = 1;

	} else if (val[8] == 1) {
		ts_data->fod_info.fp_down = 0;
	}

	ts_data->fod_info.fp_area_rate = val[2];
	ts_data->fod_info.fp_x = (val[4] << 8) + val[5];
	ts_data->fod_info.fp_y = (val[6] << 8) + val[7];
}

static int fts_fw_recovery(struct chip_data_ft8057p *ts_data)
{
	int ret = 0;
	u8 boot_state = 0;
	u8 chip_id = 0;
	u8 cmd = FTS_CMD_READ_ID;
	u8 id[2] = { 0 };
	struct touchpanel_data *ts = ts_data->ts;

	TPD_INFO("check if boot recovery");

	if (ts->loading_fw) {
		TPD_INFO("fw is loading, not download again");
		return -EINVAL;
	}

	ret = fts_read(&cmd, 1, id, 2);
	TPD_INFO("read boot id:0x%02x%02x", id[0], id[1]);
	if (id[0] != 0x80) {
		TPD_INFO("check boot id fail");
		return ret;
	}

	ret = fts_read_reg(0xD0, &boot_state);
	if (ret < 0) {
		TPD_INFO("read boot state failed, ret=%d", ret);
		return ret;
	}

	if (boot_state != 0x01) {
		TPD_INFO("not in boot mode(0x%x),exit", boot_state);
		return -EIO;
	}

	TPD_INFO("abnormal situation,need download fw");
	/*ret = request_firmware(&fw, ts->panel_data.fw_name, ts->dev);
	if (ret) {
		TPD_INFO("request_firmware(%s) fail", ts->panel_data.fw_name);
	}

	ts->loading_fw = true;
	if (ts->ts_ops && ts->ts_ops->fw_update)
		ret = ts->ts_ops->fw_update(ts->chip_data, fw, 1);
	ts->loading_fw = false;
	if (fw) {
		release_firmware(fw);
		fw = NULL;
	}*/
	if ((ts_data->ts->is_suspended) && (ts_data->last_mode == MODE_GESTURE)) {
		fts_enter_gesture_mode(ts, 1);
	}
	else {
		fts_enter_gesture_mode(ts, 0);
	}
	msleep(10);
	ret = fts_read_reg(FTS_REG_CHIP_ID, &chip_id);
	TPD_INFO("read chip id:0x%02x", chip_id);

	TPD_INFO("boot recovery pass");
	return ret;
}

static int fts_enter_gesture_mode(struct touchpanel_data *ts, bool gesture_mode)
{
	int ret = 0;
	u8 detach_flag = 0;
	uint8_t copy_len = 0;
	u8 fwStatus = 0;
	u8 i;
	const struct firmware *fw = NULL;
	char *fw_name_gesture = NULL;
	char *p_node = NULL;
	char *postfix = "_GESTURE.img";

	TPD_INFO("fw test download function\n");
	if (ts->loading_fw) {
		TPD_INFO("fw is loading, not download again\n");
		return -EINVAL;
	}
	for (i = 0; i < 3;i++) {
		fts_read_reg(FTS_REG_FACTORY_MODE_DETACH_FLAG, &fwStatus);
		TPD_INFO("regb4:0x%02x\n", fwStatus);
		if ((fwStatus == 0xAA) || (fwStatus == 0x66)) {
			break;
		}
	}
	if (gesture_mode) {
	/*write gesture firmware*/
	fw_name_gesture = kzalloc(MAX_FW_NAME_LENGTH, GFP_KERNEL);
		if (fw_name_gesture == NULL) {
			TPD_INFO("fw_name_gesture kzalloc error!\n");
			return -ENOMEM;
		}
	p_node = strstr(ts->panel_data.fw_name, ".");
		copy_len = p_node - ts->panel_data.fw_name;
		memcpy(fw_name_gesture, ts->panel_data.fw_name, copy_len);
		strlcat(fw_name_gesture, postfix, MAX_FW_NAME_LENGTH);
		TPD_INFO("fw_name_gesture is %s\n", fw_name_gesture);
	if (fwStatus == 0x66) {
		return ret;
	}
	ret = request_firmware(&fw, fw_name_gesture, ts->dev);
	if (ret) {
		TPD_INFO("request_firmware(%s) fail\n", fw_name_gesture);
		return -ENODATA;
	}
	} else {
	/*write normal firmware*/
	if (fwStatus == 0xAA) {
		return ret;
	}
	ret = request_firmware(&fw, ts->panel_data.fw_name, ts->dev);
	}

	/*download firmware*/
	ts->loading_fw = true;
	if (ts->ts_ops && ts->ts_ops->fw_update)
		ret = ts->ts_ops->fw_update(ts->chip_data, fw, 1);
	ts->loading_fw = false;

	msleep(50);
	fts_read_reg(FTS_REG_FACTORY_MODE_DETACH_FLAG, &detach_flag);
	TPD_INFO("regb4:0x%02x\n", detach_flag);

	if (fw) {
		release_firmware(fw);
		fw = NULL;
	}
	return ret;
}


static u32 fts_u32_trigger_reason(void *chip_data, int gesture_enable,
                                  int is_suspended)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	int ret = 0;
	u8 cmd = FTS_REG_POINTS;
	u32 result_event = 0;
	u8 *touch_buf = ts_data->touch_buf;
	u8 val = 0xFF;

	fts_prc_queue_work(ts_data);

	memset(touch_buf, 0xFF, FTS_MAX_POINTS_LENGTH);

	if (gesture_enable && is_suspended) {
		ret = fts_read_reg(FTS_REG_GESTURE_EN, &val);
		if (val == 0x01) {
			return IRQ_GESTURE;
		}
	}

	ret = fts_read(&cmd, 1, &touch_buf[0], ts_data->touch_size);
	if (ret < 0) {
		TPD_INFO("read touch point one fail");
		return IRQ_IGNORE;
	}

	if ((0xEF == touch_buf[1]) && (0xEF == touch_buf[2]) && (0xEF == touch_buf[3])) {
		/*recovery fw*/
		fts_fw_recovery(ts_data);
		return IRQ_IGNORE;
	}

	if ((touch_buf[1] == 0xFF) && (touch_buf[2] == 0xFF) && (touch_buf[3] == 0xFF)) {
		TPD_INFO("Need recovery TP state");
		return IRQ_FW_AUTO_RESET;
	}

	/*confirm need print debug info*/
	if (touch_buf[0] != ts_data->irq_type) {
		SET_BIT(result_event, IRQ_FW_HEALTH);
	}

	ts_data->irq_type = touch_buf[0];

	/*normal touch*/
	SET_BIT(result_event, IRQ_TOUCH);

	return result_event;
}

static int fts_get_touch_points(void *chip_data, struct point_info *points,
                                int max_num)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	int i = 0;
	int obj_attention = 0;
	int base = 0;
	int event_num = 0;
	u8 finger_num = 0;
	u8 pointid = 0;
	u8 event_flag = 0;
	u8 *touch_buf = ts_data->touch_buf;

	finger_num = touch_buf[1] & 0xFF;

	if (finger_num > max_num) {
		TPD_INFO("invalid point_num(%d),max_num(%d)", finger_num, max_num);
		return -EIO;
	}

	for (i = 0; i < max_num; i++) {
		base = 6 * i;
		pointid = (touch_buf[4 + base]) >> 4;

		if (pointid >= FTS_MAX_ID) {
			break;

		} else if (pointid >= max_num) {
			TPD_INFO("ID(%d) beyond max_num(%d)", pointid, max_num);
			return -EINVAL;
		}

		event_num++;
		if (!ts_data->high_resolution_support && !ts_data->high_resolution_support_x8) {
			points[pointid].x = ((touch_buf[2 + base] & 0x0F) << 8) + (touch_buf[3 + base] & 0xFF);
			points[pointid].y = ((touch_buf[4 + base] & 0x0F) << 8) + (touch_buf[5 + base] & 0xFF);
			points[pointid].touch_major = touch_buf[7 + base];
			points[pointid].width_major = touch_buf[7 + base];
			points[pointid].z =  touch_buf[7 + base];
			event_flag = (touch_buf[2 + base] >> 6);
		} else if (ts_data->high_resolution_support_x8) {
			points[pointid].x = (((touch_buf[2 + base] & 0x0F) << 11) +
			                     ((touch_buf[3 + base] & 0xFF) << 3) +
			                     ((touch_buf[6 + base] >> 5) & 0x07));
			points[pointid].y = (((touch_buf[4 + base] & 0x0F) << 11) +
			                     ((touch_buf[5 + base] & 0xFF) << 3) +
			                     ((touch_buf[6 + base] >> 2) & 0x07));
			points[pointid].touch_major = touch_buf[7 + base];
			points[pointid].width_major = touch_buf[7 + base];
			points[pointid].z =  touch_buf[7 + base];
			event_flag = (touch_buf[2 + base] >> 6);
		}

		points[pointid].status = 0;

		if ((event_flag == 0) || (event_flag == 2)) {
			points[pointid].status = 1;
			obj_attention |= (1 << pointid);

			if (finger_num == 0) {
				TPD_INFO("abnormal touch data from fw");
				return -EIO;
			}
		}
	}

	if (event_num == 0) {
		TPD_INFO("no touch point information");
		return -EIO;
	}

	if (ts_data->touch_analysis_support && ts_data->ta_flag) {
		ts_data->ta_flag = 0;
		ts_data->ta_size = ts_data->touch_size;
		if (ts_data->ta_buf && ts_data->ta_size)
			memcpy(ts_data->ta_buf, ts_data->touch_buf, ts_data->ta_size);
		wake_up_interruptible(&ts_data->ts_waitqueue);
	}

	return obj_attention;
}

static void fts_health_report(void *chip_data, struct monitor_data *mon_data)
{
	int ret = 0;
	u8 val = 0;

	ret = fts_read_reg(0x01, &val);
	TPD_INFO("Health register(0x01):0x%x", val);
	ret = fts_read_reg(FTS_REG_HEALTH_1, &val);
	TPD_INFO("Health register(0xFD):0x%x", val);
	ret = fts_read_reg(FTS_REG_HEALTH_2, &val);
	TPD_INFO("Health register(0xFE):0x%x", val);
}

static int fts_get_gesture_info(void *chip_data, struct gesture_info *gesture)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	int ret = 0;
	u8 cmd = FTS_REG_GESTURE_OUTPUT_ADDRESS;
	u8 buf[FTS_GESTURE_DATA_LEN] = { 0 };
	u8 gesture_id = 0;
	u8 point_num = 0;

	ret = fts_read(&cmd, 1, &buf[2], FTS_GESTURE_DATA_LEN - 2);

	if (ret < 0) {
		TPD_INFO("read gesture data fail");
		return ret;
	}

	gesture_id = buf[2];
	point_num = buf[3];
	TPD_INFO("gesture_id=%d, point_num=%d", gesture_id, point_num);

	switch (gesture_id) {
	case GESTURE_DOUBLE_TAP:
		gesture->gesture_type = DOU_TAP;
		break;

	case GESTURE_UP_VEE:
		gesture->gesture_type = UP_VEE;
		break;

	case GESTURE_DOWN_VEE:
		gesture->gesture_type = DOWN_VEE;
		break;

	case GESTURE_LEFT_VEE:
		gesture->gesture_type = LEFT_VEE;
		break;

	case GESTURE_RIGHT_VEE:
		gesture->gesture_type = RIGHT_VEE;
		break;

	case GESTURE_O_CLOCKWISE:
		gesture->clockwise = 1;
		gesture->gesture_type = CIRCLE_GESTURE;
		break;

	case GESTURE_O_ANTICLOCK:
		gesture->clockwise = 0;
		gesture->gesture_type = CIRCLE_GESTURE;
		break;

	case GESTURE_DOUBLE_SWIP:
		gesture->gesture_type = DOU_SWIP;
		break;

	case GESTURE_LEFT2RIGHT_SWIP:
		gesture->gesture_type = LEFT2RIGHT_SWIP;
		break;

	case GESTURE_RIGHT2LEFT_SWIP:
		gesture->gesture_type = RIGHT2LEFT_SWIP;
		break;

	case GESTURE_UP2DOWN_SWIP:
		gesture->gesture_type = UP2DOWN_SWIP;
		break;

	case GESTURE_DOWN2UP_SWIP:
		gesture->gesture_type = DOWN2UP_SWIP;
		break;

	case GESTURE_M:
		gesture->gesture_type = M_GESTRUE;
		break;

	case GESTURE_W:
		gesture->gesture_type = W_GESTURE;
		break;

	case GESTURE_FINGER_PRINT:
		fts_read_fod_info(ts_data);
		TPD_INFO("FOD event type:0x%x", ts_data->fod_info.event_type);
		TPD_DEBUG("%s, fgerprint, touched = %d, fp_down = %d, fp_down_report = %d, \n",
		          __func__, ts_data->ts->view_area_touched, ts_data->fod_info.fp_down,
		          ts_data->fod_info.fp_down_report);

		if (ts_data->fod_info.event_type == FTS_EVENT_FOD) {
			if (ts_data->fod_info.fp_down && !ts_data->fod_info.fp_down_report) {
				gesture->gesture_type = FINGER_PRINTDOWN;
				ts_data->fod_info.fp_down_report = 1;

			} else if (!ts_data->fod_info.fp_down && ts_data->fod_info.fp_down_report) {
				gesture->gesture_type = FRINGER_PRINTUP;
				ts_data->fod_info.fp_down_report = 0;
			}

			gesture->Point_start.x = ts_data->fod_info.fp_x;
			gesture->Point_start.y = ts_data->fod_info.fp_y;
			gesture->Point_end.x = ts_data->fod_info.fp_area_rate;
			gesture->Point_end.y = 0;
		}

		break;

	case GESTURE_SINGLE_TAP:
		gesture->gesture_type = SINGLE_TAP;
		break;

	default:
		gesture->gesture_type = UNKOWN_GESTURE;
	}

	if ((gesture->gesture_type != FINGER_PRINTDOWN)
	    && (gesture->gesture_type != FRINGER_PRINTUP)
	    && (gesture->gesture_type != UNKOWN_GESTURE)) {
		gesture->Point_start.x = (u16)((buf[4] << 8) + buf[5]);
		gesture->Point_start.y = (u16)((buf[6] << 8) + buf[7]);
		gesture->Point_end.x = (u16)((buf[8] << 8) + buf[9]);
		gesture->Point_end.y = (u16)((buf[10] << 8) + buf[11]);
		gesture->Point_1st.x = (u16)((buf[12] << 8) + buf[13]);
		gesture->Point_1st.y = (u16)((buf[14] << 8) + buf[15]);
		gesture->Point_2nd.x = (u16)((buf[16] << 8) + buf[17]);
		gesture->Point_2nd.y = (u16)((buf[18] << 8) + buf[19]);
		gesture->Point_3rd.x = (u16)((buf[20] << 8) + buf[21]);
		gesture->Point_3rd.y = (u16)((buf[22] << 8) + buf[23]);
		gesture->Point_4th.x = (u16)((buf[24] << 8) + buf[25]);
		gesture->Point_4th.y = (u16)((buf[26] << 8) + buf[27]);
	}

	return 0;
}

static void fts_register_info_read(void *chip_data, uint16_t register_addr,
                                   uint8_t *result, uint8_t length)
{
	u8 addr = (u8)register_addr;

	fts_read(&addr, 1, result, length);
}

static void fts_set_touch_direction(void *chip_data, uint8_t dir)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	ts_data->touch_direction = dir;
}

static uint8_t fts_get_touch_direction(void *chip_data)
{
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)chip_data;
	return ts_data->touch_direction;
}

static int fts_smooth_lv_set(void *chip_data, int level)
{
	TPD_INFO("set smooth lv to %d", level);

	return fts_write_reg(FTS_REG_SMOOTH_LEVEL, level);
}

static int fts_sensitive_lv_set(void *chip_data, int level)
{
	int ret = 0;

	TPD_INFO("set sensitive lv to %d", level);

	ret = fts_write_reg(FTS_REG_STABLE_DISTANCE_AFTER_N, level);
	if (ret < 0) {
		TPD_INFO("write FTS_REG_STABLE_DISTANCE_AFTER_N fail");
		return ret;
	}

	ret = fts_write_reg(FTS_REG_STABLE_DISTANCE, level);
	if (ret < 0) {
		TPD_INFO("write FTS_REG_STABLE_DISTANCE fail");
		return ret;
	}

	return 0;
}

static int ft8057p_parse_dts(struct chip_data_ft8057p *ts_data, struct spi_device *spi)
{
	struct device *dev;
	struct device_node *np;

	dev = &spi->dev;
	np = dev->of_node;

	ts_data->high_resolution_support = of_property_read_bool(np, "high_resolution_support");
	ts_data->high_resolution_support_x8 = of_property_read_bool(np, "high_resolution_support_x8");
	TPD_INFO("%s:high_resolution_support is:%d %d\n", __func__, ts_data->high_resolution_support,
	         ts_data->high_resolution_support_x8);

	return 0;
}

int fts_set_spi_max_speed(unsigned int speed, char mode)
{
	int rc;
	struct spi_device *spi = g_fts_data->ft_spi;

	if (mode) {
		spi->max_speed_hz = speed;
	} else {
		spi->max_speed_hz = g_fts_data->spi_speed;
	}

	rc = spi_setup(spi);
	if (rc) {
		TPD_INFO("%s: spi setup fail\n", __func__);
		return rc;
	}
	return rc;
}

static struct oplus_touchpanel_operations fts_ops = {
	.power_control              = fts_power_control,
	.get_vendor                 = fts_get_vendor,
	.get_chip_info              = fts_get_chip_info,
	.fw_check                   = fts_fw_check,
	.mode_switch                = fts_mode_switch,
	.reset                      = fts_reset,
	.reset_gpio_control         = fts_reset_gpio_control,
	.fw_update                  = fts_fw_update,
	.trigger_reason             = fts_u32_trigger_reason,
	.get_touch_points           = fts_get_touch_points,
	.health_report              = fts_health_report,
	.get_gesture_info           = fts_get_gesture_info,
	.ftm_process                = fts_ftm_process,
	/*.enable_fingerprint         = fts_enable_fingerprint_underscreen,*/
	/*.screenon_fingerprint_info  = fts_screenon_fingerprint_info,*/
	.register_info_read         = fts_register_info_read,
	.set_touch_direction        = fts_set_touch_direction,
	.get_touch_direction        = fts_get_touch_direction,
	.esd_handle                 = fts_esd_handle,
	.smooth_lv_set              = fts_smooth_lv_set,
	.sensitive_lv_set           = fts_sensitive_lv_set,
};

static struct focal_auto_test_operations ft8057p_test_ops = {
	.auto_test_preoperation = ft8057p_auto_preoperation,
	.test1 = ft8057p_lcd_noise_test,
	.test2 = ft8057p_rawdata_autotest,
	.test9 = ft8057p_short_test,
	.test10 = ft8057p_open_test,
	.test11 = ft8057p_cb_test,
	.black_screen_test_preoperation = ft8057p_black_screen_test_preoperation,
	.test16 = ft8057p_black_cb_test,
	.test17 = ft8057p_black_rawdata_autotest,
	.test18 = ft8057p_black_lcd_noise_test,
	.auto_test_endoperation = ft8057p_auto_endoperation,
};

static struct engineer_test_operations ft8057p_engineer_test_ops = {
	.auto_test              = focal_auto_test,
	.black_screen_test      = focal_black_screen_test,
};

static struct debug_info_proc_operations fts_debug_info_proc_ops = {
	.delta_read        = fts_delta_read,
	/*  .key_trigger_delta_read = fts_key_trigger_delta_read,*/
	.baseline_read = fts_baseline_read,
	.main_register_read = fts_main_register_read,
	/*  .self_delta_read   = fts_self_delta_read,*/
};

struct focal_debug_func focal_debug_ops = {
	.esd_check_enable       = focal_esd_check_enable,
	.get_esd_check_flag     = focal_get_esd_check_flag,
	.get_fw_version         = focal_get_fw_version,
	.dump_reg_sate          = focal_dump_reg_state,
};

static int fts_tp_probe(struct spi_device *spi)
{
	struct chip_data_ft8057p *ts_data = NULL;
	struct touchpanel_data *ts = NULL;
	u64 time_counter = 0;
	int ret = -1;

	TPD_INFO("%s  is called\n", __func__);

	reset_healthinfo_time_counter(&time_counter);
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	spi->max_speed_hz = 6000000;
	spi->cs_setup.value = 1;
	spi->cs_setup.unit = 0;
	spi->cs_hold.value = 1;
	spi->cs_hold.unit = 0;
	spi->cs_inactive.value = 1;
	spi->cs_inactive.unit = 0;
	ret = spi_setup(spi);
	if (ret) {
		TPD_INFO("spi setup fail");
		return ret;
	}

	/*step1:Alloc chip_info*/
	ts_data = kzalloc(sizeof(struct chip_data_ft8057p), GFP_KERNEL);

	if (ts_data == NULL) {
		TPD_INFO("ts_data kzalloc error\n");
		ret = -ENOMEM;
		return ret;
	}

	memset(ts_data, 0, sizeof(*ts_data));
	ts_data->spi_speed = spi->max_speed_hz;
	TPD_INFO("spi_speed = %d\n", ts_data->spi_speed);
	g_fts_data = ts_data;

	ts_data->ts_workqueue = create_singlethread_workqueue("fts_wq");
	if (!ts_data->ts_workqueue) {
		TPD_INFO("create fts workqueue fail");
	}
	init_waitqueue_head(&ts_data->ts_waitqueue);

	ret = fts_bus_init(ts_data);
	if (ret < 0) {
		TPD_INFO("bus init error\n");
		goto ts_malloc_failed;
	}

	ts_data->touch_buf = (u8 *)kzalloc(FTS_MAX_TOUCH_BUF, GFP_KERNEL);
	if (!ts_data->touch_buf) {
		TPD_INFO("failed to alloc memory for touch buf");
		ret = -ENOMEM;
		goto err_bus_init;
	}
	ts_data->touch_size = FTS_MAX_POINTS_LENGTH;
	ts_data->touch_analysis_support = 0;
	ts_data->ta_flag = 0;
	ts_data->ta_size = 0;

	fts_point_report_check_init(ts_data);

	/*step2:Alloc common ts*/
	ts = common_touch_data_alloc();

	if (ts == NULL) {
		TPD_INFO("ts kzalloc error\n");
		ret = -ENOMEM;
		goto err_report_buf;
	}

	memset(ts, 0, sizeof(*ts));

	/*step3:binding client && dev for easy operate*/
	ts_data->ft_spi = spi;
	ts_data->hw_res = &ts->hw_res;
	ts_data->ts = ts;
	ts->debug_info_ops = &fts_debug_info_proc_ops;
	ts->s_client = spi;
	ts->irq = spi->irq;
	ts->dev = &spi->dev;
	ts->chip_data = ts_data;
	spi_set_drvdata(spi, ts);

	/*step4:file_operations callback binding*/
	ts->ts_ops = &fts_ops;
	ts->engineer_ops = &ft8057p_engineer_test_ops;
	ts->com_test_data.chip_test_ops = &ft8057p_test_ops;

	ts->private_data = &focal_debug_ops;
	ft8057p_parse_dts(ts_data, spi);

	ret = fts_fwupg_init(ts_data);
	if (ret < 0) {
		goto err_report_buf;
	}

	/*step5:register common touch*/
	ret = register_common_touch_device(ts);

	if (ret < 0) {
		goto err_register_driver;
	}

	/*step6:create ftxxxx-debug related proc files*/
	fts_create_apk_debug_channel(ts_data);
	fts_create_sysfs(ts_data);

	/*step7:Chip Related function*/
	focal_create_sysfs_spi(spi);
	schedule_work(&ts->fw_update_work);
	if (ts->health_monitor_support) {
		tp_healthinfo_report(&ts->monitor_data, HEALTH_PROBE, &time_counter);
	}
	ts_data->probe_done = 1;
	TPD_INFO("%s, probe normal end\n", __func__);

	return 0;

err_register_driver:
	common_touch_data_free(ts);
	ts = NULL;

err_report_buf:
	kfree(ts_data->touch_buf);
	ts_data->touch_buf = NULL;

err_bus_init:
	kfree(ts_data->bus_tx_buf);
	ts_data->bus_tx_buf = NULL;
	kfree(ts_data->bus_rx_buf);
	ts_data->bus_rx_buf = NULL;

ts_malloc_failed:

	kfree(ts_data);
	ts_data = NULL;

	TPD_INFO("%s, probe error\n", __func__);

	return ret;
}

static int fts_tp_remove(struct spi_device *spi)
{
	struct touchpanel_data *ts = spi_get_drvdata(spi);
	struct chip_data_ft8057p *ts_data = (struct chip_data_ft8057p *)ts->chip_data;

	TPD_INFO("%s is called\n", __func__);
	fts_point_report_check_exit(ts_data);
	fts_release_apk_debug_channel(ts_data);
	fts_remove_sysfs(ts_data);
	fts_bus_exit(ts_data);
	kfree(ts_data->touch_buf);
	ts_data->touch_buf = NULL;

	kfree(ts_data);
	ts_data = NULL;

	kfree(ts);

	return 0;
}

static int fts_spi_suspend(struct device *dev)
{
	struct touchpanel_data *ts = dev_get_drvdata(dev);

	TPD_INFO("%s: is called\n", __func__);
	tp_pm_suspend(ts);


	return 0;
}

static int fts_spi_resume(struct device *dev)
{
	struct touchpanel_data *ts = dev_get_drvdata(dev);

	TPD_INFO("%s is called\n", __func__);
	tp_pm_resume(ts);


	return 0;
}

static const struct spi_device_id tp_id[] = {
#ifdef CONFIG_TOUCHPANEL_MULTI_NOFLASH
	{ "oplus,tp_noflash", 0 },
#else
	{ TPD_DEVICE, 0 },
#endif
	{ },
};

static struct of_device_id tp_match_table[] = {
#ifdef CONFIG_TOUCHPANEL_MULTI_NOFLASH
	{ .compatible = "oplus,tp_noflash", },
#else
	{ .compatible = TPD_DEVICE, },
#endif
	{ },
};

static const struct dev_pm_ops tp_pm_ops = {
	.suspend = fts_spi_suspend,
	.resume = fts_spi_resume,
};

static struct spi_driver fts_ts_driver = {
	.probe          = fts_tp_probe,
	.remove         = fts_tp_remove,
	.id_table       = tp_id,
	.driver         = {
		.name   = TPD_DEVICE,
		.of_match_table =  tp_match_table,
		.pm = &tp_pm_ops,
	},
};

static int __init tp_driver_init_ft8057p(void)
{
	TPD_INFO("%s is called\n", __func__);

	if (!tp_judge_ic_match(TPD_DEVICE)) {
		return 0;
	}

	if (spi_register_driver(&fts_ts_driver) != 0) {
		TPD_INFO("unable to add spi driver.\n");
		return 0;
	}

	return 0;
}

/* should never be called */
static void __exit tp_driver_exit_ft8057p(void)
{
	spi_unregister_driver(&fts_ts_driver);
	return;
}
#ifdef CONFIG_TOUCHPANEL_LATE_INIT
late_initcall(tp_driver_init_ft8057p);
#else
module_init(tp_driver_init_ft8057p);
#endif
module_exit(tp_driver_exit_ft8057p);

MODULE_DESCRIPTION("Touchscreen ft8057p Driver");
MODULE_LICENSE("GPL");

