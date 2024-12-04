#define LOG_TAG         "Core"

#include "cts_config.h"
#include "cts_core.h"
#include "cts_sfctrl.h"
#include "cts_spi_flash.h"

extern struct cts_interface tcs_if;
#define INT_DATA_MAX_SIZ     8192

#ifdef CONFIG_CTS_I2C_HOST
static size_t cts_plat_get_max_i2c_xfer_size(struct cts_platform_data *pdata)
{
    return CFG_CTS_MAX_I2C_XFER_SIZE;
}

static u8 *cts_plat_get_i2c_xfer_buf(struct cts_platform_data *pdata,
    size_t xfer_size)
{
        return pdata->i2c_fifo_buf;
}

static int cts_plat_i2c_write(struct cts_platform_data *pdata, u8 i2c_addr,
        const void *src, size_t len, int retry, int delay)
{
    int ret = 0, retries = 0;

    struct i2c_msg msg = {
        .addr  = i2c_addr,
        .flags = !I2C_M_RD,
        .len   = len,
        .buf   = (u8 *)src,
    };

    do {
        ret = i2c_transfer(pdata->i2c_client->adapter, &msg, 1);
        if (ret != 1) {
            if (ret >= 0) {
                ret = -EIO;
            }

            if (delay) {
                mdelay(delay);
            }
            continue;
        } else {
            return 0;
        }
    } while (++retries < retry);

    return ret;
}

static int cts_plat_i2c_read(struct cts_platform_data *pdata, u8 i2c_addr,
        const u8 *wbuf, size_t wlen, void *rbuf, size_t rlen,
        int retry, int delay)
{
    int num_msg, ret = 0, retries = 0;

    struct i2c_msg msgs[2] = {
        {
            .addr  = i2c_addr,
            .flags = !I2C_M_RD,
            .buf   = (u8 *)wbuf,
            .len   = wlen
        },
        {
            .addr  = i2c_addr,
            .flags = I2C_M_RD,
            .buf   = (u8 *)rbuf,
            .len   = rlen
        }
    };

    if (wbuf == NULL || wlen == 0) {
        num_msg = 1;
    } else {
        num_msg = 2;
    }

    do {
        ret = i2c_transfer(pdata->i2c_client->adapter,
                msgs + ARRAY_SIZE(msgs) - num_msg, num_msg);

        if (ret != num_msg) {
            if (ret >= 0) {
                ret = -EIO;
            }

            if (delay) {
                mdelay(delay);
            }
            continue;
        } else {
            return 0;
        }
    } while (++retries < retry);

    return ret;
}

int cts_plat_is_i2c_online(struct cts_platform_data *pdata, u8 i2c_addr)
{
    u8 dummy_bytes[2] = {0x00, 0x00};
    int ret;

    ret = cts_plat_i2c_write(pdata, i2c_addr, dummy_bytes,
            sizeof(dummy_bytes), 5, 2);
    if (ret) {
        TPD_INFO("<E> !!! I2C addr 0x%02x is offline !!!\n", i2c_addr);
        return false;
    } else {
        TPD_DEBUG("<D> I2C addr 0x%02x is online\n", i2c_addr);
        return true;
    }
}
#else /* CONFIG_CTS_I2C_HOST */

#ifdef CFG_MTK_LEGEND_PLATFORM
struct mt_chip_conf cts_spi_conf_mt65xx = {
    .setuptime = 15,
    .holdtime = 15,
    .high_time = 21, //for mt6582, 104000khz/(4+4) = 130000khz
    .low_time = 21,
    .cs_idletime = 20,
    .ulthgh_thrsh = 0,

    .cpol = 0,
    .cpha = 0,

    .rx_mlsb = 1,
    .tx_mlsb = 1,

    .tx_endian = 0,
    .rx_endian = 0,

    .com_mod = FIFO_TRANSFER,
    .pause = 1,
    .finish_intr = 1,
    .deassert = 0,
    .ulthigh = 0,
    .tckdly = 0,
};

typedef enum {
    SPEED_500KHZ = 500,
    SPEED_1MHZ = 1000,
    SPEED_2MHZ = 2000,
    SPEED_3MHZ = 3000,
    SPEED_4MHZ = 4000,
    SPEED_6MHZ = 6000,
    SPEED_8MHZ = 8000,
    SPEED_KEEP,
    SPEED_UNSUPPORTED
} SPI_SPEED;

static void cts_plat_spi_set_mode(struct spi_device *spi,
        SPI_SPEED speed, int flag)
{
    struct mt_chip_conf *mcc = &cts_spi_conf_mt65xx;
    if (flag == 0) {
        mcc->com_mod = FIFO_TRANSFER;
    } else {
        mcc->com_mod = DMA_TRANSFER;
    }

    switch (speed) {
    case SPEED_500KHZ:
        mcc->high_time = 120;
        mcc->low_time = 120;
        break;
    case SPEED_1MHZ:
        mcc->high_time = 60;
        mcc->low_time = 60;
        break;
    case SPEED_2MHZ:
        mcc->high_time = 30;
        mcc->low_time = 30;
        break;
    case SPEED_3MHZ:
        mcc->high_time = 20;
        mcc->low_time = 20;
        break;
    case SPEED_4MHZ:
        mcc->high_time = 15;
        mcc->low_time = 15;
        break;
    case SPEED_6MHZ:
        mcc->high_time = 10;
        mcc->low_time = 10;
        break;
    case SPEED_8MHZ:
        mcc->high_time = 8;
        mcc->low_time = 8;
        break;
    case SPEED_KEEP:
    case SPEED_UNSUPPORTED:
        break;
    }
    if (spi_setup(spi) < 0) {
        TPD_INFO("<E> Failed to set spi\n");
    }
}

int cts_plat_spi_setup(struct cts_platform_data *pdata)
{
    pdata->spi_client->mode = SPI_MODE_0;
    pdata->spi_client->bits_per_word = 8;
//  pdata->spi_client->chip_select = 0;
    pdata->spi_client->cs_setup.value =1;
    pdata->spi_client->cs_setup.unit  =0;
    pdata->spi_client->cs_hold.value = 1;
    pdata->spi_client->cs_hold.unit =0;
    pdata->spi_client->cs_inactive.value =1;
    pdata->spi_client->cs_inactive.unit = 0;
    pdata->spi_client->controller_data = (void *)&cts_spi_conf_mt65xx;
    spi_setup(pdata->spi_client);
    cts_plat_spi_set_mode(pdata->spi_client, pdata->spi_speed, 0);
    return 0;
}
#else
int cts_plat_spi_setup(struct cts_platform_data *pdata)
{
    pdata->spi_client->mode = SPI_MODE_0;
    pdata->spi_client->bits_per_word = 8;
    pdata->spi_client->cs_setup.value =1;
    pdata->spi_client->cs_setup.unit  =0;
    pdata->spi_client->cs_hold.value = 1;
    pdata->spi_client->cs_hold.unit =0;
    pdata->spi_client->cs_inactive.value =1;
    pdata->spi_client->cs_inactive.unit = 0;
    spi_setup(pdata->spi_client);
    return 0;
}
#endif

static int cts_spi_send_recv(struct cts_platform_data *pdata, size_t len,
        u8 *tx_buffer, u8 *rx_buffer)
{
    struct chipone_ts_data *cts_data;
    struct spi_message msg;
    struct spi_transfer cmd = {
        .cs_change = 0,
  //      .delay_usecs = 0,
        .speed_hz = pdata->spi_speed * 1000,
        .tx_buf = tx_buffer,
        .rx_buf = rx_buffer,
        .len    = len,
        //.tx_dma = 0,
        //.rx_dma = 0,
        .bits_per_word = 8,
    };
    int ret = 0 ;
    cts_data = container_of(pdata->cts_dev, struct chipone_ts_data, cts_dev);

    spi_message_init(&msg);
    spi_message_add_tail(&cmd,  &msg);
    ret = spi_sync(cts_data->spi_client, &msg);
    if (ret) {
        TPD_INFO("<E> spi_sync failed.\n");
    }

    return ret;
}

static size_t cts_plat_get_max_spi_xfer_size(struct cts_platform_data *pdata)
{
    return CFG_CTS_MAX_SPI_XFER_SIZE;
}

static u8 *cts_plat_get_spi_xfer_buf(struct cts_platform_data *pdata,
    size_t xfer_size)
{
    return pdata->spi_cache_buf;
}

static int cts_plat_spi_write(struct cts_platform_data *pdata, u8 dev_addr,
        const void *src, size_t len, int retry, int delay)
{
    int ret = 0, retries = 0;
    u16 crc;
    size_t data_len;

    if (len > CFG_CTS_MAX_SPI_XFER_SIZE) {
        TPD_INFO("<E> write too much data:wlen=%zd\n", len);
        return -EIO;
    }

    if (pdata->cts_dev->rtdata.program_mode) {
        pdata->spi_tx_buf[0] = dev_addr;
        memcpy(&pdata->spi_tx_buf[1], src, len);

        do {
            ret = cts_spi_send_recv(pdata, len + 1,
                pdata->spi_tx_buf, pdata->spi_rx_buf);
            if (ret) {
                if (delay) {
                    mdelay(delay);
                }
            } else {
                return 0;
            }
        } while (++retries < retry);
    }
    else {
        data_len = len - 2;
        pdata->spi_tx_buf[0] = dev_addr;
        pdata->spi_tx_buf[1] = *((u8 *)src + 1);
        pdata->spi_tx_buf[2] = *((u8 *)src);
        put_unaligned_le16(data_len, &pdata->spi_tx_buf[3]);
        crc = (u16)cts_crc32(pdata->spi_tx_buf, 5);
        put_unaligned_le16(crc, &pdata->spi_tx_buf[5]);
        memcpy(&pdata->spi_tx_buf[7], (char *)src + 2, data_len);
        crc = (u16)cts_crc32((char *)src + 2, data_len);
        put_unaligned_le16(crc, &pdata->spi_tx_buf[7+data_len]);
        do {
            ret = cts_spi_send_recv(pdata, len + 7, pdata->spi_tx_buf,
                pdata->spi_rx_buf);
            udelay(10 * data_len);
            if (ret) {
                if (delay) {
                    mdelay(delay);
                }
            } else {
                return 0;
            }
        } while (++retries < retry);
    }
    return ret;
}

static int cts_plat_spi_read(struct cts_platform_data *pdata, u8 dev_addr,
        const u8 *wbuf, size_t wlen, void *rbuf, size_t rlen,
        int retry, int delay)
{
    int ret = 0, retries = 0;
    u16 crc;

    if (wlen > CFG_CTS_MAX_SPI_XFER_SIZE || rlen > CFG_CTS_MAX_SPI_XFER_SIZE) {
        TPD_INFO("<E> write/read too much:wlen=%zd, rlen=%zd\n", wlen, rlen);
        return -EIO;
    }

    if (pdata->cts_dev->rtdata.program_mode)
    {
        pdata->spi_tx_buf[0] = dev_addr | 0x01;
        memcpy(&pdata->spi_tx_buf[1], wbuf, wlen);
        do {
            ret = cts_spi_send_recv(pdata, rlen + 5, pdata->spi_tx_buf,
                    pdata->spi_rx_buf);
            if (ret) {
                if (delay) {
                    mdelay(delay);
                }
                continue;
            }
            memcpy(rbuf, pdata->spi_rx_buf+5, rlen);
            return 0;
        } while(++retries < retry);
    }
    else {
        do {
            if (wlen != 0) {
                pdata->spi_tx_buf[0] = dev_addr | 0x01;
                pdata->spi_tx_buf[1] = wbuf[1];
                pdata->spi_tx_buf[2] = wbuf[0];
                put_unaligned_le16(rlen, &pdata->spi_tx_buf[3]);
                crc = (u16)cts_crc32(pdata->spi_tx_buf, 5);
                put_unaligned_le16(crc, &pdata->spi_tx_buf[5]);
                ret = cts_spi_send_recv(pdata, 7, pdata->spi_tx_buf,
                        pdata->spi_rx_buf);
                if (ret) {
                    if (delay) {
                        mdelay(delay);
                    }
                    continue;
                }
            }
            memset(pdata->spi_tx_buf, 0, 7);
            pdata->spi_tx_buf[0] = dev_addr | 0x01;
            udelay(100);
                ret = cts_spi_send_recv(pdata, rlen + 2, pdata->spi_tx_buf,
                        pdata->spi_rx_buf);
            if (ret) {
                if (delay) {
                    mdelay(delay);
                }
                continue;
            }
            memcpy(rbuf, pdata->spi_rx_buf, rlen);
            crc = (u16)cts_crc32(pdata->spi_rx_buf, rlen);
            if (get_unaligned_le16(&pdata->spi_rx_buf[rlen]) != crc) {
                continue;
            }
            return 0;
        } while (++retries < retry);
    }
    if (retries >= retry) {
        TPD_INFO("<E> cts_plat_spi_read error\n");
    }

    return -ENODEV;
}

static int cts_plat_spi_read_delay_idle(struct cts_platform_data *pdata,
        u8 dev_addr, const u8 *wbuf, size_t wlen, void *rbuf, size_t rlen,
        int retry, int delay, int idle)
{
    int ret = 0, retries = 0;
    u16 crc;

    if (wlen > CFG_CTS_MAX_SPI_XFER_SIZE || rlen > CFG_CTS_MAX_SPI_XFER_SIZE) {
        TPD_INFO("<E> write/read too much:wlen=%zd, rlen=%zd\n", wlen, rlen);
        return -EIO;
    }

    if (pdata->cts_dev->rtdata.program_mode)
    {
        pdata->spi_tx_buf[0] = dev_addr | 0x01;
        memcpy(&pdata->spi_tx_buf[1], wbuf, wlen);
        do {
            ret = cts_spi_send_recv(pdata, rlen + 5, pdata->spi_tx_buf,
                    pdata->spi_rx_buf);
            if (ret) {
                if (delay) {
                    mdelay(delay);
                }
                continue;
            }
            memcpy(rbuf, pdata->spi_rx_buf+5, rlen);
            return 0;
        } while(++retries < retry);
    }
    else {
        do {
            if (wlen != 0) {
                pdata->spi_tx_buf[0] = dev_addr | 0x01;
                pdata->spi_tx_buf[1] = wbuf[1];
                pdata->spi_tx_buf[2] = wbuf[0];
                put_unaligned_le16(rlen, &pdata->spi_tx_buf[3]);
                crc = (u16)cts_crc32(pdata->spi_tx_buf, 5);
                put_unaligned_le16(crc, &pdata->spi_tx_buf[5]);
                ret = cts_spi_send_recv(pdata, 7, pdata->spi_tx_buf,
                        pdata->spi_rx_buf);
                if (ret) {
                    if (delay) {
                        mdelay(delay);
                    }
                    continue;
                }
            }
            memset(pdata->spi_tx_buf, 0, 7);
            pdata->spi_tx_buf[0] = dev_addr | 0x01;
            udelay(idle);
            ret = cts_spi_send_recv(pdata, rlen + 2, pdata->spi_tx_buf,
                    pdata->spi_rx_buf);
            if (ret) {
                if (delay) {
                    mdelay(delay);
                }
                continue;
            }
            memcpy(rbuf, pdata->spi_rx_buf, rlen);
            crc = (u16)cts_crc32(pdata->spi_rx_buf, rlen);
            if (get_unaligned_le16(&pdata->spi_rx_buf[rlen]) != crc) {
               continue;
            }
            return 0;
        } while (++retries < retry);
    }
    if (retries >= retry) {
        TPD_INFO("<E> cts_plat_spi_read error\n");
    }

    return -ENODEV;
}

int cts_plat_is_normal_mode(struct cts_platform_data *pdata)
{
    u16 fwid;
    int ret;

    TPD_DEBUG("<I> Enter cts_plat_is_normal_mode\n");

    cts_set_normal_addr(pdata->cts_dev);

    ret = pdata->cts_dev->cts_if->get_fwid(pdata->cts_dev, &fwid);

    if (ret || !cts_is_fwid_valid(fwid))
        return false;

    return true;
}
#endif /* CONFIG_CTS_I2C_HOST */


#ifdef CONFIG_CTS_I2C_HOST
static int cts_i2c_writeb(const struct cts_device *cts_dev,
        u32 addr, u8 b, int retry, int delay)
{
    u8  buff[8];

    TPD_DEBUG("<D> Write to slave_addr: 0x%02x reg: 0x%0*x val: 0x%02x\n",
        cts_dev->rtdata.slave_addr, cts_dev->rtdata.addr_width * 2, addr, b);

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, buff);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, buff);
    } else {
        TPD_INFO("<E> Writeb invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }
    buff[cts_dev->rtdata.addr_width] = b;

    return cts_plat_i2c_write(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            buff, cts_dev->rtdata.addr_width + 1, retry ,delay);
}

static int cts_i2c_writew(const struct cts_device *cts_dev,
        u32 addr, u16 w, int retry, int delay)
{
    u8  buff[8];

    TPD_DEBUG("<D> Write to slave_addr: 0x%02x reg: 0x%0*x val: 0x%04x\n",
        cts_dev->rtdata.slave_addr, cts_dev->rtdata.addr_width * 2, addr, w);

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, buff);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, buff);
    } else {
        TPD_INFO("<E> Writew invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    put_unaligned_le16(w, buff + cts_dev->rtdata.addr_width);

    return cts_plat_i2c_write(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            buff, cts_dev->rtdata.addr_width + 2, retry, delay);
}

static int cts_i2c_writel(const struct cts_device *cts_dev,
        u32 addr, u32 l, int retry, int delay)
{
    u8  buff[8];

    TPD_DEBUG("<D> Write to slave_addr: 0x%02x reg: 0x%0*x val: 0x%08x\n",
        cts_dev->rtdata.slave_addr, cts_dev->rtdata.addr_width * 2, addr, l);

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, buff);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, buff);
    } else {
        TPD_INFO("<E> Writel invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    put_unaligned_le32(l, buff + cts_dev->rtdata.addr_width);

    return cts_plat_i2c_write(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            buff, cts_dev->rtdata.addr_width + 4, retry, delay);
}

static int cts_i2c_writesb(const struct cts_device *cts_dev, u32 addr,
        const u8 *src, size_t len, int retry, int delay)
{
    int ret;
    u8 *data;
    size_t max_xfer_size;
    size_t payload_len;
    size_t xfer_len;

    TPD_DEBUG("<D> Write to slave_addr: 0x%02x reg: 0x%0*x len: %zu\n",
        cts_dev->rtdata.slave_addr, cts_dev->rtdata.addr_width * 2, addr, len);

    max_xfer_size = cts_plat_get_max_i2c_xfer_size(cts_dev->pdata);
    data = cts_plat_get_i2c_xfer_buf(cts_dev->pdata, len);
    while (len) {
        payload_len =
            min((size_t)(max_xfer_size - cts_dev->rtdata.addr_width), len);
        xfer_len = payload_len + cts_dev->rtdata.addr_width;

        if (cts_dev->rtdata.addr_width == 2) {
            put_unaligned_be16(addr, data);
        } else if (cts_dev->rtdata.addr_width == 3) {
            put_unaligned_be24(addr, data);
        } else {
            TPD_INFO("<E> Writesb invalid address width %u\n",
                cts_dev->rtdata.addr_width);
            return -EINVAL;
        }

        memcpy(data + cts_dev->rtdata.addr_width, src, payload_len);

        ret = cts_plat_i2c_write(cts_dev->pdata, cts_dev->rtdata.slave_addr,
                data, xfer_len, retry, delay);
        if (ret) {
            TPD_INFO("<E> Platform i2c write failed %d\n", ret);
            return ret;
        }

        src  += payload_len;
        len  -= payload_len;
        addr += payload_len;
    }

    return 0;
}

static int cts_i2c_readb(const struct cts_device *cts_dev,
        u32 addr, u8 *b, int retry, int delay)
{
    u8 addr_buf[4];

    TPD_DEBUG("<D> Readb from slave_addr: 0x%02x reg: 0x%0*x\n",
        cts_dev->rtdata.slave_addr, cts_dev->rtdata.addr_width * 2, addr);

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, addr_buf);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, addr_buf);
    } else {
        TPD_INFO("<E> Readb invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    return cts_plat_i2c_read(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            addr_buf, cts_dev->rtdata.addr_width, b, 1, retry, delay);
}

static int cts_i2c_readw(const struct cts_device *cts_dev,
        u32 addr, u16 *w, int retry, int delay)
{
    int ret;
    u8  addr_buf[4];
    u8  buff[2];

    TPD_DEBUG("<D> Readw from slave_addr: 0x%02x reg: 0x%0*x\n",
        cts_dev->rtdata.slave_addr, cts_dev->rtdata.addr_width * 2, addr);

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, addr_buf);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, addr_buf);
    } else {
        TPD_INFO("<E> Readw invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    ret = cts_plat_i2c_read(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            addr_buf, cts_dev->rtdata.addr_width, buff, 2, retry, delay);
    if (ret == 0) {
        *w = get_unaligned_le16(buff);
    }

    return ret;
}

static int cts_i2c_readl(const struct cts_device *cts_dev,
        u32 addr, u32 *l, int retry, int delay)
{
    int ret;
    u8  addr_buf[4];
    u8  buff[4];

    TPD_DEBUG("<D> Readl from slave_addr: 0x%02x reg: 0x%0*x\n",
        cts_dev->rtdata.slave_addr, cts_dev->rtdata.addr_width * 2, addr);

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, addr_buf);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, addr_buf);
    } else {
        TPD_INFO("<E> Readl invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    ret = cts_plat_i2c_read(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            addr_buf, cts_dev->rtdata.addr_width, buff, 4, retry, delay);
    if (ret == 0) {
        *l = get_unaligned_le32(buff);
    }

    return ret;
}

static int cts_i2c_readsb(const struct cts_device *cts_dev,
        u32 addr, void *dst, size_t len, int retry, int delay)
{
    int ret;
    u8 addr_buf[4];
    size_t max_xfer_size, xfer_len;

    TPD_DEBUG("<D> Readsb from slave_addr: 0x%02x reg: 0x%0*x len: %zu\n",
        cts_dev->rtdata.slave_addr, cts_dev->rtdata.addr_width * 2, addr, len);

    max_xfer_size = cts_plat_get_max_i2c_xfer_size(cts_dev->pdata);
    while (len) {
        xfer_len = min(max_xfer_size, len);

        if (cts_dev->rtdata.addr_width == 2) {
            put_unaligned_be16(addr, addr_buf);
        } else if (cts_dev->rtdata.addr_width == 3) {
            put_unaligned_be24(addr, addr_buf);
        } else {
            TPD_INFO("<E> Readsb invalid address width %u\n",
                cts_dev->rtdata.addr_width);
            return -EINVAL;
        }

        ret = cts_plat_i2c_read(cts_dev->pdata, cts_dev->rtdata.slave_addr,
                addr_buf, cts_dev->rtdata.addr_width, dst, xfer_len, retry, delay);
        if (ret) {
            TPD_INFO("<E> Platform i2c read failed %d\n", ret);
            return ret;
        }

        dst  += xfer_len;
        len  -= xfer_len;
        addr += xfer_len;
    }

    return 0;
}
#else
static int cts_spi_writeb(const struct cts_device *cts_dev,
        u32 addr, u8 b, int retry, int delay)
{
    u8  buff[8];

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, buff);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, buff);
    } else {
        TPD_INFO("<E> Writeb invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }
    buff[cts_dev->rtdata.addr_width] = b;

    return cts_plat_spi_write(cts_dev->pdata, cts_dev->rtdata.slave_addr, buff,
        cts_dev->rtdata.addr_width + 1, retry ,delay);
}

static int cts_spi_writew(const struct cts_device *cts_dev,
        u32 addr, u16 w, int retry, int delay)
{
    u8  buff[8];

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, buff);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, buff);
    } else {
        TPD_INFO("<E> Writew invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    put_unaligned_le16(w, buff + cts_dev->rtdata.addr_width);

    return cts_plat_spi_write(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            buff, cts_dev->rtdata.addr_width + 2, retry, delay);
}

static int cts_spi_writel(const struct cts_device *cts_dev,
        u32 addr, u32 l, int retry, int delay)
{
    u8  buff[8];

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, buff);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, buff);
    } else {
        TPD_INFO("<E> Writel invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    put_unaligned_le32(l, buff + cts_dev->rtdata.addr_width);

    return cts_plat_spi_write(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            buff, cts_dev->rtdata.addr_width + 4, retry, delay);
}

static int cts_spi_writesb(const struct cts_device *cts_dev, u32 addr,
        const u8 *src, size_t len, int retry, int delay)
{
    int ret;
    u8 *data;
    size_t max_xfer_size;
    size_t payload_len;
    size_t xfer_len;

    max_xfer_size = cts_plat_get_max_spi_xfer_size(cts_dev->pdata);
    data = cts_plat_get_spi_xfer_buf(cts_dev->pdata, len);
    while (len) {
        payload_len =
            min((size_t)(max_xfer_size - cts_dev->rtdata.addr_width), len);
        xfer_len = payload_len + cts_dev->rtdata.addr_width;

        if (cts_dev->rtdata.addr_width == 2) {
            put_unaligned_be16(addr, data);
        } else if (cts_dev->rtdata.addr_width == 3) {
            put_unaligned_be24(addr, data);
        } else {
            TPD_INFO("<E> Writesb invalid address width %u\n",
                cts_dev->rtdata.addr_width);
            return -EINVAL;
        }

        memcpy(data + cts_dev->rtdata.addr_width, src, payload_len);

        ret = cts_plat_spi_write(cts_dev->pdata, cts_dev->rtdata.slave_addr,
                data, xfer_len, retry, delay);
        if (ret) {
            TPD_INFO("<E> Platform i2c write failed %d\n", ret);
            return ret;
        }

        src  += payload_len;
        len  -= payload_len;
        addr += payload_len;
    }
    return 0;
}

static int cts_spi_readb(const struct cts_device *cts_dev,
        u32 addr, u8 *b, int retry, int delay)
{
    u8 addr_buf[4];

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, addr_buf);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, addr_buf);
    } else {
        TPD_INFO("<E> Readb invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    return cts_plat_spi_read(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            addr_buf, cts_dev->rtdata.addr_width, b, 1, retry, delay);
}

static int cts_spi_readw(const struct cts_device *cts_dev,
        u32 addr, u16 *w, int retry, int delay)
{
    int ret;
    u8  addr_buf[4];
    u8  buff[2];

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, addr_buf);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, addr_buf);
    } else {
        TPD_INFO("<E> Readw invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    ret = cts_plat_spi_read(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            addr_buf, cts_dev->rtdata.addr_width, buff, 2, retry, delay);
    if (ret == 0) {
        *w = get_unaligned_le16(buff);
    }

    return ret;
}

static int cts_spi_readl(const struct cts_device *cts_dev,
        u32 addr, u32 *l, int retry, int delay)
{
    int ret;
    u8  addr_buf[4];
    u8  buff[4];

    if (cts_dev->rtdata.addr_width == 2) {
        put_unaligned_be16(addr, addr_buf);
    } else if (cts_dev->rtdata.addr_width == 3) {
        put_unaligned_be24(addr, addr_buf);
    } else {
        TPD_INFO("<E> Readl invalid address width %u\n",
            cts_dev->rtdata.addr_width);
        return -EINVAL;
    }

    ret = cts_plat_spi_read(cts_dev->pdata, cts_dev->rtdata.slave_addr,
            addr_buf, cts_dev->rtdata.addr_width, buff, 4, retry, delay);
    if (ret == 0) {
        *l = get_unaligned_le32(buff);
    }

    return ret;
}

static int cts_spi_readsb(const struct cts_device *cts_dev,
        u32 addr, void *dst, size_t len, int retry, int delay)
{
    int ret;
    u8 addr_buf[4];
    size_t max_xfer_size, xfer_len;

    max_xfer_size = cts_plat_get_max_spi_xfer_size(cts_dev->pdata);
    while (len) {
        xfer_len = min(max_xfer_size, len);

        if (cts_dev->rtdata.addr_width == 2) {
            put_unaligned_be16(addr, addr_buf);
        } else if (cts_dev->rtdata.addr_width == 3) {
            put_unaligned_be24(addr, addr_buf);
        } else {
            TPD_INFO("<E> Readsb invalid address width %u\n",
                cts_dev->rtdata.addr_width);
            return -EINVAL;
        }

        ret = cts_plat_spi_read(cts_dev->pdata, cts_dev->rtdata.slave_addr,
                addr_buf, cts_dev->rtdata.addr_width,
                dst, xfer_len, retry, delay);
        if (ret) {
            TPD_INFO("<E> Platform i2c read failed %d\n", ret);
            return ret;
        }

        dst  += xfer_len;
        len  -= xfer_len;
        addr += xfer_len;
    }
    return 0;
}

static int cts_spi_readsb_delay_idle(const struct cts_device *cts_dev,
        u32 addr, void *dst, size_t len, int retry, int delay, int idle)
{
    int ret;
    u8 addr_buf[4];
    size_t max_xfer_size, xfer_len;

    max_xfer_size = cts_plat_get_max_spi_xfer_size(cts_dev->pdata);
    while (len) {
        xfer_len = min(max_xfer_size, len);

        if (cts_dev->rtdata.addr_width == 2) {
            put_unaligned_be16(addr, addr_buf);
        } else if (cts_dev->rtdata.addr_width == 3) {
            put_unaligned_be24(addr, addr_buf);
        } else {
            TPD_INFO("<E> Readsb invalid address width %u\n",
                cts_dev->rtdata.addr_width);
            return -EINVAL;
        }

        ret = cts_plat_spi_read_delay_idle(cts_dev->pdata,
                cts_dev->rtdata.slave_addr, addr_buf,
                cts_dev->rtdata.addr_width, dst, xfer_len,
                retry, delay, idle);
        if (ret) {
            TPD_INFO("<E> Platform i2c read failed %d\n", ret);
            return ret;
        }

        dst  += xfer_len;
        len  -= xfer_len;
        addr += xfer_len;
    }
    return 0;
}

#endif /* CONFIG_CTS_I2C_HOST */

static inline int cts_dev_writeb(const struct cts_device *cts_dev,
        u32 addr, u8 b, int retry, int delay)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_writeb(cts_dev, addr, b, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_writeb(cts_dev, addr, b, retry, delay);
#endif /* CONFIG_CTS_I2C_HOST */
}

static inline int cts_dev_writew(const struct cts_device *cts_dev,
        u32 addr, u16 w, int retry, int delay)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_writew(cts_dev, addr, w, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_writew(cts_dev, addr, w, retry, delay);
#endif /* CONFIG_CTS_I2C_HOST */
}

static inline int cts_dev_writel(const struct cts_device *cts_dev,
        u32 addr, u32 l, int retry, int delay)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_writel(cts_dev, addr, l, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_writel(cts_dev, addr, l, retry, delay);
#endif /* CONFIG_CTS_I2C_HOST */
}

static inline int cts_dev_writesb(const struct cts_device *cts_dev, u32 addr,
        const u8 *src, size_t len, int retry, int delay)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_writesb(cts_dev, addr, src, len, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_writesb(cts_dev, addr, src, len, retry, delay);
#endif /* CONFIG_CTS_I2C_HOST */
}

static inline int cts_dev_readb(const struct cts_device *cts_dev,
        u32 addr, u8 *b, int retry, int delay)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_readb(cts_dev, addr, b, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_readb(cts_dev, addr, b, retry, delay);
#endif /* CONFIG_CTS_I2C_HOST */
}

static inline int cts_dev_readw(const struct cts_device *cts_dev,
        u32 addr, u16 *w, int retry, int delay)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_readw(cts_dev, addr, w, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_readw(cts_dev, addr, w, retry, delay);;
#endif /* CONFIG_CTS_I2C_HOST */
}

static inline int cts_dev_readl(const struct cts_device *cts_dev,
        u32 addr, u32 *l, int retry, int delay)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_readl(cts_dev, addr, l, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_readl(cts_dev, addr, l, retry, delay);
#endif /* CONFIG_CTS_I2C_HOST */
}

static inline int cts_dev_readsb(const struct cts_device *cts_dev,
        u32 addr, void *dst, size_t len, int retry, int delay)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_readsb(cts_dev, addr, dst, len, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_readsb(cts_dev, addr, dst, len, retry, delay);
#endif /* CONFIG_CTS_I2C_HOST */
}

static inline int cts_dev_readsb_delay_idle(const struct cts_device *cts_dev,
        u32 addr, void *dst, size_t len, int retry, int delay, int idle)
{
#ifdef CONFIG_CTS_I2C_HOST
    return cts_i2c_readsb(cts_dev, addr, dst, len, retry, delay);
#else /* CONFIG_CTS_I2C_HOST */
    return cts_spi_readsb_delay_idle(cts_dev, addr, dst, len, retry, delay, idle);
#endif /* CONFIG_CTS_I2C_HOST */
}

static int cts_write_sram_normal_mode(const struct cts_device *cts_dev,
        u32 addr, const void *src, size_t len, int retry, int delay)
{
    int i, ret;
    u8    buff[5];

    for (i = 0; i < len; i++) {
        put_unaligned_le32(addr, buff);
        buff[4] = *(u8 *)src;

        addr++;
        src++;

        ret = cts_dev_writesb(cts_dev,
                CTS_DEVICE_FW_REG_DEBUG_INTF, buff, 5, retry, delay);
        if (ret) {
            TPD_INFO("<E> Write rDEBUG_INTF len=5B failed %d\n",
                    ret);
            return ret;
        }
    }

    return 0;
}

int cts_sram_writeb_retry(const struct cts_device *cts_dev,
        u32 addr, u8 b, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        return cts_dev_writeb(cts_dev, addr, b, retry, delay);
    } else {
        return cts_write_sram_normal_mode(cts_dev, addr, &b, 1, retry, delay);
    }
}

int cts_sram_writew_retry(const struct cts_device *cts_dev,
        u32 addr, u16 w, int retry, int delay)
{
    u8 buff[2];

    if (cts_dev->rtdata.program_mode) {
        return cts_dev_writew(cts_dev, addr, w, retry, delay);
    } else {
        put_unaligned_le16(w, buff);

        return cts_write_sram_normal_mode(cts_dev, addr, buff, 2, retry, delay);
    }
}

int cts_sram_writel_retry(const struct cts_device *cts_dev,
        u32 addr, u32 l, int retry, int delay)
{
    u8 buff[4];

    if (cts_dev->rtdata.program_mode) {
        return cts_dev_writel(cts_dev, addr, l, retry, delay);
    } else {
        put_unaligned_le32(l, buff);

        return cts_write_sram_normal_mode(cts_dev, addr, buff, 4, retry, delay);
    }
}

int cts_sram_writesb_retry(const struct cts_device *cts_dev,
        u32 addr, const void *src, size_t len, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        return cts_dev_writesb(cts_dev, addr, src, len, retry, delay);
    } else {
        return cts_write_sram_normal_mode(cts_dev, addr, src, len, retry, delay);
    }
}

static int cts_calc_sram_crc(const struct cts_device *cts_dev,
    u32 sram_addr, size_t size, u32 *crc)
{
    TPD_INFO("<I> Calc crc from sram 0x%06x size %zu\n", sram_addr, size);

    return cts_dev->hwdata->sfctrl->ops->calc_sram_crc(cts_dev,
        sram_addr, size, crc);
}

int cts_sram_writesb_check_crc_retry(const struct cts_device *cts_dev,
        u32 addr, const void *src, size_t len, u32 crc, int retry)
{
    int ret, retries;

    retries = 0;
    do {
        u32 crc_sram;

        retries++;

        if ((ret = cts_sram_writesb(cts_dev, 0, src, len)) != 0) {
            TPD_INFO("<E> SRAM writesb failed %d\n", ret);
            continue;
        }

        if ((ret = cts_calc_sram_crc(cts_dev, 0, len, &crc_sram)) != 0) {
            TPD_INFO("<E> Get CRC for sram writesb failed %d retries %d\n",
                ret, retries);
            continue;
        }

        if (crc == crc_sram) {
            return 0;
        }

        TPD_INFO("<E> Check CRC for sram writesb mismatch %x != %x retries %d\n",
                crc, crc_sram, retries);
        ret = -EFAULT;
    }while (retries < retry);

    return ret;
}
int cts_sram_writesb_boot_crc_retry(const struct cts_device *cts_dev,
        size_t len, u32 crc, int retry)
{
    int ret = 0, retries;
    u32 addr[3];

    if (cts_dev->hwdata->hwid == CTS_DEV_HWID_ICNL9911C) {
        addr[0] = 0x015ff0;
        addr[1] = 0x08fffc;
        addr[2] = 0x08fff8;
    } else if (cts_dev->hwdata->hwid == CTS_DEV_HWID_ICNL9916) {
        addr[0] = 0x01fff0;
        addr[1] = 0x01fff8;
        addr[2] = 0x01fffc;
    } else {

        return -EIO;
    }

    retries = 0;
    do {
        ret = cts_dev_writel(cts_dev, addr[0], 0xCC33555A, 3, 1);
        if (ret != 0) {
            TPD_INFO("<E> SRAM writesb failed %d\n", ret);
            continue;
        }

        ret = cts_dev_writel(cts_dev, addr[1], crc, 3, 1);
        if (ret != 0) {
            TPD_INFO("<E> SRAM writesb failed %d\n", ret);
            continue;
        }

        ret = cts_dev_writel(cts_dev, addr[2], len, 3, 1);
        if (ret != 0) {
            TPD_INFO("<E> SRAM writesb failed %d\n", ret);
            continue;
        }

        break;
    } while (retries++ < retry);

    return ret;
}

static int cts_read_sram_normal_mode(const struct cts_device *cts_dev,
        u32 addr, void *dst, size_t len, int retry, int delay)
{
    int i, ret;

    for (i = 0; i < len; i++) {
        ret = cts_dev_writel(cts_dev,
                CTS_DEVICE_FW_REG_DEBUG_INTF, addr, retry, delay);
        if (ret) {
            TPD_INFO("<E> Write addr to rDEBUG_INTF failed %d\n", ret);
            return ret;
        }

        ret = cts_dev_readb(cts_dev,
                CTS_DEVICE_FW_REG_DEBUG_INTF + 4, (u8 *)dst, retry, delay);
        if (ret) {
            TPD_INFO("<E> Read value from rDEBUG_INTF + 4 failed %d\n",
                ret);
            return ret;
        }

        addr++;
        dst++;
    }

    return 0;
}

int cts_sram_readb_retry(const struct cts_device *cts_dev,
        u32 addr, u8 *b, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        return cts_dev_readb(cts_dev, addr, b, retry, delay);
    } else {
        return cts_read_sram_normal_mode(cts_dev, addr, b, 1, retry, delay);
    }
}

int cts_sram_readw_retry(const struct cts_device *cts_dev,
        u32 addr, u16 *w, int retry, int delay)
{
    int ret;
    u8 buff[2];

    if (cts_dev->rtdata.program_mode) {
        return cts_dev_readw(cts_dev, addr, w, retry, delay);
    } else {
        ret = cts_read_sram_normal_mode(cts_dev, addr, buff, 2, retry, delay);
        if (ret) {
            TPD_INFO("<E> SRAM readw in normal mode failed %d\n", ret);
            return ret;
        }

        *w = get_unaligned_le16(buff);

        return 0;
    }
}

int cts_sram_readl_retry(const struct cts_device *cts_dev,
        u32 addr, u32 *l, int retry, int delay)
{
    int ret;
    u8 buff[4];

    if (cts_dev->rtdata.program_mode) {
        return cts_dev_readl(cts_dev, addr, l, retry, delay);
    } else {
        ret = cts_read_sram_normal_mode(cts_dev, addr, buff, 4, retry, delay);
        if (ret) {
            TPD_INFO("<E> SRAM readl in normal mode failed %d\n", ret);
            return ret;
        }

        *l = get_unaligned_le32(buff);

        return 0;
    }
}

int cts_sram_readsb_retry(const struct cts_device *cts_dev,
        u32 addr, void *dst, size_t len, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        return cts_dev_readsb(cts_dev, addr, dst, len, retry, delay);
    } else {
        return cts_read_sram_normal_mode(cts_dev, addr, dst, len, retry, delay);
    }
}

int cts_fw_reg_writeb_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u8 b, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Writeb to fw reg 0x%04x under program mode\n", reg_addr);
        return -ENODEV;
    }

    return cts_dev_writeb(cts_dev, reg_addr, b, retry, delay);
}

int cts_fw_reg_writew_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u16 w, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Writew to fw reg 0x%04x under program mode\n", reg_addr);
        return -ENODEV;
    }

    return cts_dev_writew(cts_dev, reg_addr, w, retry, delay);
}

int cts_fw_reg_writel_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u32 l, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Writel to fw reg 0x%04x under program mode\n", reg_addr);
        return -ENODEV;
    }

    return cts_dev_writel(cts_dev, reg_addr, l, retry, delay);
}

int cts_fw_reg_writesb_retry(const struct cts_device *cts_dev,
        u32 reg_addr, const void *src, size_t len, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Writesb to fw reg 0x%04x under program mode\n", reg_addr);
        return -ENODEV;
    }

    return cts_dev_writesb(cts_dev, reg_addr, src, len, retry, delay);
}

int cts_fw_reg_readb_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u8 *b, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Readb from fw reg under program mode\n");
        return -ENODEV;
    }

    return cts_dev_readb(cts_dev, reg_addr, b, retry, delay);
}

int cts_fw_reg_readw_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u16 *w, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Readw from fw reg under program mode\n");
        return -ENODEV;
    }

    return cts_dev_readw(cts_dev, reg_addr, w, retry, delay);
}

int cts_fw_reg_readl_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u32 *l, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Readl from fw reg under program mode\n");
        return -ENODEV;
    }

    return cts_dev_readl(cts_dev, reg_addr, l, retry, delay);
}

int cts_fw_reg_readsb_retry(const struct cts_device *cts_dev,
        u32 reg_addr, void *dst, size_t len, int retry, int delay)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Readsb from fw reg under program mode\n");
        return -ENODEV;
    }

    return cts_dev_readsb(cts_dev, reg_addr, dst, len, retry, delay);
}
int cts_fw_reg_readsb_retry_delay_idle(const struct cts_device *cts_dev,
        u32 reg_addr, void *dst, size_t len, int retry, int delay, int idle)
{
    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Readsb from fw reg under program mode\n");
        return -ENODEV;
    }

    return cts_dev_readsb_delay_idle(cts_dev, reg_addr, dst, len,
        retry, delay, idle);
}

int cts_hw_reg_writeb_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u8 b, int retry, int delay)
{
    return cts_sram_writeb_retry(cts_dev, reg_addr, b, retry, delay);
}

int cts_hw_reg_writew_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u16 w, int retry, int delay)
{
    return cts_sram_writew_retry(cts_dev, reg_addr, w, retry, delay);
}

int cts_hw_reg_writel_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u32 l, int retry, int delay)
{
    return cts_sram_writel_retry(cts_dev, reg_addr, l, retry, delay);
}

int cts_hw_reg_writesb_retry(const struct cts_device *cts_dev,
        u32 reg_addr, const void *src, size_t len, int retry, int delay)
{
    return cts_sram_writesb_retry(cts_dev, reg_addr, src, len, retry, delay);
}

int cts_hw_reg_readb_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u8 *b, int retry, int delay)
{
    return cts_sram_readb_retry(cts_dev, reg_addr, b, retry, delay);
}

int cts_hw_reg_readw_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u16 *w, int retry, int delay)
{
    return cts_sram_readw_retry(cts_dev, reg_addr, w, retry, delay);
}

int cts_hw_reg_readl_retry(const struct cts_device *cts_dev,
        u32 reg_addr, u32 *l, int retry, int delay)
{
    return cts_sram_readl_retry(cts_dev, reg_addr, l, retry, delay);
}

int cts_hw_reg_readsb_retry(const struct cts_device *cts_dev,
        u32 reg_addr, void *dst, size_t len, int retry, int delay)
{
    return cts_sram_readsb_retry(cts_dev, reg_addr, dst, len, retry, delay);
}

const static struct cts_sfctrl icnl9911_sfctrl = {
    .reg_base = 0x34000,
    .xchg_sram_base = (80 - 1) * 1024,
    .xchg_sram_size = 1024, /* For non firmware programming */
    .ops = &cts_sfctrlv2_ops
};

const static struct cts_sfctrl icnl9911s_sfctrl = {
    .reg_base = 0x34000,
    .xchg_sram_base = (64 - 1) * 1024,
    .xchg_sram_size = 1024, /* For non firmware programming */
    .ops = &cts_sfctrlv2_ops
};

const static struct cts_sfctrl icnl9911c_sfctrl = {
    .reg_base = 0x34000,
    .xchg_sram_base = 64 * 1024,
    .xchg_sram_size = 1024, /* For non firmware programming */
    .ops = &cts_sfctrlv2_ops
};

const static struct cts_sfctrl icnl9916_sfctrl = {
    .reg_base = 0x34000,
    .xchg_sram_base = 96 * 1024,
    .xchg_sram_size = 32 * 1024,    /* For non firmware programming */
    .ops = &cts_sfctrlv2_ops
};
#define CTS_DEV_HW_REG_DDI_REG_CTRL     (0x3002Cu)

static int icnl9911_set_access_ddi_reg(struct cts_device *cts_dev, bool enable)
{
    int ret;
    u8  access_flag;

    TPD_INFO("<I> ICNL9911 %s access ddi reg\n", enable ? "enable" : "disable");

    ret = cts_hw_reg_readb(cts_dev, CTS_DEV_HW_REG_DDI_REG_CTRL, &access_flag);
    if (ret) {
        TPD_INFO("<E> Read HW_REG_DDI_REG_CTRL failed %d\n", ret);
        return ret;
    }

    access_flag = enable ? (access_flag | 0x01) : (access_flag & (~0x01));
    ret = cts_hw_reg_writeb(cts_dev, CTS_DEV_HW_REG_DDI_REG_CTRL, access_flag);
    if (ret) {
        TPD_INFO("<E> Write HW_REG_DDI_REG_CTRL %02x failed %d\n", access_flag, ret);
        return ret;
    }

    ret = cts_hw_reg_writew(cts_dev, 0x3DFF0, enable ? 0x4BB4 : 0xB44B);
    if (ret) {
        TPD_INFO("<E> Write password failed\n");
        goto disable_access_ddi_reg;
    }

    return 0;

disable_access_ddi_reg: {
        int r;

        access_flag = enable ? (access_flag & (~0x01)) : (access_flag | 0x01);
        r = cts_hw_reg_writeb(cts_dev, CTS_DEV_HW_REG_DDI_REG_CTRL, access_flag);
        if (r) {
            TPD_INFO("<E> disable_access_ddi_reg\n");
        }
    }

    return ret;
}

static int icnl9911s_set_access_ddi_reg(struct cts_device *cts_dev, bool enable)
{
    int ret;
    u8  access_flag;

    TPD_INFO("<I> ICNL9911S %s access ddi reg\n", enable ? "enable" : "disable");

    ret = cts_hw_reg_readb(cts_dev, CTS_DEV_HW_REG_DDI_REG_CTRL, &access_flag);
    if (ret) {
        TPD_INFO("<E> Read HW_REG_DDI_REG_CTRL failed %d\n", ret);
        return ret;
    }

    access_flag = enable ? (access_flag | 0x01) : (access_flag & (~0x01));
    ret = cts_hw_reg_writeb(cts_dev, CTS_DEV_HW_REG_DDI_REG_CTRL, access_flag);
    if (ret) {
        TPD_INFO("<E> Write HW_REG_DDI_REG_CTRL %02x failed %d\n", access_flag, ret);
        return ret;
    }

    ret = cts_hw_reg_writeb(cts_dev, 0x30074, enable ? 1 : 0);
    if (ret) {
        TPD_INFO("<E> Write 0x30074 failed %d, %d\n", access_flag, ret);
        return ret;
    }

    ret = cts_hw_reg_writew(cts_dev, 0x3DFF0, enable ? 0x595A : 0x5A5A);
    if (ret) {
        TPD_INFO("<E> Write password to F0 failed\n");
        return ret;
    }
    ret = cts_hw_reg_writew(cts_dev, 0x3DFF4, enable ? 0xA6A5 : 0x5A5A);
    if (ret) {
        TPD_INFO("<E> Write password to F1 failed\n");
        return ret;
    }

    return 0;
}

const static struct cts_device_hwdata cts_device_hwdatas[] = {
    {
        .name = "ICNL9911",
        .hwid = CTS_DEV_HWID_ICNL9911,
        .fwid = CTS_DEV_FWID_ICNL9911,
        .num_row = 32,
        .num_col = 18,
        .sram_size = 80 * 1024,

        .program_addr_width = 3,

        .sfctrl = &icnl9911_sfctrl,
        .enable_access_ddi_reg = icnl9911_set_access_ddi_reg,
    },
    {
        .name = "ICNL9911S",
        .hwid = CTS_DEV_HWID_ICNL9911S,
        .fwid = CTS_DEV_FWID_ICNL9911S,
        .num_row = 32,
        .num_col = 18,
        .sram_size = 64 * 1024,

        .program_addr_width = 3,

        .sfctrl = &icnl9911s_sfctrl,
        .enable_access_ddi_reg = icnl9911s_set_access_ddi_reg,
    },
    {
        .name = "ICNL9911C",
        .hwid = CTS_DEV_HWID_ICNL9911C,
        .fwid = CTS_DEV_FWID_ICNL9911C,
        .num_row = 32,
        .num_col = 18,
        .sram_size = 64 * 1024,

        .program_addr_width = 3,

        .sfctrl = &icnl9911c_sfctrl,
        .enable_access_ddi_reg = icnl9911s_set_access_ddi_reg,
    },
    {
        .name = "ICNL9916",
        .hwid = CTS_DEV_HWID_ICNL9916,
        .fwid = CTS_DEV_FWID_ICNL9916,
        .num_row = 32,
        .num_col = 18,
        .sram_size = 96 * 1024,
        .program_addr_width = 3,
        .sfctrl = &icnl9916_sfctrl,
        .enable_access_ddi_reg = icnl9911s_set_access_ddi_reg,
    },
};

static int cts_init_device_hwdata(struct cts_device *cts_dev,
        u32 hwid, u16 fwid)
{
    int i;

    TPD_INFO("<I> Init hardware data hwid: %06x fwid: %04x\n", hwid, fwid);

    for (i = 0; i < ARRAY_SIZE(cts_device_hwdatas); i++) {
        if (hwid == cts_device_hwdatas[i].hwid ||
            fwid == cts_device_hwdatas[i].fwid) {
            cts_dev->hwdata = &cts_device_hwdatas[i];
            return 0;
        }
    }

    return -EINVAL;
}

const char *cts_work_mode2str(enum cts_work_mode work_mode)
{
#define case_work_mode(mode) \
    case CTS_WORK_MODE_ ## mode: return #mode "_MODE"

    switch (work_mode) {
        case_work_mode(UNKNOWN);
        case_work_mode(SUSPEND);
        case_work_mode(NORMAL_ACTIVE);
        case_work_mode(NORMAL_IDLE);
        case_work_mode(GESTURE_ACTIVE);
        case_work_mode(GESTURE_IDLE);
        default: return "INVALID";
    }

#undef case_work_mode
}

void cts_lock_device(const struct cts_device *cts_dev)
{
    TPD_DEBUG("<D> *** Lock ***\n");

    mutex_lock(&cts_dev->pdata->dev_lock);
}

void cts_unlock_device(const struct cts_device *cts_dev)
{
    TPD_DEBUG("<D> ### Un-Lock ###\n");

    mutex_unlock(&cts_dev->pdata->dev_lock);
}


static int cts_get_dev_boot_mode(const struct cts_device *cts_dev,
        u8 *boot_mode)
{
    int ret;

    if (cts_dev->rtdata.program_mode)
        ret = cts_hw_reg_readb_retry(cts_dev,
            CTS_DEV_HW_REG_CURRENT_MODE, boot_mode, 5, 10);
    else
        ret = cts_dev->cts_if->read_hw_reg(cts_dev,
            CTS_DEV_HW_REG_CURRENT_MODE, boot_mode, 1);
    if (ret) {
        TPD_INFO("<E> Read boot mode failed %d\n", ret);
        return ret;
    }

    *boot_mode &= CTS_DEV_BOOT_MODE_MASK;

    TPD_INFO("<I> Curr dev boot mode: %u(%s)\n", *boot_mode,
        cts_dev_boot_mode2str(*boot_mode));
    return 0;
}

static int cts_set_dev_boot_mode(const struct cts_device *cts_dev,
        u8 boot_mode)
{
    int ret;

    TPD_INFO("<I> Set dev boot mode to %u(%s)\n", boot_mode,
        cts_dev_boot_mode2str(boot_mode));

    ret = cts_hw_reg_writeb_retry(cts_dev, CTS_DEV_HW_REG_BOOT_MODE,
        boot_mode, 5, 5);
    if (ret) {
        TPD_INFO("<E> Write hw register BOOT_MODE failed %d\n", ret);
        return ret;
    }

    return 0;
}

static int cts_init_fwdata(struct cts_device *cts_dev)
{
    struct cts_device_fwdata *fwdata = &cts_dev->fwdata;
    struct cts_interface *cts_if = cts_dev->cts_if;
    int ret;

    TPD_INFO("<I> Init firmware data\n");

    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> Init firmware data while in program mode\n");
        return -EINVAL;
    }

    ret = cts_if->get_fw_ver(cts_dev, &fwdata->version);
    if (ret) {
        TPD_INFO("<E> Read firmware version failed %d\n", ret);
        return ret;
    }
    TPD_INFO("<I>   %-24s: %04x\n", "Firmware version", fwdata->version);

    ret = cts_if->get_lib_ver(cts_dev, &fwdata->lib_version);
    if (ret) {
        TPD_INFO("<E> Read firmware Lib version failed %d\n", ret);
    }
    TPD_INFO("<I>   %-24s: v%x.%x\n", "Fimrware lib verion",
        (u8)(fwdata->lib_version >> 8),
        (u8)(fwdata->lib_version));

    ret = cts_if->get_ddi_ver(cts_dev, &fwdata->ddi_version);
    if (ret) {
        TPD_INFO("<E> Read ddi version failed %d\n", ret);
        return ret;
    }
    TPD_INFO("<I>   %-24s: %02x\n", "DDI init code verion", fwdata->ddi_version);

    ret = cts_if->get_res_x(cts_dev, &fwdata->res_x);
    if (ret) {
        TPD_INFO("<E> Read firmware X resoltion failed %d\n", ret);
        return ret;
    }
    TPD_INFO("<I>   %-24s: %u\n", "X resolution", fwdata->res_x);

    ret = cts_if->get_res_y(cts_dev, &fwdata->res_y);
    if (ret) {
        TPD_INFO("<E> Read firmware Y resolution failed %d\n", ret);
        return ret;
    }
    TPD_INFO("<I>   %-24s: %u\n", "Y resolution", fwdata->res_y);

    ret = cts_if->get_rows(cts_dev, &fwdata->rows);
    if (ret) {
        TPD_INFO("<E> Read firmware num TX failed %d\n", ret);
        return ret;
    }
    TPD_INFO("<I>   %-24s: %u\n", "Num rows", fwdata->rows);

    ret = cts_if->get_cols(cts_dev, &fwdata->cols);
    if (ret) {
        TPD_INFO("<E> Read firmware num RX failed %d\n", ret);
        return ret;
    }
    TPD_INFO("<I>   %-24s: %u\n", "Num cols", fwdata->cols);


    ret = cts_if->get_flip_x(cts_dev, &fwdata->flip_x);
    if (ret < 0) {
        TPD_INFO("<E> Read FW_REG_FLIP_X failed %d\n", ret);
        return -EINVAL;
    }
    TPD_INFO("<I>   %-24s: %s\n", "Flip X",
        cts_dev->fwdata.flip_x ? "True" : "False");

    ret = cts_if->get_flip_y(cts_dev, &fwdata->flip_y);
    if (ret < 0) {
        TPD_INFO("<E> Read FW_REG_FLIP_Y failed %d\n", ret);
        return -EINVAL;
    }
    TPD_INFO("<I>   %-24s: %s\n", "Flip Y",
        cts_dev->fwdata.flip_y ? "True" : "False");

    ret = cts_if->get_swap_axes(cts_dev, &fwdata->swap_axes);
    if (ret < 0) {
        TPD_INFO("<E> Read FW_REG_SWAP_AXES failed %d\n", ret);
        return -EINVAL;
    }
    TPD_INFO("<I>   %-24s: %s\n", "Swap axes",
        cts_dev->fwdata.swap_axes ? "True" : "False");

    ret = cts_if->get_int_mode(cts_dev, &fwdata->int_mode);
    if (ret < 0) {
        TPD_INFO("<E> Read firmware Int mode failed %d\n", ret);
        return -EINVAL;
    }
    TPD_INFO("<I>   %-24s: %s\n", "Int polarity",
        (fwdata->int_mode == 0) ? "LOW" : "HIGH");

    ret = cts_if->get_int_keep_time(cts_dev, &fwdata->int_keep_time);
    if (ret < 0) {
        TPD_INFO("<E> Read firmware Int keep time failed %d\n", ret);
        return -EINVAL;
    }
    TPD_INFO("<I>   %-24s: %d\n", "Int keep time", fwdata->int_keep_time);

    ret = cts_if->get_rawdata_target(cts_dev, &fwdata->rawdata_target);
    if (ret < 0) {
        TPD_INFO("<E> Read firmware Raw dest value failed %d\n", ret);
        return -EINVAL;
    }
    TPD_INFO("<I>   %-24s: %d\n", "Raw target value", fwdata->rawdata_target);

    ret = cts_if->get_esd_protection(cts_dev, &fwdata->esd_method);
    if (ret < 0) {
        TPD_INFO("<E> Read firmware Esd method failed %d\n", ret);
        return -EINVAL;
    }
    TPD_INFO("<I>   %-24s: %d\n", "Esd method", fwdata->esd_method);

    if (cts_if->get_has_int_data == NULL) {
        TPD_INFO("<I> Not has int data, it's comm host protocol\n");
        return ret;
    }

    ret = cts_if->get_has_int_data(cts_dev, &fwdata->has_int_data);
    if (ret < 0) {
        TPD_INFO("<E> get_has_int_data failed: %d\n", ret);
        return -EINVAL;
    }
    TPD_INFO("<I>   %-24s: %d\n", "Has int data", fwdata->has_int_data);

    if (fwdata->has_int_data) {
        ret = cts_if->get_int_data_method(cts_dev, &fwdata->int_data_method);
        if (ret < 0) {
            TPD_INFO("<E> get_int_data_method failed: %d\n", ret);
            return -EINVAL;
        }
        if (fwdata->int_data_method >= INT_DATA_METHOD_CNT) {
            return -EINVAL;
        }
        TPD_INFO("<I>   %-24s: %d\n", "Int data method", fwdata->int_data_method);

        ret = cts_if->get_int_data_types(cts_dev, &fwdata->int_data_types);
        if (ret < 0) {
            TPD_INFO("<E> get_int_data_types failed: %d\n", ret);
            return -EINVAL;
        }
        fwdata->int_data_types &= INT_DATA_TYPE_MASK;
        TPD_INFO("<I>   %-24s: %d\n", "Int data types", fwdata->int_data_types);

        ret = cts_if->calc_int_data_size(cts_dev);
        if (ret < 0) {
            TPD_INFO("<E> calc_int_data_size failed: %d\n", ret);
            return -EINVAL;
        }
        TPD_INFO("<I>   %-24s: %lu\n", "Int data size", fwdata->int_data_size);

        ret = cts_if->init_int_data(cts_dev);
        if (ret < 0) {
            return -EINVAL;
        }
    }

    return 0;
}


bool cts_is_device_program_mode(const struct cts_device *cts_dev)
{
    return cts_dev->rtdata.program_mode;
}

static inline void cts_init_rtdata_with_normal_mode(struct cts_device *cts_dev)
{
    cts_set_normal_addr(cts_dev);
    cts_dev->rtdata.updating                = false;
    cts_dev->rtdata.testing                 = false;
}

int cts_enter_program_mode(struct cts_device *cts_dev)
{
    const static u8 magic_num[] = {0xCC, 0x33, 0x55, 0x5A};
    u8  boot_mode;
    int ret;

    TPD_INFO("<I> Enter program mode\n");

    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<W> Enter program mode while alredy in\n");
        //return 0;
    }

#ifdef CONFIG_CTS_I2C_HOST
    ret = cts_plat_i2c_write(cts_dev->pdata,
            CTS_DEV_PROGRAM_MODE_I2CADDR, magic_num, 4, 5, 10);
    if (ret) {
        TPD_INFO("<E> Write magic number to i2c_dev: 0x%02x failed %d\n",
            CTS_DEV_PROGRAM_MODE_I2CADDR, ret);
        return ret;
    }

    cts_set_program_addr(cts_dev);
    /* Write i2c deglitch register */
    ret = cts_hw_reg_writeb_retry(cts_dev, 0x37001, 0x0F, 5, 1);
    if (ret) {
        TPD_INFO("<E> Write i2c deglitch register failed\n");
        // FALL through
    }
#else
    cts_set_program_addr(cts_dev);
    cts_plat_reset_device(cts_dev->pdata);
    ret = cts_plat_spi_write(cts_dev->pdata,
            0xcc, &magic_num[1], 3, 5, 10);
    if (ret) {
        TPD_INFO("<E> Write magic number to i2c_dev: 0x%02x failed %d\n",
            CTS_DEV_PROGRAM_MODE_SPIADDR, ret);
        return ret;
    }
#endif /* CONFIG_CTS_I2C_HOST */
    ret = cts_get_dev_boot_mode(cts_dev, &boot_mode);
    if (ret) {
        TPD_INFO("<E> Read BOOT_MODE failed %d\n", ret);
        return ret;
    }

#ifdef CONFIG_CTS_I2C_HOST
    if ((boot_mode == CTS_DEV_BOOT_MODE_TCH_PRG_9916) ||
        (boot_mode == CTS_DEV_BOOT_MODE_I2C_PRG_9911C))
#else
    if ((boot_mode == CTS_DEV_BOOT_MODE_TCH_PRG_9916) ||
        (boot_mode == CTS_DEV_BOOT_MODE_SPI_PRG_9911C))
#endif
    {
        return 0;
    }
    TPD_INFO("<E> BOOT_MODE readback %u != I2C/SPI PROMGRAM mode\n", boot_mode);
    return -EFAULT;
}

const char *cts_dev_boot_mode2str(u8 boot_mode)
{
    switch (boot_mode) {
    case CTS_DEV_BOOT_MODE_IDLE:
        return "IDLE-BOOT";
    case CTS_DEV_BOOT_MODE_FLASH:
        return "FLASH-BOOT";
    case CTS_DEV_BOOT_MODE_SRAM:
        return "SRAM-BOOT";
        /* case CTS_DEV_BOOT_MODE_I2C_PRG_9911C: */
    case CTS_DEV_BOOT_MODE_TCH_PRG_9916:
        return "I2C-PRG-BOOT/TCH-PRG-BOOT";
    case CTS_DEV_BOOT_MODE_DDI_PRG:
        return "DDI-PRG-BOOT";
    case CTS_DEV_BOOT_MODE_SPI_PRG_9911C:
        return "SPI-PROG-BOOT/INVALID-BOOT";
    default:
        return "INVALID";
    }
}

int cts_enter_normal_mode(struct cts_device *cts_dev)
{
    u16 fwid = CTS_DEV_FWID_INVALID;
    u8 boot_mode;
    int retries;
    int ret = 0;

    TPD_INFO("<I> Enter normal mode\n");

    if (!cts_dev->rtdata.program_mode) {
        TPD_INFO("<W> Enter normal mode while already in\n");
        return 0;
    }

    for (retries = 5; retries > 0; retries--) {
        cts_set_program_addr(cts_dev);
        ret = cts_set_dev_boot_mode(cts_dev, CTS_DEV_BOOT_MODE_SRAM);
        if (ret)
            TPD_INFO("<E> Set BOOT_MODE to SRAM failed %d\n", ret);

        mdelay(30);

        cts_set_normal_addr(cts_dev);
        ret = cts_get_dev_boot_mode(cts_dev, &boot_mode);
        if (ret)
            TPD_INFO("<E> Get BOOT_MODE failed %d\n", ret);

        if (boot_mode != CTS_DEV_BOOT_MODE_SRAM) {
            TPD_INFO("<E> Curr boot mode %u(%s) != SRAM_BOOT\n",
                boot_mode, cts_dev_boot_mode2str(boot_mode));
            cts_plat_reset_device(cts_dev->pdata);
        } else {
            break;
        }
        TPD_INFO("<W> retry: %d\n", retries);
    }

    if (retries == 0)
        goto err_out;

    ret = cts_dev->cts_if->get_fwid(cts_dev, &fwid);
    if (ret)
        TPD_INFO("<E> Get firmware id failed %d\n", ret);

    if (fwid == CTS_DEV_FWID_ICNL9916 || fwid == CTS_DEV_FWID_ICNL9911C) {
        TPD_INFO("<I> Get firmware id successful 0x%02x\n", fwid);
        ret = cts_init_fwdata(cts_dev);
        if (ret) {
            TPD_INFO("<E> Device init firmware data failed %d\n", ret);
            return ret;
        }
        return 0;
    }

err_out:
    cts_set_program_addr(cts_dev);
    return ret;
}

bool cts_is_fwid_valid(u16 fwid)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(cts_device_hwdatas); i++) {
        if (cts_device_hwdatas[i].fwid == fwid) {
            return true;
        }
    }

    return false;
}

static bool cts_is_hwid_valid(u32 hwid)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(cts_device_hwdatas); i++) {
        if (cts_device_hwdatas[i].hwid == hwid) {
            return true;
        }
    }

    return false;
}

static int cts_get_hwid(struct cts_device *cts_dev, u32 *hwid)
{
    int ret;

    TPD_INFO("<I> Get device hardware id\n");

    if (cts_dev->hwdata) {
        *hwid = cts_dev->hwdata->hwid;
        return 0;
    }

    TPD_INFO("<I> Device hardware data not initialized, try to read from register\n");

    if (!cts_dev->rtdata.program_mode) {
        ret = cts_enter_program_mode(cts_dev);
        if (ret) {
            TPD_INFO("<E> Enter program mode failed %d\n", ret);
            goto err_out;
        }
    }

    ret = cts_hw_reg_readl_retry(cts_dev, CTS_DEV_HW_REG_HARDWARE_ID, hwid, 5, 0);
    if (ret) {
        goto err_out;
    }

    *hwid = le32_to_cpu(*hwid);
    *hwid &= 0XFFFFFFF0;
    TPD_INFO("<I> Device hardware id: %04x\n", *hwid);

    if (!cts_is_hwid_valid(*hwid)) {
        TPD_INFO("<W> Device hardware id %04x invalid\n", *hwid);
        ret = -EINVAL;
        goto err_out;
    }

    return 0;

err_out:
    *hwid = CTS_DEV_HWID_INVALID;
    return ret;
}

int cts_init_trans_buf(struct cts_device *cts_dev)
{
    cts_dev->rtdata.tbuf = kzalloc(INT_DATA_MAX_SIZ, GFP_KERNEL);
    if (cts_dev->rtdata.tbuf == NULL) {
        TPD_INFO("<E> Alloc rtdata.tbuf failed");
        return -ENOMEM;
    }
    cts_dev->rtdata.rbuf = kzalloc(INT_DATA_MAX_SIZ, GFP_KERNEL);
    if (cts_dev->rtdata.rbuf == NULL) {
        TPD_INFO("<E> Alloc rtdata.rbuf failed");
        goto err_alloc_rbuf;
    }

    return 0;
err_alloc_rbuf:
    kfree(cts_dev->rtdata.tbuf);
    return -ENOMEM;
}

void cts_deinit_trans_buf(struct cts_device *cts_dev)
{
    if (cts_dev->rtdata.tbuf) {
        kfree(cts_dev->rtdata.tbuf);
        cts_dev->rtdata.tbuf = NULL;
    }
    if (cts_dev->rtdata.rbuf) {
        kfree(cts_dev->rtdata.rbuf);
        cts_dev->rtdata.rbuf = NULL;
    }
}

int cts_probe_device(struct cts_device *cts_dev)
{
    int  ret, retries = 0;
    u16  fwid = CTS_DEV_FWID_INVALID;
    u32  hwid = CTS_DEV_HWID_INVALID;

    TPD_INFO("<I> Probe device\n");

    /** - Try to read hardware id,
        it will enter program mode as normal */
read_hwid:
    ret = cts_get_hwid(cts_dev, &hwid);
    if (ret || hwid == CTS_DEV_HWID_INVALID) {
        retries++;

        TPD_INFO("<E> Get hardware id failed %d retries %d\n", ret, retries);

        if (retries < 3) {
            cts_plat_reset_device(cts_dev->pdata);
            goto read_hwid;
        } else {
            return -ENODEV;
        }
    }

    cts_init_rtdata_with_normal_mode(cts_dev);

#ifdef CONFIG_CTS_I2C_HOST
    if (!cts_plat_is_i2c_online(cts_dev->pdata, CTS_DEV_NORMAL_MODE_I2CADDR))
        TPD_INFO("<E> Normal mode i2c addr is offline\n");
#else
	/* Will be not normal mode in no flash case, not show this msg */
    if (!cts_plat_is_normal_mode(cts_dev->pdata))
        TPD_DEBUG("<W> Normal mode spi addr is offline\n");
#endif

    ret = cts_init_device_hwdata(cts_dev, hwid, fwid);
    if (ret) {
        TPD_INFO("<E> Device hwid: %06x fwid: %04x not found\n", hwid, fwid);
        return -ENODEV;
    }

    return 0;
}
