#include "cts_config.h"
#include "cts_core.h"

#define TCS_RD_ADDR          0xF1
#define TCS_WR_ADDR          0xF0

#define INT_DATA_MAX_SIZ     8192

#pragma pack(1)
struct tcs_command {
    unsigned cmd_id:8;
    unsigned class_id:5;
    unsigned is_write:1;
    unsigned is_read:1;
    unsigned base_flag:1;
};

struct tcs_tx_head {
#ifndef CONFIG_CTS_I2C_HOST
    u8 addr;
#endif

    struct tcs_command cmd;
    u16 data_len;
    u16 crc16;
};

struct tcs_rx_tail {
    u8 ecode;
    u16 cmd;
    u16 crc16;
};
#pragma pack()


#include "cts_icnl9916.h"


#define RAWDATA_BUFFER_SIZE(cts_dev) \
			(cts_dev->hwdata->num_row * cts_dev->hwdata->num_col * 2)

#define HEADER_SIZE                    (sizeof(struct tcs_tx_head))
#define TAIL_SIZE                      (sizeof(struct tcs_rx_tail))

#define TOUCH_INFO_SIZ                 (112)
#define INT_DATA_VALID_SIZ             (62)
#define INT_DATA_INFO_SIZ              (64)
#define INT_DATA_TYPE_LEN_SIZ          (4)

static void dump_spi(const struct cts_device *cts_dev, size_t tlen, size_t rlen)
{
    static u8 str[1024];
    int offset = 0;
    int len;
    int i;

    if (LEVEL_DEBUG != tp_debug)
        return;

    if (tlen > sizeof(str))
        len = sizeof(str);
    else
        len = tlen;

    memset(str, 0, sizeof(str));
    offset += snprintf(str + offset, sizeof(str) - offset, "%s", "SPI-TX:");
    for (i = 0; i < len; i++) {
        offset += snprintf(str + offset, sizeof(str) - offset, " %02x",
            cts_dev->rtdata.tbuf[i]);
    }
    TPD_DEBUG("%s\n", str);

    if (rlen == 0)
        return;

    memset(str, 0, sizeof(str));
    offset = 0;

    if (rlen > sizeof(str))
        len = sizeof(str);
    else
        len = rlen;

    offset += snprintf(str + offset, sizeof(str) - offset, "%s", "SPI-RX:");
    for (i = 0; i < len; i++) {
        offset += snprintf(str + offset, sizeof(str) - offset, " %02x",
            cts_dev->rtdata.rbuf[i]);
    }
    TPD_DEBUG("%s\n", str);

/*
    for (i = 0; i < tlen; i++) {
        if (i % 32 == 0) {
            printk("\n");
            printk("CTS-TX:");
        }
        printk(KERN_CONT" %02x", cts_dev->rtdata.tbuf[i]);
    }

    if (rlen == 0)
        return;

    for (i = 0; i < rlen; i++) {
        if (i % 32 == 0) {
            printk("\n");
            printk("CTS-RX:");
        }
        printk(KERN_CONT" %02x", cts_dev->rtdata.rbuf[i]);
    }
*/
}


static u16 cts_crc16(const u8 *data, size_t len)
{
    u16 crc = 0;

    const static u16 crc16_table[] = {
        0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
        0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
        0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
        0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
        0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
        0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
        0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
        0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
        0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,
        0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
        0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,
        0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
        0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
        0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
        0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
        0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
        0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,
        0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
        0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,
        0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
        0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
        0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
        0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,
        0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
        0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
        0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
        0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
        0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
        0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
        0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
        0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,
        0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202,
    };

    while (len) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ *data) & 0xFF];
        data++;
        len--;
    }

    return crc;
}

static int cts_tcs_spi_pack(const struct cts_device *cts_dev, u8 class_id,
         u8 cmd_id, u8 *buf, u16 len)
{
    struct tcs_tx_head head;
    int offset = offsetof(struct tcs_tx_head, crc16);
    u8 *tbuf = cts_dev->rtdata.tbuf;
    u16 crc16;
    int i;

    for (i = 0; i < ARRAY_SIZE(TcsCmd); i++) {
        if (TcsCmd[i].class_id == class_id
        && TcsCmd[i].cmd_id == cmd_id) {
            break;
        }
    }

    if (i == ARRAY_SIZE(TcsCmd)) {
        TPD_INFO("<E> class_id and cmd_id were not found!\n");
        TPD_INFO("<E> Please add class_id:%d cmd_id:%d to cts_icnl9916.h\n",
                class_id, cmd_id);
        return -EFAULT;
    }

    head.addr = (buf == NULL)?TCS_RD_ADDR:TCS_WR_ADDR;
    head.cmd = TcsCmd[i];
    head.data_len = len;
    head.crc16 = cts_crc16((const u8 *)&head, offset);
    memcpy(tbuf, &head, HEADER_SIZE);

    if (buf != NULL && len > 0) {
        memcpy(tbuf + HEADER_SIZE, buf, len);
        crc16 = cts_crc16(buf, len);
        *(tbuf + HEADER_SIZE + len) = ((crc16 >> 0) & 0xFF);
        *(tbuf + HEADER_SIZE + len + 1) = ((crc16 >> 8) & 0xFF);
    }

    return 0;
}

static int cts_tcs_spi_xtrans(const struct cts_device *cts_dev,
        size_t tlen, size_t rlen)
{
    struct chipone_ts_data *cts_data = container_of(cts_dev,
        struct chipone_ts_data, cts_dev);
    u8 *tbuf = cts_dev->rtdata.tbuf;
    u8 *rbuf = cts_dev->rtdata.rbuf;
    struct spi_transfer xfer[2];
    struct spi_message msg;
    u16 crc16_recv;
    u16 crc16_calc;
    int ret;

    memset(&xfer[0], 0, sizeof(struct spi_transfer));
//    xfer[0].delay_usecs = 0;
    xfer[0].speed_hz = cts_dev->pdata->spi_speed * 1000u;
    xfer[0].tx_buf = tbuf;
    xfer[0].rx_buf = NULL;
    xfer[0].len = tlen;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer[0], &msg);
    ret = spi_sync(cts_data->spi_client, &msg);
    if (ret < 0) {
        TPD_INFO("<E> spi_sync xfer[0] failed: %d\n", ret);
        return ret;
    }
    udelay(100);

    memset(&xfer[1], 0, sizeof(struct spi_transfer));
//    xfer[1].delay_usecs = 0;
    xfer[1].speed_hz = cts_dev->pdata->spi_speed * 1000u;
    xfer[1].tx_buf = NULL;
    xfer[1].rx_buf = rbuf;
    xfer[1].len = rlen;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer[1], &msg);
    ret = spi_sync(cts_data->spi_client, &msg);
    if (ret < 0) {
        TPD_INFO("<E> spi_sync xfer[1] failed: %d\n", ret);
        return ret;
    }

    crc16_recv = (rbuf[rlen - 1] << 8) | rbuf[rlen - 2];
    crc16_calc = cts_crc16(rbuf, rlen - 2);
    if (crc16_recv != crc16_calc) {
        TPD_INFO("<E> xtrans crc error: recv %04x != %04x calc\n",
            crc16_recv, crc16_calc);
        return -EIO;
    }

    return 0;
}

static int cts_tcs_spi_read(const struct cts_device *cts_dev,
    u8 class_id, u8 cmd_id, u8 *buf, size_t len)
{
    int ret;

    ret = cts_tcs_spi_pack(cts_dev, class_id, cmd_id, NULL, len);
    if (ret < 0)
        return ret;

    ret = cts_tcs_spi_xtrans(cts_dev, HEADER_SIZE, len + TAIL_SIZE);

    dump_spi(cts_dev, HEADER_SIZE, len + TAIL_SIZE);

    if (ret < 0)
        return ret;

    memcpy(buf, cts_dev->rtdata.rbuf, len);

    return 0;
}

static int cts_tcs_spi_write(const struct cts_device *cts_dev,
    u8 class_id, u8 cmd_id, u8 *buf, size_t len)
{
    int tlen = HEADER_SIZE + len + 2;
    int ret;

    ret = cts_tcs_spi_pack(cts_dev, class_id, cmd_id, buf, len);
    if (ret < 0)
        return ret;

    ret = cts_tcs_spi_xtrans(cts_dev, tlen, TAIL_SIZE);

    dump_spi(cts_dev, tlen, TAIL_SIZE);

    return ret;
}

static int cts_tcs_spi_get_touch_data(const struct cts_device *cts_dev,
    u8 class_id, u8 cmd_id, u8 *buf, size_t len)
{
    struct chipone_ts_data *cts_data = container_of(cts_dev,
        struct chipone_ts_data, cts_dev);
    struct spi_transfer xfer;
    struct spi_message msg;
    u8 *rbuf = cts_dev->rtdata.rbuf;
    u8 *tbuf = cts_dev->rtdata.tbuf;
    u16 crc16_recv;
    u16 crc16_calc;
    int ret;

    ret = cts_tcs_spi_pack(cts_dev, class_id, cmd_id, NULL, len);
    if (ret < 0)
        return ret;

    memset(&xfer, 0, sizeof(struct spi_transfer));
 //   xfer.delay_usecs = 0;
    xfer.speed_hz = cts_dev->pdata->spi_speed * 1000u;
    xfer.tx_buf = tbuf;
    xfer.rx_buf = rbuf;
    xfer.len = len;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer, &msg);

    ret = spi_sync(cts_data->spi_client, &msg);
    if (ret < 0) {
        TPD_INFO("<E> spi_sync xfer failed: %d\n", ret);
        return ret;
    }

    crc16_recv = (rbuf[len - 1] << 8) | rbuf[len - 2];
    crc16_calc = cts_crc16(rbuf, len - 2);
    if (crc16_recv != crc16_calc) {
        TPD_INFO("<E> crc error: recv %04x != %04x calc\n",
            crc16_recv, crc16_calc);
        return -EIO;
    }

    memcpy(buf, rbuf, len);

    return 0;
}

int cts_tcs_tool_xtrans(const struct cts_device *cts_dev,
    u8 *tbuf, size_t tlen, u8 *rbuf, size_t rlen)
{
    memcpy(cts_dev->rtdata.tbuf, tbuf, tlen);
    memcpy(cts_dev->rtdata.rbuf, rbuf, rlen);
    return cts_tcs_spi_xtrans(cts_dev, tlen, rlen);
}


static int cts_tcs_get_fw_ver(const struct cts_device *cts_dev, u16 *fwver)
{
    int ret = -1;
    u8 buf[4];

    ret = cts_tcs_spi_read(cts_dev, 0, 5, buf, sizeof(buf));
    if (!ret) {
        *fwver = buf[0] | (buf[1] << 8);
        return 0;
    }
    return ret;
}

static int cts_tcs_get_lib_ver(const struct cts_device *cts_dev, u16 *libver)
{
    int ret = -1;
    u8 buf[4];

    ret = cts_tcs_spi_read(cts_dev, 0, 5, buf, sizeof(buf));
    if (!ret) {
        *libver = buf[2] | (buf[3] << 8);
        return 0;
    }
    return ret;
}

static int cts_tcs_get_ddi_ver(const struct cts_device *cts_dev, u8 *ddiver)
{
    int ret = -1;
    u8 buf[1];

    ret = cts_tcs_spi_read(cts_dev, 2, 6, buf, sizeof(buf));
    if (!ret) {
        *ddiver = buf[0];
        return 0;
    }
    return ret;
}

static int cts_tcs_get_res_x(const struct cts_device *cts_dev, u16 *res_x)
{
    int ret = -1;
    u8 buf[10];

    ret = cts_tcs_spi_read(cts_dev, 0, 7, buf, sizeof(buf));
    if (!ret) {
        *res_x = buf[0] | (buf[1] << 8);
        return 0;
    }
    return ret;
}

static int cts_tcs_get_res_y(const struct cts_device *cts_dev, u16 *res_y)
{
    int ret = -1;
    u8 buf[10];

    ret = cts_tcs_spi_read(cts_dev, 0, 7, buf, sizeof(buf));
    if (!ret) {
        *res_y = buf[2] | (buf[3] << 8);
        return 0;
    }
    return ret;
}

static int cts_tcs_get_rows(const struct cts_device *cts_dev, u8 *rows)
{
    int ret = -1;
    u8 buf[10];
    ret = cts_tcs_spi_read(cts_dev, 0, 7, buf, sizeof(buf));
    if (!ret) {
        *rows = buf[5];
        return 0;
    }
    return ret;
}

static int cts_tcs_get_cols(const struct cts_device *cts_dev, u8 *cols)
{
    int ret = -1;
    u8 buf[10];
    ret = cts_tcs_spi_read(cts_dev, 0, 7, buf, sizeof(buf));
    if (!ret) {
        *cols = buf[4];
        return 0;
    }
    return ret;
}

static int cts_tcs_get_flip_x(const struct cts_device *cts_dev, bool *flip_x)
{
    int ret = -1;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev, 7, 2, buf, sizeof(buf));
    if (!ret) {
        *flip_x = !!buf[0];
        return 0;
    }
    return ret;
}

static int cts_tcs_get_flip_y(const struct cts_device *cts_dev, bool *flip_y)
{
    int ret = -1;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev, 7, 3, buf, sizeof(buf));
    if (!ret) {
        *flip_y = !!buf[0];
        return 0;
    }
    return ret;
}

static int cts_tcs_get_swap_axes(const struct cts_device *cts_dev, bool *swap_axes)
{
    int ret = -1;
    u8 buf[1];

    ret = cts_tcs_spi_read(cts_dev, 7, 4, buf, sizeof(buf));
    if (!ret) {
        *swap_axes = !!buf[0];
        return 0;
    }
    return ret;
}

static int cts_tcs_get_int_mode(const struct cts_device *cts_dev, u8 *int_mode)
{
    int ret = -1;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev, 2, 35, buf, sizeof(buf));
    if (!ret) {
        *int_mode = buf[0];
        return 0;
    }
    return ret;
}

static int cts_tcs_get_int_keep_time(const struct cts_device *cts_dev,
    u16 *int_keep_time)
{
    int ret = -1;
    u8 buf[2];
    ret = cts_tcs_spi_read(cts_dev, 2, 36, buf, sizeof(buf));
    if (!ret) {
        *int_keep_time = (buf[0] | (buf[1] << 8));
        return 0;
    }
    return ret;

}

static int cts_tcs_get_rawdata_target(const struct cts_device *cts_dev,
    u16 *rawdata_target)
{
    int ret = -1;
    u8 buf[2];
    ret = cts_tcs_spi_read(cts_dev, 6, 2, buf, sizeof(buf));
    if (!ret) {
        *rawdata_target = (buf[0] | (buf[1] << 8));
        return 0;
    }
    return ret;

}
/*
int cts_tcs_get_esd_protection(const struct cts_device *cts_dev, u8 *esd_method)
{
    int ret = -1;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev, 5, 2, buf, sizeof(buf));
    if (!ret) {
        *esd_method = buf[0];
        return 0;
    }
    return ret;
}
*/
int cts_tcs_get_esd_protection(const struct cts_device *cts_dev,
    u8 *protection)
{
    int ret = -1;
    u8 buf[4];

    buf[0] = 0x01;
    buf[1] = 0x56;
    buf[2] = 0x81;
    buf[3] = 0x00;
    ret = cts_tcs_spi_write(cts_dev, 1, 1, buf, sizeof(buf));
    if (ret != 0)
        return ret;

    ret = cts_tcs_spi_read(cts_dev, 1, 1, protection, sizeof(u8));
    if (!ret)
        return 0;

    return ret;
}


static int cts_tcs_clr_gstr_ready_flag(const struct cts_device *cts_dev)
{
    int ret;
    u8 ready = 0;
    ret = cts_tcs_spi_write(cts_dev, 3, 30, &ready, sizeof(ready));
    return ret;
}

static int cts_tcs_get_gestureinfo(const struct cts_device *cts_dev,
        void *gesture_info)
{
    int ret;
    size_t size = sizeof(*gesture_info) + TAIL_SIZE;

    ret = cts_tcs_spi_get_touch_data(cts_dev, 1, 3,
        cts_dev->rtdata.int_data, size);

    dump_spi(cts_dev, HEADER_SIZE, size);

    if (cts_tcs_clr_gstr_ready_flag(cts_dev)) {
        TPD_INFO("<E> Clear gesture ready flag failed\n");
    }

    if (ret < 0) {
        TPD_INFO("<E> Get gesture info failed: ret=%d\n", ret);
        return ret;
    }
    if (!ret) {
        memcpy(gesture_info, cts_dev->rtdata.int_data, sizeof(*gesture_info));
    }

    return ret;
}

static int cts_tcs_get_touchinfo(const struct cts_device *cts_dev,
    struct cts_device_touch_info *touch_info)
{
    int ret = -1;
    size_t size = cts_dev->fwdata.int_data_size;
    u8 method = cts_dev->fwdata.int_data_method;

    if (!size)
        size = TOUCH_INFO_SIZ;

    memset(touch_info, 0, sizeof(*touch_info));

    ret = cts_tcs_spi_get_touch_data(cts_dev, 1, 3,
        cts_dev->rtdata.int_data, size);

    dump_spi(cts_dev, HEADER_SIZE, size);

    if (ret) {
        TPD_INFO("<E> tcs_spi_read_1_cs failed\n");
        return ret;
    }

    memcpy(touch_info, cts_dev->rtdata.int_data, sizeof(*touch_info));

    if (method == INT_DATA_METHOD_HOST) {
        udelay(200);
    }

    return ret;
}


static int cts_tcs_get_fwid(const struct cts_device *cts_dev, u16 *fwid)
{
    int ret = -1;
    u8 buf[4];

    ret = cts_tcs_spi_read(cts_dev, 0, 3, buf, sizeof(buf));
    if (!ret) {
        *fwid = buf[0] | (buf[1] << 8);
        return 0;
    }

    return ret;
}

static int cts_tcs_get_workmode(const struct cts_device *cts_dev, u8 *workmode)
{
    int ret = -1;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev, 2, 51, buf, sizeof(buf));
    if (!ret) {
        *workmode = buf[0];
        return 0;
    }

    return ret;
}

static int cts_tcs_set_workmode(const struct cts_device *cts_dev, u8 workmode)
{
    int ret = -1;
    ret = cts_tcs_spi_write(cts_dev, 2, 1, &workmode, sizeof(workmode));
    if (!ret)
        return 0;

    return ret;
}

int cts_tcs_set_esd_enable(const struct cts_device *cts_dev, bool enable)
{
    int ret = -1;
    u8 buf = enable ? 1 : 0;
    ret = cts_tcs_spi_write(cts_dev, 5, 1, &buf, sizeof(buf));
    if (!ret)
        return 0;

    return ret;
}

static int cts_tcs_set_cneg_enable(const struct cts_device *cts_dev, bool enable)
{
    int ret = -1;
    u8 buf = enable ? 1 : 0;
    ret = cts_tcs_spi_write(cts_dev, 6, 1, &buf, sizeof(buf));
    if (!ret)
        return 0;

    return ret;
}

static int cts_tcs_set_mnt_enable(const struct cts_device *cts_dev, bool enable)
{
    int ret = -1;
    u8 buf = enable ? 1 : 0;
    ret = cts_tcs_spi_write(cts_dev, 4, 1, &buf, sizeof(buf));
    if (!ret)
        return 0;

    return ret;
}

static int cts_tcs_get_pwr_mode(const struct cts_device *cts_dev, u8 *pwr_mode)
{
    int ret;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev, 2, 4, buf, sizeof(buf));
    if (!ret) {
        *pwr_mode = buf[0];
        return 0;
    }
    return ret;
}

static int cts_tcs_set_pwr_mode(const struct cts_device *cts_dev, u8 pwr_mode)
{
    int ret;
    u8 buf[1];

    buf[0] = pwr_mode;

    ret = cts_tcs_spi_write(cts_dev, 2, 4, buf, sizeof(buf));
    return ret;
}

static int cts_tcs_set_black_test_pwr_mode(const struct cts_device *cts_dev, u8 pwr_mode)
{
    int ret;
    u8 buf[1];

    buf[0] = pwr_mode;

    ret = cts_tcs_spi_write(cts_dev, 3, 43, buf, sizeof(buf));
    return ret;
}


static int cts_tcs_set_product_en(const struct cts_device *cts_dev, u8 enable)
{
    int ret;
    u8 buf[1];

    buf[0] = enable;

    ret = cts_tcs_spi_write(cts_dev, 2, 82, buf, sizeof(buf));
    return ret;
}

static int cts_tcs_set_gesture_mode(const struct cts_device *cts_dev, u8 mode)
{
    int ret;
    u8 buf[1];

    buf[0] = mode;

    ret = cts_tcs_spi_write(cts_dev, 3, 32, buf, sizeof(buf));
    return ret;
}


static int cts_tcs_init_int_data(struct cts_device *cts_dev)
{
    if (!cts_dev->rtdata.int_data) {
        cts_dev->rtdata.int_data = kmalloc(INT_DATA_MAX_SIZ, GFP_KERNEL);
        if (!cts_dev->rtdata.int_data) {
            TPD_INFO("<E> Malloc for int_data failed\n");
            return -ENOMEM;
        }
        return 0;
    }

    return 0;
}

static int cts_tcs_get_has_int_data(const struct cts_device *cts_dev, bool *has)
{
    int ret = -1;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev,  2, 63, buf, sizeof(buf));
    if (!ret) {
        *has = !!buf[0];
        return 0;
    }
    return ret;
}

static int cts_tcs_get_int_data_types(const struct cts_device *cts_dev, u16 *types)
{
    int ret = -1;
    u8 buf[2];
    ret = cts_tcs_spi_read(cts_dev, 2, 65, buf, sizeof(buf));
    if (!ret) {
        *types = buf[0] | (buf[1] << 8);
        return 0;
    }
    return ret;
}

static int cts_tcs_set_int_data_types(const struct cts_device *cts_dev, u16 types)
{
    int ret = -1;
    ret = cts_tcs_spi_write(cts_dev, 2, 65,
        (u8 *) &types, sizeof(types));
    if (!ret)
        return 0;

    return ret;
}

static int cts_tcs_get_int_data_method(const struct cts_device *cts_dev, u8 *method)
{
    int ret = -1;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev, 2, 64, buf, sizeof(buf));
    if (!ret) {
        *method = buf[0];
        return 0;
    }
    return ret;
}

static int cts_tcs_set_int_data_method(const struct cts_device *cts_dev, u8 method)
{
    int ret = -1;
    ret = cts_tcs_spi_write(cts_dev, 2, 64,
        &method, sizeof(method));

    if (!ret)
        return 0;

    return ret;
}

static int cts_tcs_calc_int_data_size(struct cts_device *cts_dev)
{
#define INT_DATA_TYPE_U8_SIZ        \
    (cts_dev->hwdata->num_row * cts_dev->hwdata->num_col * sizeof(u8))
#define INT_DATA_TYPE_U16_SIZ        \
    (cts_dev->hwdata->num_row * cts_dev->hwdata->num_col * sizeof(u16))

    int data_size = TOUCH_INFO_SIZ + TAIL_SIZE;
    u16 data_types = cts_dev->fwdata.int_data_types;
    u8 data_method = cts_dev->fwdata.int_data_method;

    if (data_method == INT_DATA_METHOD_NONE) {
        cts_dev->fwdata.int_data_size = data_size;
        return 0;
    } else if (data_method == INT_DATA_METHOD_DEBUG) {
        data_size += INT_DATA_INFO_SIZ;
        data_size += (INT_DATA_TYPE_LEN_SIZ + INT_DATA_TYPE_U16_SIZ);
        cts_dev->fwdata.int_data_size = data_size;
        return 0;
    }

    TPD_INFO("<I> data_method:%d, data_type:%d\n", data_method, data_types);
    if (data_types != INT_DATA_TYPE_NONE) {
        data_size += INT_DATA_INFO_SIZ;
        if ((data_types & INT_DATA_TYPE_RAWDATA)) {
            data_size += (INT_DATA_TYPE_LEN_SIZ + INT_DATA_TYPE_U16_SIZ);
        }
        if ((data_types & INT_DATA_TYPE_MANUAL_DIFF)) {
            data_size += (INT_DATA_TYPE_LEN_SIZ + INT_DATA_TYPE_U16_SIZ);
        }
        if ((data_types & INT_DATA_TYPE_REAL_DIFF)) {
            data_size += (INT_DATA_TYPE_LEN_SIZ + INT_DATA_TYPE_U16_SIZ);
        }
        if ((data_types & INT_DATA_TYPE_NOISE_DIFF)) {
            data_size += (INT_DATA_TYPE_LEN_SIZ + INT_DATA_TYPE_U16_SIZ);
        }
        if ((data_types & INT_DATA_TYPE_BASEDATA)) {
            data_size += (INT_DATA_TYPE_LEN_SIZ + INT_DATA_TYPE_U16_SIZ);
        }
        if ((data_types & INT_DATA_TYPE_CNEGDATA)) {
            data_size += (INT_DATA_TYPE_LEN_SIZ + INT_DATA_TYPE_U8_SIZ);
        }
    }

    TPD_INFO("<I> data_size: %d\n", data_size);
    cts_dev->fwdata.int_data_size = data_size;
    return 0;
}


int cts_set_int_data_types(struct cts_device *cts_dev, u16 types)
{
    int ret = 0;
    u16 realtypes = types & INT_DATA_TYPE_MASK;

    TPD_INFO("<I> Set int data types: %#06x, mask to %#06x\n", types, realtypes);

    cts_tcs_set_int_data_types(cts_dev, realtypes);
    if (ret) {
        TPD_INFO("<E> Set int data type failed: %d\n", ret);
        return -EIO;
    }
    cts_dev->fwdata.int_data_types = realtypes;
    ret = 0;

    cts_tcs_calc_int_data_size(cts_dev);

    return ret;
}

int cts_set_int_data_method(struct cts_device *cts_dev, u8 method)
{
    int ret = 0;

    TPD_INFO("<I> Set int data method: %d\n", method);

    if (method >= INT_DATA_METHOD_CNT) {
        TPD_INFO("<E> Invalid int data method\n");
        return -EINVAL;
    }

    ret = cts_tcs_set_int_data_method(cts_dev, method);
    if (ret) {
        TPD_INFO("<E> Set int data method failed: %d\n", ret);
        return -EIO;
    }
    cts_dev->fwdata.int_data_method = method;
    ret = 0;

    cts_tcs_calc_int_data_size(cts_dev);

    return ret;
}


static int cts_tcs_read_hw_reg(const struct cts_device *cts_dev, u32 addr,
        u8 *regbuf, size_t size)
{
    int ret;
    u8 buf[4];

    buf[0] = 1;
    buf[1] = ((addr >> 0) & 0xFF);
    buf[2] = ((addr >> 8) & 0xFF);
    buf[3] = ((addr >> 16) & 0xFF);

    ret = cts_tcs_spi_write(cts_dev, 1, 1, buf, sizeof(buf));
    if (ret != 0)
        return ret;

    ret = cts_tcs_spi_read(cts_dev, 1, 2, regbuf, size);
    if (ret != 0)
        return ret;

    return 0;
}

static int cts_tcs_write_hw_reg(const struct cts_device *cts_dev, u32 addr,
        u8 *regbuf, size_t size)
{
    int ret;
    u8 *buf;

    buf = kmalloc(size + 6, GFP_KERNEL);
    if (buf == NULL)
        return -ENOMEM;

    buf[0] = ((size >> 0) & 0xFF);
    buf[1] = ((size >> 8) & 0xFF);
    buf[2] = ((addr >> 0) & 0xFF);
    buf[3] = ((addr >> 8) & 0xFF);
    buf[4] = ((addr >> 16) & 0xFF);
    buf[5] = 0x00;
    memcpy(buf + 6, regbuf, size);

    ret = cts_tcs_spi_write(cts_dev, 1, 20, buf, size + 6);
    if (ret != 0) {
        kfree(buf);
        return ret;
    }

    kfree(buf);

    return ret;
}


static int cts_tcs_get_data_ready_flag(const struct cts_device *cts_dev, u8 *ready)
{
    int ret = -1;
    u8 buf[1];

    ret = cts_tcs_spi_read(cts_dev, 2, 3, buf, sizeof(buf));
    if (!ret) {
        *ready = buf[0];
        return 0;
    }
    return ret;
}
static int cts_tcs_clr_data_ready_flag(const struct cts_device *cts_dev)
{
    int ret;
    u8 ready = 0;
    ret = cts_tcs_spi_write(cts_dev, 2, 3, &ready, sizeof(ready));
    return ret;
}
static int cts_tcs_polling_data(const struct cts_device *cts_dev,
    u8 *buf, size_t size)
{
    int ret = -1;
    int retries = 100;
    u8 ready = 0;
    int offset = TOUCH_INFO_SIZ + INT_DATA_INFO_SIZ + INT_DATA_TYPE_LEN_SIZ;
    size_t data_size = cts_dev->fwdata.int_data_size;

    if (!data_size)
        data_size = TOUCH_INFO_SIZ + TAIL_SIZE;

    do {
        ret = cts_tcs_get_data_ready_flag(cts_dev, &ready);
        if (!ret && ready)
            break;
        mdelay(10);
    } while (!ready && --retries);

    TPD_DEBUG("<I> get data rdy, retries left %d\n", retries);

    if (!ready) {
        TPD_INFO("<E> time out wait for data rdy\n");
        return -EIO;
    }

    mdelay(2);

    retries = 5;
    do {
        ret = cts_tcs_spi_get_touch_data(cts_dev, 1, 35,
            cts_dev->rtdata.int_data, data_size);
        if (ret) {
            TPD_INFO("<E> get touch data failed\n");
            mdelay(1);
            continue;
        }

        if (cts_tcs_clr_data_ready_flag(cts_dev)) {
            TPD_INFO("<E> Clear data ready flag failed\n");
        }

        if (cts_dev->rtdata.int_data[TOUCH_INFO_SIZ + INT_DATA_VALID_SIZ]) {
            memcpy(buf, cts_dev->rtdata.int_data + offset, size);
            break;
        }
    } while (--retries);

    return ret;
}

static int cts_tcs_get_data(struct cts_device *cts_dev, u8 *buf, size_t size,
    enum int_data_type type)
{
    u8 old_int_data_method;
    u16 old_int_data_types;

    old_int_data_types = cts_dev->fwdata.int_data_types;
    old_int_data_method = cts_dev->fwdata.int_data_method;

    cts_set_int_data_types(cts_dev, type);
    cts_set_int_data_method(cts_dev, INT_DATA_METHOD_POLLING);

    cts_tcs_polling_data(cts_dev, buf, size);

    cts_set_int_data_method(cts_dev, old_int_data_method);
    cts_set_int_data_types(cts_dev, old_int_data_types);

    return 0;
}

static int cts_tcs_get_rawdata(struct cts_device *cts_dev, u8 *buf, size_t size)
{
    return cts_tcs_get_data(cts_dev, buf, size, INT_DATA_TYPE_RAWDATA);
}

static int cts_tcs_get_manual_diff(struct cts_device *cts_dev, u8 *buf, size_t size)
{
    return cts_tcs_get_data(cts_dev, buf, size, INT_DATA_TYPE_MANUAL_DIFF);
}

static int cts_tcs_get_real_diff(struct cts_device *cts_dev, u8 *buf, size_t size)
{
    return cts_tcs_get_data(cts_dev, buf, size, INT_DATA_TYPE_REAL_DIFF);
}

static int cts_tcs_get_cnegdata(struct cts_device *cts_dev, u8 *buf, size_t size)
{
    return cts_tcs_get_data(cts_dev, buf, size, INT_DATA_TYPE_CNEGDATA);
}

static int cts_tcs_set_charger_plug(const struct cts_device *cts_dev, bool set)
{
    int ret;
    u8 buf[1];

    buf[0] = set ? 1 : 0;

    ret = cts_tcs_spi_write(cts_dev, 2, 5, buf, sizeof(buf));
    if (!ret) {
        return 0;
    }

    return ret;
}

static int cts_tcs_set_earjack_plug(const struct cts_device *cts_dev, bool set)
{
    int ret;
    u8 buf[1];

    buf[0] = set ? 1 : 0;

    ret = cts_tcs_spi_write(cts_dev, 2, 19, buf, sizeof(buf));
    if (!ret) {
        return 0;
    }

    return ret;
}

static int cts_tcs_set_panel_direction(const struct cts_device *cts_dev,
    u8 direction)
{
    int ret;
    u8 buf[1] = { direction };
    ret = cts_tcs_spi_write(cts_dev, 2, 66, buf, sizeof(buf));
    if (!ret) {
        return 0;
    }

    return ret;
}

static int cts_tcs_set_game_mode(const struct cts_device *cts_dev, u8 enable)
{
    int ret;
    u8 buf[1] = { enable };
    ret = cts_tcs_spi_write(cts_dev, 2, 78, buf, sizeof(buf));
    if (!ret) {
        return 0;
    }

    return ret;
}

static int cts_tcs_get_water_flag(const struct cts_device *cts_dev, u8* cmd)
{
	int ret = -1;
    u8 buf[5];

    ret = cts_tcs_spi_read(cts_dev, 2, 88, buf, sizeof(buf));
    if (!ret) {
        *cmd = buf[0];
        return 0;
    }
    return ret;
}
/*diaphragm ckliu*/
static int cts_tcs_set_diaphragm_lv_set(const struct cts_device *cts_dev, int cmd)
{
	int ret;
    u8 buf[1] = { cmd };
    ret = cts_tcs_spi_write(cts_dev, 2, 84, buf, sizeof(buf));
    if (!ret) {
        return 0;
    }
    return ret;
}

/*smooth ckliu*/
static int cts_tcs_set_smooth_lv_set(const struct cts_device *cts_dev, int cmd)
{
	int ret;
    u8 buf[1] = { cmd };
    ret = cts_tcs_spi_write(cts_dev, 2, 85, buf, sizeof(buf));
    if (!ret) {
        return 0;
    }
    return ret;

}

/*sensitive ckliu*/
static int cts_tcs_set_sensitive_lv_set(const struct cts_device *cts_dev, int cmd)
{
	int ret;
    u8 buf[1] = { cmd };
    ret = cts_tcs_spi_write(cts_dev, 2, 86, buf, sizeof(buf));
    if (!ret) {
        return 0;
    }
    return ret;

}

/*ctrl rate*/
static int cts_tcs_set_panel_rate(const struct cts_device *cts_dev, int cmd)
{

	int ret;
    u8 buf[1] = { cmd };
    ret = cts_tcs_spi_write(cts_dev, 2, 87, buf, sizeof(buf));
    if (!ret) {
        return 0;
    }
    return ret;
}

static int cts_tcs_set_int_test(const struct cts_device *cts_dev, u8 enable)
{
    int ret;
    ret = cts_tcs_spi_write(cts_dev, 2, 23, &enable, sizeof(enable));
    if (!ret) {
        return 0;
    }

    return -1;
}

static int cts_tcs_set_int_pin(const struct cts_device *cts_dev, u8 high)
{
    int ret;
    ret = cts_tcs_spi_write(cts_dev, 2, 24, &high, sizeof(high));
    if (!ret) {
        return 0;
    }

    return -1;
}

static int set_display_state(struct cts_device *cts_dev, bool active)
{
    int ret;
    u8  access_flag;

    TPD_INFO("<I> Set display state to %s\n", active ? "ACTIVE" : "SLEEP");

    ret = cts_hw_reg_readb(cts_dev, 0x3002C, &access_flag);
    if (ret) {
        TPD_INFO("<E> Read display access flag failed %d\n", ret);
        return ret;
    }

    ret = cts_hw_reg_writeb(cts_dev, 0x3002C, access_flag | 0x01);
    if (ret) {
        TPD_INFO("<E> Write display access flag %02x failed %d\n",
            access_flag, ret);
        return ret;
    }

    if (active) {
        ret = cts_hw_reg_writeb(cts_dev, 0x3C044, 0x55);
        if (ret) {
            TPD_INFO("<E> Write DCS-CMD11 fail\n");
            return ret;
        }

        msleep(100);

        ret = cts_hw_reg_writeb(cts_dev, 0x3C0A4, 0x55);
        if (ret) {
            TPD_INFO("<E> Write DCS-CMD29 fail\n");
            return ret;
        }

        msleep(100);
    } else {
        ret = cts_hw_reg_writeb(cts_dev, 0x3C0A0, 0x55);
        if (ret) {
            TPD_INFO("<E> Write DCS-CMD28 fail\n");
            return ret;
        }

        msleep(100);

        ret = cts_hw_reg_writeb(cts_dev, 0x3C040, 0x55);
        if (ret) {
            TPD_INFO("<E> Write DCS-CMD10 fail\n");
            return ret;
        }

        msleep(100);
    }

    ret = cts_hw_reg_writeb(cts_dev, 0x3002C, access_flag);
    if (ret) {
        TPD_INFO("<E> Restore display access flag %02x failed %d\n", access_flag, ret);
        return ret;
    }

    return 0;
}


static int cts_tcs_set_openshort_mode(const struct cts_device *cts_dev, u8 mode)
{
    int ret = -1;
    ret = cts_tcs_spi_write(cts_dev, 11, 2, &mode, sizeof(mode));
    if (!ret)
        return 0;

    return ret;
}

static int cts_tcs_set_short_test_type(const struct cts_device *cts_dev,
    u8 short_type)
{
    int ret = -1;
    ret = cts_tcs_spi_write(cts_dev, 11, 3, &short_type, sizeof(short_type));
    if (!ret)
        return 0;

    return ret;
}

static int cts_tcs_is_display_on(const struct cts_device *cts_dev, u8 *display_on)
{
    int ret;
    u8 buf[1];
    ret = cts_tcs_spi_read(cts_dev, 11, 4, buf, sizeof(buf));
    if (!ret) {
        *display_on = buf[0];
        return 0;
    }

    return ret;
}

static int cts_tcs_set_display_on(const struct cts_device *cts_dev, u8 display_on)
{
    int ret = -1;
    ret = cts_tcs_spi_write(cts_dev, 11, 4, &display_on, sizeof(display_on));
    if (!ret)
        return 0;

    return ret;
}

static int wait_fw_to_normal_work(struct cts_device *cts_dev)
{
    u8 work_mode;
    int i = 0;
    int ret;

    TPD_INFO("<I> Wait fw to normal work\n");

    do {
        mdelay(10);

        ret = cts_tcs_get_workmode(cts_dev, &work_mode);
        if (ret)
            continue;

        if (work_mode == CTS_FIRMWARE_WORK_MODE_NORMAL) {
            TPD_INFO("<I> Wait to normal mode Okay\n");
            return 0;
        }
    } while (++i < 100);

    TPD_INFO("<E> Wait to normal mode failed, retry:%d\n", i);

	return ret ? ret : -ETIMEDOUT;
}

static int wait_fw_to_curr_mode(struct cts_device *cts_dev)
{
    int i = 0;
    int ret;
	u8 work_mode;

    TPD_INFO("<I> Wait fw to curr work mode");

    do {
        ret = cts_tcs_get_workmode(cts_dev, &work_mode);
        if (ret) {
            TPD_INFO("<E> Get fw curr work mode failed %d", work_mode);
            continue;
        } else if (work_mode == CTS_FIRMWARE_WORK_MODE_OPEN_SHORT) {
        	return 0;
        }
        mdelay(10);
    } while (++i < 100);

	TPD_INFO("<E> Get work_mode: %d != %d", work_mode, CTS_FIRMWARE_WORK_MODE_OPEN_SHORT);

	return -ETIMEDOUT;
}

static int cts_output_data(struct cts_device *cts_dev, struct auto_testdata *cts_testdata,
	void *data, char *name)
{
	uint8_t data_buf[64];
	int rows = cts_dev->fwdata.rows;
	int cols = cts_dev->fwdata.cols;
	int nodes = 0;
	int r, c;
	u16 *rawdata = NULL;
	u8 *cap_data = NULL;

	TPD_INFO("<I> %s, pos: %ld, length: %lu\n", name, *cts_testdata->pos,
		cts_testdata->length);

	if (strstr(name, "comp_cap") != NULL) {
		cap_data = (u8 *)data;
	} else if (strstr(name, "lp") != NULL) {
		cols = CTS_TEST_LP_DATA_CHANNEL;
		rawdata = (u16 *)data;
	} else {
		rawdata = (u16 *)data;
	}
	nodes = rows * cols;

	if (!rawdata && !cap_data) {
		TPD_INFO("<I> rawdata = NULL, cap_data = NULL\n");
		return -1;
	}

	TPD_INFO("<I> rows: %d, cols: %d, nodes: %d\n", rows, cols, nodes);

	memset(data_buf, 0, sizeof(data_buf));

	for (r = 0; r < rows; r++) {
		for (c = 0; c < cols; c++) {
			snprintf(data_buf, 64, "%d,", (rawdata == NULL) ?
				cap_data[r*cts_dev->hwdata->num_col + c] : rawdata[r*cts_dev->hwdata->num_col + c]);
			tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf,
				strlen(data_buf), cts_testdata->pos);
		}		
		snprintf(data_buf, 64, "\n");
		tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf,
				  strlen(data_buf), cts_testdata->pos);
	}

	return 0;
}

static int cts_prepare_test(struct cts_device *cts_dev)
{
    //struct cts_interface *cts_if = cts_dev->cts_if;
	int i = 0;
    int ret;
	u8 workmode = 0xFF;

    TPD_INFO("<I> Prepare test\n");

    cts_plat_reset_device(cts_dev->pdata);

    ret = wait_fw_to_normal_work(cts_dev);
    if (ret) {
        TPD_INFO("<E> Wait fw to normal work failed %d\n", ret);
        return ret;
    }

	ret = cts_tcs_set_workmode(cts_dev, CTS_FIRMWARE_WORK_MODE_CFG);
    if (ret) {
        TPD_INFO("<E> Set firmware work mode to WORK_MODE_CFG failed %d\n", ret);
        return ret;
    }
	mdelay(30);
    do {
		ret = cts_tcs_get_workmode(cts_dev, &workmode);
		if (ret) {
			TPD_INFO("<E> Get real workmode failed %d\n", ret);
		} else if (workmode == CTS_FIRMWARE_WORK_MODE_CFG) {
			break;
		}
		mdelay(30);
		TPD_INFO("<I> Get workmode: %d, CTS_FIRMWARE_WORK_MODE_CFG: %d, retry count: %d",
			workmode, CTS_FIRMWARE_WORK_MODE_CFG, i);
    } while (i++ < 10);
	if (workmode != CTS_FIRMWARE_WORK_MODE_CFG)
		return -EINVAL;	

	ret = cts_tcs_set_product_en(cts_dev, 1);
    if (ret) {
        TPD_INFO("<E> Set product en failed %d\n", ret);
        return ret;
    }

	ret = cts_tcs_set_workmode(cts_dev, CTS_FIRMWARE_WORK_MODE_NORMAL);
    if (ret) {
        TPD_INFO("<E> Set fw work mode to WORK_MODE_NORMAL failed %d\n", ret);
        return ret;
    }

    ret = wait_fw_to_normal_work(cts_dev);
    if (ret) {
        TPD_INFO("<E> Wait fw to normal work failed %d\n", ret);
        return ret;
    }

    cts_dev->rtdata.testing = true;

    return 0;
}

static void cts_post_test(struct cts_device *cts_dev)
{
    int ret;

    TPD_INFO("<I> Post test\n");

    cts_plat_reset_device(cts_dev->pdata);

	ret = cts_tcs_set_workmode(cts_dev, CTS_FIRMWARE_WORK_MODE_NORMAL);
    if (ret) {
        TPD_INFO("<E> Set firmware work mode to WORK_MODE_NORMAL failed %d\n", ret);
    }

    ret = wait_fw_to_normal_work(cts_dev);
    if (ret) {
        TPD_INFO("<E> Wait fw to normal work failed %d\n", ret);
    }

    cts_dev->rtdata.testing = false;
}

static int cts_tcs_test_int_pin(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
	struct cts_device *cts_dev = &chip_data->cts_dev;
    int ret;

	TPD_INFO("<I> %s +\n", __func__);
	
    cts_lock_device(cts_dev);
    ret = cts_tcs_set_int_test(cts_dev, 1);
    if (ret) {
        TPD_INFO("<E> Enable Int Test failed\n");
        goto unlock_device;
    }
    ret = cts_tcs_set_int_pin(cts_dev, 1);
    if (ret) {
        TPD_INFO("<E> Enable Int Test High failed\n");
        goto exit_int_test;
    }
    mdelay(10);
    if (cts_plat_get_int_pin(cts_dev->pdata) == 0) {
        TPD_INFO("<E> INT pin state != HIGH\n");
        ret = -EFAULT;
        goto exit_int_test;
    }

    ret = cts_tcs_set_int_pin(cts_dev, 0);
    if (ret) {
        TPD_INFO("<E> Enable Int Test LOW failed\n");
        goto exit_int_test;
    }
    mdelay(10);
    if (cts_plat_get_int_pin(cts_dev->pdata) != 0) {
        TPD_INFO("<E> INT pin state != LOW\n");
        ret = -EFAULT;
        goto exit_int_test;
    }

exit_int_test:
    {
        int r = cts_tcs_set_int_test(cts_dev, 0);
        if (r) {
            TPD_INFO("<E> Disable Int Test failed %d\n", r);
        }
    }
    mdelay(10);

unlock_device:
    cts_unlock_device(cts_dev);

	if (ret) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_INT");
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_INT");
	}

	TPD_INFO("<I> %s -\n", __func__);

    return ret;
}

static int cts_tcs_test_reset_pin(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
	struct cts_device *cts_dev = &chip_data->cts_dev;
	
	bool ret = false;
	int result = 0;
	
	TPD_INFO("<I> %s +\n", __func__);

	cts_lock_device(cts_dev);

	cts_plat_set_reset(cts_dev->pdata, 0);
	mdelay(50);

#ifdef CONFIG_CTS_I2C_HOST
	ret = cts_plat_is_i2c_online(cts_dev->pdata, CTS_DEV_NORMAL_MODE_I2CADDR);
#else
	ret = cts_plat_is_normal_mode(cts_dev->pdata);
#endif
	if (ret) {
		result = -EIO;
		TPD_INFO("<E> Device is alive while reset is low\n");
	}

	cts_plat_set_reset(cts_dev->pdata, 1);
	mdelay(50);

	wait_fw_to_normal_work(cts_dev);

#ifdef CONFIG_CTS_I2C_HOST
	ret = cts_plat_is_i2c_online(cts_dev->pdata, CTS_DEV_NORMAL_MODE_I2CADDR);
#else
	ret = cts_plat_is_normal_mode(cts_dev->pdata);
#endif
	if (!ret) {
		result = -EIO;
		TPD_INFO("<E> Device is offline while reset is high\n");
	}

	cts_unlock_device(cts_dev);

	if (result) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_RESET");
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_RESET");
	}

	TPD_INFO("<I> %s -\n", __func__);
	
	return result;
}

static int cts_tcs_test_compensate_cap(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
#define SPLIT_LINE_STR \
		"------------------------------"

	struct cts_device *cts_dev = &chip_data->cts_dev;

	int result = 0;
	int ret = -1;
	int i;
	int r, c;
	int failed_cnt = 0;
	int num_nodes;
	int offset = 0;
	uint8_t data_buf[64];
	u8 *cap = NULL;
	uint16_t item_limit_type = 0xFFFF;

	TPD_INFO("<I> %s +\n", __func__);

	item_limit_type = chip_data->p_cts_test_para->limit_type_comp_cap;
	
	num_nodes = cts_dev->hwdata->num_row * cts_dev->hwdata->num_row;

	cap = (u8 *)kzalloc(num_nodes, GFP_KERNEL);
	if (cap == NULL) {
		TPD_INFO("<E> Allocate memory for cap failed\n");
		ret = -ENOMEM;
		goto show_test_result;
	}

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "[CTS COMP CAP]");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);

	cts_lock_device(cts_dev);
	ret = cts_prepare_test(cts_dev);
	if (ret) {
		TPD_INFO("<I> Prepare test failed %d", ret);
		goto err_unlock;
	}

	for (i = 0; i < 3; i++) {
		ret = cts_tcs_get_cnegdata(cts_dev, cap, num_nodes);
		if (ret < 0) {
			TPD_INFO("<E> Get comp cap failed: %d\n", ret);
			mdelay(30);
		} else {
			break;
		}
	}
	if (ret) {
		goto err_unlock;
	}

	cts_output_data(cts_dev, cts_testdata, cap, "comp_cap");

	TPD_INFO("<I> item limit type: %d\n", item_limit_type);
	if (item_limit_type) {
		if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
			TPD_INFO("<I> [%s] max: %d, min: %d", "Comp cap",
				*chip_data->p_cts_autotest_offset->cts_comp_cap_max,
				*chip_data->p_cts_autotest_offset->cts_comp_cap_min);
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;

					if ((cap[offset] < *chip_data->p_cts_autotest_offset->cts_comp_cap_min) &&
						(cap[offset] > *chip_data->p_cts_autotest_offset->cts_comp_cap_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Comp cap");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, cap[offset]);
					}
				}
			}
		} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;
			
					if ((cap[offset] < chip_data->p_cts_autotest_offset->cts_comp_cap_min[offset])
						&& (cap[offset] > chip_data->p_cts_autotest_offset->cts_comp_cap_max[offset])) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Comp cap");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, cap[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
		}
	} else {
		if ((chip_data->p_cts_test_para->test_open_max != 0) &&
			(chip_data->p_cts_test_para->test_open_min != 0)) {
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;

					if ((cap[offset] < chip_data->p_cts_test_para->test_comp_cap_min) &&
						(cap[offset] > chip_data->p_cts_test_para->test_comp_cap_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Comp cap");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, cap[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
		}
	}

err_unlock:
	cts_post_test(cts_dev);
	cts_unlock_device(cts_dev);

	if (cap) {
		kfree(cap);
	}

show_test_result:
	if (ret || failed_cnt) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_COMP_CAP");
		result = -1;
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_COMP_CAP");
	}

	TPD_INFO("<I> %s -\n", __func__);

	return result;
}

#define SHORT_COLS_TEST_LOOP            3
#define SHORT_ROWS_TEST_LOOP            6
/* Return 0 success
    negative value while error occurs
    positive value means how many nodes fail */
static int cts_tcs_test_short(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
#define SPLIT_LINE_STR \
		"------------------------------"

	struct cts_device *cts_dev = &chip_data->cts_dev;

	int result = 0;
	int ret = -1;
	int i;
	int r, c;
    int loopcnt;
	int failed_cnt = 0;
	int offset = 0;
	int num_nodes, tsdata_frame_size;
	uint8_t data_buf[64];
	u16 *test_result = NULL;
	u8 need_display_on;
	bool recovery_display_state = false;
	uint16_t item_limit_type = 0xFFFF;

	TPD_INFO("<I> %s +\n", __func__);

	item_limit_type = chip_data->p_cts_test_para->limit_type_short;
	
	num_nodes = cts_dev->hwdata->num_row * cts_dev->hwdata->num_row;
	tsdata_frame_size = 2 * num_nodes;

	test_result = (u16 *)kmalloc(tsdata_frame_size, GFP_KERNEL);
	if (test_result == NULL) {
		TPD_INFO("<E> Allocate memory for rawdata failed\n");
		ret = -ENOMEM;
		goto show_test_result;
	}

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "[CTS SHORTDATA]");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);

	cts_lock_device(cts_dev);
	ret = cts_prepare_test(cts_dev);
	if (ret) {
		TPD_INFO("<I> Prepare test failed %d", ret);
		goto err_unlock;
	}

	ret = cts_tcs_is_display_on(cts_dev, &need_display_on);
	if (ret) {
		TPD_INFO("<E> Read need display on register failed %d\n", ret);
		goto err_unlock;
	}

	if (need_display_on == 0) {
		ret = cts_tcs_set_display_on(cts_dev, 0x00);
		if (ret) {
			TPD_INFO("<E> Set display state to SLEEP failed %d\n", ret);
			goto err_unlock;
		}
		recovery_display_state = true;
	}

    TPD_INFO("<I> Test short to GND");
	ret = cts_tcs_set_short_test_type(cts_dev, CTS_SHORT_TEST_UNDEFINED);
	if (ret) {
		TPD_INFO("<E> Set short test type failed %d\n", ret);
		goto err_recovery_display_state;
	}
    ret = cts_tcs_set_openshort_mode(cts_dev, CTS_TEST_SHORT);
    if (ret) {
        TPD_INFO("<E> Set test type to SHORT failed %d\n", ret);
        goto err_recovery_display_state;
    }

	ret = cts_tcs_set_workmode(cts_dev, CTS_FIRMWARE_WORK_MODE_OPEN_SHORT);
	if (ret) {
		TPD_INFO("<E> Set firmware work mode to WORK_MODE_TEST failed %d\n", ret);
		goto err_recovery_display_state;
	}

	ret = wait_fw_to_curr_mode(cts_dev);
	if (ret) {
		TPD_INFO("<E> Set firmware work mode to WORK_MODE_TEST failed %d\n", ret);
		goto err_recovery_display_state;
	}
	
	cts_set_int_data_types(cts_dev, INT_DATA_TYPE_RAWDATA);
	cts_set_int_data_method(cts_dev, INT_DATA_METHOD_POLLING);

	ret = cts_tcs_set_short_test_type(cts_dev, CTS_SHORT_TEST_BETWEEN_GND);
	if (ret) {
		TPD_INFO("<E> Set short test type to SHORT_TO_GND failed %d\n", ret);
		goto err_recovery_display_state;
	}

	for (i = 0; i < 3; i++) {
		ret = cts_tcs_polling_data(cts_dev, (u8 *)test_result, RAWDATA_BUFFER_SIZE(cts_dev));
		if (ret < 0) {
			TPD_INFO("<E> Get short data failed: %d\n", ret);
			mdelay(30);
		} else {
			break;
		}
	}
	if (ret) {
		goto err_recovery_display_state;
	}

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "short to GND");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
	cts_output_data(cts_dev, cts_testdata, test_result, "shortdata");
	TPD_INFO("<I> item limit type: %d\n", item_limit_type);
	if (item_limit_type) {
		if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
			TPD_INFO("<I> [%s] max: %d, min: %d", "Shortdata",
				*chip_data->p_cts_autotest_offset->cts_short_max,
				*chip_data->p_cts_autotest_offset->cts_short_min);
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;

					if ((test_result[offset] < *chip_data->p_cts_autotest_offset->cts_short_min) &&
						(test_result[offset] > *chip_data->p_cts_autotest_offset->cts_short_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, test_result[offset]);
					}
				}
			}
		} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;
			
					if ((test_result[offset] < chip_data->p_cts_autotest_offset->cts_short_min[offset])
						&& (test_result[offset] > chip_data->p_cts_autotest_offset->cts_short_max[offset])) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, test_result[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
		}
	} else {
		if ((chip_data->p_cts_test_para->test_short_max != 0) &&
			(chip_data->p_cts_test_para->test_short_min != 0)) {
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;

					if ((test_result[offset] < chip_data->p_cts_test_para->test_short_min) &&
						(test_result[offset] > chip_data->p_cts_test_para->test_short_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, test_result[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
		}
	}
	if (failed_cnt)
		goto err_recovery_display_state;

    TPD_INFO("<I> Test short between columns");
	ret = cts_tcs_set_short_test_type(cts_dev, CTS_SHORT_TEST_BETWEEN_COLS);
	if (ret) {
		TPD_INFO("<E> Set short test type to BETWEEN_COLS failed %d\n", ret);
		goto err_recovery_display_state;
	}

    for (loopcnt = 0; loopcnt < SHORT_COLS_TEST_LOOP; loopcnt++) {
		for (i = 0; i < 3; i++) {
			ret = cts_tcs_polling_data(cts_dev, (u8 *)test_result, RAWDATA_BUFFER_SIZE(cts_dev));
			if (ret < 0) {
				TPD_INFO("<E> Get short data failed: %d\n", ret);
				mdelay(30);
			} else {
				break;
			}
		}
		if (ret)
			goto err_recovery_display_state;

		memset(data_buf, 0, sizeof(data_buf));
		snprintf(data_buf, 64, "%s, loopcnt: %d\n", "short between columns", loopcnt+1);
		tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
		cts_output_data(cts_dev, cts_testdata, test_result, "shortdata");
		TPD_INFO("<I> item limit type: %d\n", item_limit_type);
		if (item_limit_type) {
			if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;
		
						if ((test_result[offset] < *chip_data->p_cts_autotest_offset->cts_short_min) &&
							(test_result[offset] > *chip_data->p_cts_autotest_offset->cts_short_max)) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, test_result[offset]);
						}
					}
				}
			} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;
				
						if ((test_result[offset] < chip_data->p_cts_autotest_offset->cts_short_min[offset])
							&& (test_result[offset] > chip_data->p_cts_autotest_offset->cts_short_max[offset])) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, test_result[offset]);
						}
					}
				}
			} else {
				TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
			}
		} else {
			if ((chip_data->p_cts_test_para->test_short_max != 0) &&
				(chip_data->p_cts_test_para->test_short_min != 0)) {
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;
		
						if ((test_result[offset] < chip_data->p_cts_test_para->test_short_min) &&
							(test_result[offset] > chip_data->p_cts_test_para->test_short_max)) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, test_result[offset]);
						}
					}
				}
			} else {
				TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
			}
		}
		if (failed_cnt)
			goto err_recovery_display_state;
	}
	
	TPD_INFO("<I> Test short between rows");
	ret = cts_tcs_set_short_test_type(cts_dev, CTS_SHORT_TEST_BETWEEN_ROWS);
	if (ret) {
		TPD_INFO("<E> Set short test type to BETWEEN_ROWS failed %d\n", ret);
		goto err_recovery_display_state;
	}

	for (loopcnt = 0; loopcnt < SHORT_ROWS_TEST_LOOP; loopcnt++) {
		for (i = 0; i < 3; i++) {
			ret = cts_tcs_polling_data(cts_dev, (u8 *)test_result, RAWDATA_BUFFER_SIZE(cts_dev));
			if (ret < 0) {
				TPD_INFO("<E> Get short data failed: %d\n", ret);
				mdelay(30);
			} else {
				break;
			}
		}
		if (ret)
			goto err_recovery_display_state;

		memset(data_buf, 0, sizeof(data_buf));
		snprintf(data_buf, 64, "%s, loopcnt: %d\n", "short between rows", loopcnt+1);
		tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
		cts_output_data(cts_dev, cts_testdata, test_result, "shortdata");
		TPD_INFO("<I> item limit type: %d\n", item_limit_type);
		if (item_limit_type) {
			if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;
		
						if ((test_result[offset] < *chip_data->p_cts_autotest_offset->cts_short_min) &&
							(test_result[offset] > *chip_data->p_cts_autotest_offset->cts_short_max)) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, test_result[offset]);
						}
					}
				}
			} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;
				
						if ((test_result[offset] < chip_data->p_cts_autotest_offset->cts_short_min[offset])
							&& (test_result[offset] > chip_data->p_cts_autotest_offset->cts_short_max[offset])) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, test_result[offset]);
						}
					}
				}
			} else {
				TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
			}
		} else {
			if ((chip_data->p_cts_test_para->test_short_max != 0) &&
				(chip_data->p_cts_test_para->test_short_min != 0)) {
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;
		
						if ((test_result[offset] < chip_data->p_cts_test_para->test_short_min) &&
							(test_result[offset] > chip_data->p_cts_test_para->test_short_max)) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s failed nodes:\n", "Shortdata");
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, test_result[offset]);
						}
					}
				}
			} else {
				TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
			}
		}
	}

err_recovery_display_state:
    if (recovery_display_state) {
        int r = set_display_state(cts_dev, true);
        if (r) {
            TPD_INFO("<E> Set display state to ACTIVE failed %d\n", r);
        }
    }
	
	cts_set_int_data_method(cts_dev, INT_DATA_METHOD_NONE);
	cts_set_int_data_types(cts_dev, INT_DATA_TYPE_NONE);

err_unlock:
	cts_post_test(cts_dev);
	cts_unlock_device(cts_dev);

	if (test_result) {
		kfree(test_result);
	}

show_test_result:
	if (ret || failed_cnt) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_SHORTDATA");
		result = -1;
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_SHORTDATA");
	}

	TPD_INFO("<I> %s -\n", __func__);

	return result;
}

/* Return 0 success
    negative value while error occurs
    positive value means how many nodes fail */
static int cts_tcs_test_open(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
#define SPLIT_LINE_STR \
		"------------------------------"

	struct cts_device *cts_dev = &chip_data->cts_dev;

	int result = 0;
	int ret = -1;
	int i;
	int r, c;
	int failed_cnt = 0;
	int num_nodes, tsdata_frame_size;
	int offset = 0;
	uint8_t data_buf[64];
	u16 *test_result = NULL;
	u8 need_display_on;
	bool recovery_display_state = false;
	uint16_t item_limit_type = 0xFFFF;

	TPD_INFO("<I> %s +\n", __func__);

	item_limit_type = chip_data->p_cts_test_para->limit_type_open;
	
	num_nodes = cts_dev->hwdata->num_row * cts_dev->hwdata->num_row;
	tsdata_frame_size = 2 * num_nodes;

	test_result = (u16 *)kmalloc(tsdata_frame_size, GFP_KERNEL);
	if (test_result == NULL) {
		TPD_INFO("<E> Allocate memory for rawdata failed\n");
		ret = -ENOMEM;
		goto show_test_result;
	}

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "[CTS OPENDATA]");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);

	cts_lock_device(cts_dev);
	ret = cts_prepare_test(cts_dev);
	if (ret) {
		TPD_INFO("<I> Prepare test failed %d", ret);
		goto err_unlock;
	}

	ret = cts_tcs_is_display_on(cts_dev, &need_display_on);
	if (ret) {
		TPD_INFO("<E> Read need display on register failed %d\n", ret);
		goto err_unlock;
	}

	if (need_display_on == 0) {
		ret = cts_tcs_set_display_on(cts_dev, 0x00);
		if (ret) {
			TPD_INFO("<E> Set display state to SLEEP failed %d\n", ret);
			goto err_unlock;
		}
		recovery_display_state = true;
	}

	ret = cts_tcs_set_openshort_mode(cts_dev, CTS_TEST_OPEN);
	if (ret) {
		TPD_INFO("<E> Set test type to OPEN_TEST failed %d\n", ret);
		goto err_recovery_display_state;
	}

    ret = cts_tcs_set_workmode(cts_dev, CTS_FIRMWARE_WORK_MODE_OPEN_SHORT);
	if (ret) {
		TPD_INFO("<E> Set firmware work mode to WORK_MODE_TEST failed %d\n", ret);
		goto err_recovery_display_state;
	}

    ret = wait_fw_to_curr_mode(cts_dev);
	if (ret) {
		TPD_INFO("<E> Set firmware work mode to WORK_MODE_TEST failed %d\n", ret);
		goto err_recovery_display_state;
	}
	
	cts_set_int_data_types(cts_dev, INT_DATA_TYPE_RAWDATA);
	cts_set_int_data_method(cts_dev, INT_DATA_METHOD_POLLING);

	for (i = 0; i < 3; i++) {
		ret = cts_tcs_polling_data(cts_dev, (u8 *)test_result, RAWDATA_BUFFER_SIZE(cts_dev));
		if (ret < 0) {
			TPD_INFO("<E> Get open data failed: %d\n", ret);
			mdelay(30);
		} else {
			break;
		}
	}
	if (ret) {
		goto err_recovery_display_state;
	}

	cts_output_data(cts_dev, cts_testdata, test_result, "opendata");

	TPD_INFO("<I> item limit type: %d\n", item_limit_type);
	if (item_limit_type) {
		if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
			TPD_INFO("<I> [%s] max: %d, min: %d", "Opendata",
				*chip_data->p_cts_autotest_offset->cts_open_max,
				*chip_data->p_cts_autotest_offset->cts_open_min);
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;

					if ((test_result[offset] < *chip_data->p_cts_autotest_offset->cts_open_min) &&
						(test_result[offset] > *chip_data->p_cts_autotest_offset->cts_open_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Opendata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, test_result[offset]);
					}
				}
			}
		} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;
			
					if ((test_result[offset] < chip_data->p_cts_autotest_offset->cts_open_min[offset])
						&& (test_result[offset] > chip_data->p_cts_autotest_offset->cts_open_max[offset])) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Opendata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, test_result[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
		}
	} else {
		if ((chip_data->p_cts_test_para->test_open_max != 0) &&
			(chip_data->p_cts_test_para->test_open_min != 0)) {
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;

					if ((test_result[offset] < chip_data->p_cts_test_para->test_open_min) &&
						(test_result[offset] > chip_data->p_cts_test_para->test_open_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Opendata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, test_result[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
		}
	}

err_recovery_display_state:
    if (recovery_display_state) {
        int r = cts_tcs_set_display_on(cts_dev, 0x01);
        if (r) {
            TPD_INFO("<E> Set display state to ACTIVE failed %d", r);
        }
    }
	
	cts_set_int_data_method(cts_dev, INT_DATA_METHOD_NONE);
	cts_set_int_data_types(cts_dev, INT_DATA_TYPE_NONE);

err_unlock:
	cts_post_test(cts_dev);
	cts_unlock_device(cts_dev);

	if (test_result) {
		kfree(test_result);
	}

show_test_result:
	if (ret || failed_cnt) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_OPENDATA");
		result = -1;
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_OPENDATA");
	}

	TPD_INFO("<I> %s -\n", __func__);

	return result;
}

static int cts_tcs_test_rawdata(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
#define SPLIT_LINE_STR \
		"------------------------------"

	struct cts_device *cts_dev = &chip_data->cts_dev;

	int result = 0;
	int ret = -1;
	int frame = 0, frame_failed = 0;
	int i;
	int r, c;
	int failed_cnt = 0;
	int num_nodes, tsdata_frame_size;
	int offset = 0;
	uint8_t data_buf[64];
	u16 *rawdata = NULL;
	uint16_t item_limit_type;

	TPD_INFO("<I> %s +\n", __func__);

	item_limit_type = chip_data->p_cts_test_para->limit_type_rawdata;

    if (chip_data->p_cts_test_para->test_rawdata_frames <= 0) {
        TPD_INFO("<I> Rawdata test with too little frame %u\n",
            chip_data->p_cts_test_para->test_rawdata_frames);
        return -EINVAL;
    }
	
	num_nodes = cts_dev->hwdata->num_row * cts_dev->hwdata->num_row;
	tsdata_frame_size = 2 * num_nodes;

	rawdata = (u16 *)kmalloc(tsdata_frame_size, GFP_KERNEL);
	if (rawdata == NULL) {
		TPD_INFO("<E> Allocate memory for rawdata failed\n");
		ret = -ENOMEM;
		goto show_test_result;
	}

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "[CTS RAWDATA]");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);

	cts_lock_device(cts_dev);
	ret = cts_prepare_test(cts_dev);
	if (ret) {
		TPD_INFO("<I> Prepare test failed %d", ret);
		goto err_unlock;
	}
	
	cts_set_int_data_types(cts_dev, INT_DATA_TYPE_RAWDATA);
	cts_set_int_data_method(cts_dev, INT_DATA_METHOD_POLLING);

	for (frame = 0; frame < chip_data->p_cts_test_para->test_rawdata_frames; frame++) {
		bool data_valid = false;
		for (i = 0; i < 3; i++) {
			ret = cts_tcs_polling_data(cts_dev, (u8 *)rawdata, RAWDATA_BUFFER_SIZE(cts_dev));
			if (ret < 0) {
				TPD_INFO("<E> Get raw data failed: %d\n", ret);
				mdelay(30);
			} else {
				data_valid = true;
				break;
			}
		}

		if (!data_valid) {
			ret = -EIO;
			break;
		}

		memset(data_buf, 0, sizeof(data_buf));
		snprintf(data_buf, 64, "[No: %d]\n", frame+1);
		tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
		cts_output_data(cts_dev, cts_testdata, rawdata, "rawdata");
		
		TPD_INFO("<I> item limit type: %d\n", item_limit_type);
		if (item_limit_type) {
			if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
				TPD_INFO("<I> [%s] max: %d, min: %d", "Rawdata",
					*chip_data->p_cts_autotest_offset->cts_rawdata_max,
					*chip_data->p_cts_autotest_offset->cts_rawdata_min);
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;

						if ((rawdata[offset] < *chip_data->p_cts_autotest_offset->cts_rawdata_min) &&
							(rawdata[offset] > *chip_data->p_cts_autotest_offset->cts_rawdata_max)) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s(frame %d) failed nodes:\n", "Rawdata", frame+1);
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, rawdata[offset]);
						}
					}
				}
			} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;
				
						if ((rawdata[offset] < chip_data->p_cts_autotest_offset->cts_rawdata_min[offset])
							&& (rawdata[offset] > chip_data->p_cts_autotest_offset->cts_rawdata_max[offset])) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s(frame %d) failed nodes:\n", "Rawdata", frame+1);
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, rawdata[offset]);
						}
					}
				}
			} else {
				TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
			}
		} else {
			if ((chip_data->p_cts_test_para->test_rawdata_max != 0) &&
				(chip_data->p_cts_test_para->test_rawdata_min != 0)) {
				for (r = 0; r < cts_dev->fwdata.rows; r++) {
					for (c = 0; c < cts_dev->fwdata.cols; c++) {
						offset = r * cts_dev->fwdata.cols + c;

						if ((rawdata[offset] < chip_data->p_cts_test_para->test_rawdata_min) &&
							(rawdata[offset] > chip_data->p_cts_test_para->test_rawdata_max)) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s(frame %d) failed nodes:\n", "Rawdata", frame+1);
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, rawdata[offset]);
						}
					}
				}
			} else {
				TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
			}
		}
		if (failed_cnt) {
			frame_failed++;
			failed_cnt = 0;
		}
	}

	cts_set_int_data_method(cts_dev, INT_DATA_METHOD_NONE);
	cts_set_int_data_types(cts_dev, INT_DATA_TYPE_NONE);

err_unlock:
	cts_post_test(cts_dev);
	cts_unlock_device(cts_dev);

	if (rawdata) {
		kfree(rawdata);
	}

show_test_result:
	if (ret || frame_failed) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_RAWDATA");
		result = -1;
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_RAWDATA");
	}

	TPD_INFO("<I> %s -\n", __func__);

	return result;
}

static int cts_tcs_test_noise(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
#define SPLIT_LINE_STR \
		"------------------------------"

	struct cts_device *cts_dev = &chip_data->cts_dev;

	int result = 0;
	int ret = -1;
	int frame = 0;
	int i;
	int r, c;
	int failed_cnt = 0;
	int num_nodes, tsdata_frame_size;
	int offset = 0;
	uint8_t data_buf[64];
	bool first_frame = true;
	u16 *buffer = NULL;
	int buf_size = 0;
	u16 *noise = NULL;
	u16 *curr_rawdata = NULL;
	u16 *max_rawdata = NULL;
	u16 *min_rawdata = NULL;
	uint16_t item_limit_type = 0xFFFF;

	TPD_INFO("<I> %s +\n", __func__);

	item_limit_type = chip_data->p_cts_test_para->limit_type_noise;

    if (chip_data->p_cts_test_para->test_noise_frames <= 2) {
        TPD_INFO("<I> Noise test with too little frame %u\n",
            chip_data->p_cts_test_para->test_noise_frames);
        return -EINVAL;
    }
	
	num_nodes = cts_dev->hwdata->num_row * cts_dev->hwdata->num_row;
	tsdata_frame_size = 2 * num_nodes;

	buf_size = 4 * tsdata_frame_size;
	buffer = (u16 *)kmalloc(buf_size, GFP_KERNEL);
	if (buffer == NULL) {
		TPD_INFO("<E> Allocate memory for rawdata failed\n");
		ret = -ENOMEM;
		goto show_test_result;
	}

	curr_rawdata	= buffer;
	max_rawdata 	= curr_rawdata + 1 * num_nodes;
	min_rawdata 	= curr_rawdata + 2 * num_nodes;
	noise			= curr_rawdata + 3 * num_nodes;

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "[CTS NOISEDATA]");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);

	cts_lock_device(cts_dev);
	ret = cts_prepare_test(cts_dev);
	if (ret) {
		TPD_INFO("<I> Prepare test failed %d", ret);
		goto err_unlock;
	}
	
	cts_set_int_data_types(cts_dev, INT_DATA_TYPE_RAWDATA);
	cts_set_int_data_method(cts_dev, INT_DATA_METHOD_POLLING);

	for (frame = 0; frame < chip_data->p_cts_test_para->test_noise_frames; frame++) {
		bool data_valid = false;
		for (i = 0; i < 3; i++) {
			ret = cts_tcs_polling_data(cts_dev, (u8 *)curr_rawdata, RAWDATA_BUFFER_SIZE(cts_dev));
			if (ret < 0) {
				TPD_INFO("<E> Get noise data failed: %d\n", ret);
				mdelay(30);
			} else {
				data_valid = true;
				break;
			}
		}

		if (!data_valid) {
			ret = -EIO;
			break;
		}

		memset(data_buf, 0, sizeof(data_buf));
		snprintf(data_buf, 64, "[No: %d]\n", frame+1);
		tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
		cts_output_data(cts_dev, cts_testdata, curr_rawdata, "rawdata");

		if (unlikely(first_frame)) {
			memcpy(max_rawdata, curr_rawdata, tsdata_frame_size);
			memcpy(min_rawdata, curr_rawdata, tsdata_frame_size);
			first_frame = false;
		} else {
			for (i = 0; i < num_nodes; i++) {
				if (curr_rawdata[i] > max_rawdata[i]) {
					max_rawdata[i] = curr_rawdata[i];
				} else if (curr_rawdata[i] < min_rawdata[i]) {
					min_rawdata[i] = curr_rawdata[i];
				}
			}
		}
	}
	for (i = 0; i < num_nodes; i++) {
		noise[i] = max_rawdata[i] - min_rawdata[i];
	}

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "max rawdata");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
	cts_output_data(cts_dev, cts_testdata, max_rawdata, "max rawdata");
	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "min rawdata");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
	cts_output_data(cts_dev, cts_testdata, min_rawdata, "min rawdata");
	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "noisedata");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
	cts_output_data(cts_dev, cts_testdata, noise, "noisedata");

	TPD_INFO("<I> item limit type: %d\n", item_limit_type);
	if (item_limit_type) {
		if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
			TPD_INFO("<I> [%s] max: %d, min: %d", "Noisedata",
				*chip_data->p_cts_autotest_offset->cts_noise_max,
				*chip_data->p_cts_autotest_offset->cts_noise_min);
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;
	
					if ((noise[offset] < *chip_data->p_cts_autotest_offset->cts_noise_min) &&
						(noise[offset] > *chip_data->p_cts_autotest_offset->cts_noise_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Noisedata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, noise[offset]);
					}
				}
			}
		} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;
			
					if ((noise[offset] < chip_data->p_cts_autotest_offset->cts_noise_min[offset])
						&& (noise[offset] > chip_data->p_cts_autotest_offset->cts_noise_max[offset])) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Noisedata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, noise[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
		}
	} else {
		if ((chip_data->p_cts_test_para->test_noise_max != 0) &&
			(chip_data->p_cts_test_para->test_noise_min != 0)) {
			for (r = 0; r < cts_dev->fwdata.rows; r++) {
				for (c = 0; c < cts_dev->fwdata.cols; c++) {
					offset = r * cts_dev->fwdata.cols + c;
	
					if ((noise[offset] < chip_data->p_cts_test_para->test_noise_min) &&
						(noise[offset] > chip_data->p_cts_test_para->test_noise_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", "Noisedata");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, noise[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
		}
	}
	
	cts_set_int_data_method(cts_dev, INT_DATA_METHOD_NONE);
	cts_set_int_data_types(cts_dev, INT_DATA_TYPE_NONE);

err_unlock:
	cts_unlock_device(cts_dev);

	if (buffer) {
		kfree(buffer);
	}

show_test_result:
	if (ret || failed_cnt) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_NOISEDATA");
		result = -1;
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_NOISEDATA");
	}

	TPD_INFO("<I> %s -\n", __func__);

	return result;
}


static int cts_tcs_prepare_black_test(struct cts_device *cts_dev)
{
    int ret;
	u8 buf[1] = {0};

    TPD_INFO("<I> Prepare black test\n");

    //cts_plat_reset_device(cts_dev->pdata);
	cts_lock_device(cts_dev);
/*
    ret = cts_tcs_set_esd_enable(cts_dev, false);
    if (ret) {
        TPD_INFO("<E> Disable firmware ESD protection failed %d\n", ret);
        goto unlock_device;
    }

    ret = cts_tcs_set_mnt_enable(cts_dev, false);
    if (ret) {
        TPD_INFO("<E> Disable firmware monitor mode failed %d\n", ret);
        goto unlock_device;
    }
*/
    ret = cts_tcs_set_pwr_mode(cts_dev, 0x05);
    if (ret) {
        TPD_INFO("<E> Set WORK_MODE_GSTR_DBG failed %d\n", ret);
        goto unlock_device;
    } else {
	    ret = cts_tcs_get_pwr_mode(cts_dev, buf);
		if (buf[0] != 0x05) {
	        TPD_INFO("<E> Get pwr mode: 0x%02x != 0x05 failed %d\n", buf[0], ret);
		}
	}

unlock_device:
	cts_unlock_device(cts_dev);

    return ret;
}

static int cts_tcs_test_gesture_rawdata(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
	struct cts_device *cts_dev = &chip_data->cts_dev;

	int result = 0;
	int ret = -1;
	int frame = 0, frame_failed = 0;
	int i;
	int r, c;
	unsigned int test_frame = 0;
	u8 rows, cols;
    int  idle_mode;
	int failed_cnt = 0;
	int num_nodes, tsdata_frame_size;
	int offset = 0;
	uint8_t data_buf[64];
    u16 *gstr_rawdata = NULL;
	uint16_t item_limit_type = 0xFFFF;

	TPD_INFO("<I> %s +\n", __func__);

    if (chip_data->p_cts_test_para->test_gstr_rawdata_frames <= 0) {
        TPD_INFO("<I> Gesture rawdata test with too little frame %u\n",
            chip_data->p_cts_test_para->test_gstr_rawdata_frames);
        return -EINVAL;
    }

    num_nodes = cts_dev->hwdata->num_row * cts_dev->hwdata->num_col;
    tsdata_frame_size = 2 * num_nodes;

    idle_mode = chip_data->p_cts_test_para->test_work_mode;
	if (idle_mode) {
		rows = cts_dev->fwdata.rows;
		cols = cts_dev->fwdata.cols;
		test_frame = chip_data->p_cts_test_para->test_gstr_rawdata_frames;
		item_limit_type = chip_data->p_cts_test_para->limit_type_gstr_rawdata;
	} else {
		rows = cts_dev->fwdata.rows;
		cols = CTS_TEST_LP_DATA_CHANNEL;
		test_frame = chip_data->p_cts_test_para->test_gstr_lp_rawdata_frames;
		item_limit_type = chip_data->p_cts_test_para->limit_type_gstr_lp_rawdata;
	}

    gstr_rawdata = (u16 *)kmalloc(tsdata_frame_size, GFP_KERNEL);
    if (gstr_rawdata == NULL) {
        TPD_INFO("<E> Allocate memory for gstr rawdata failed\n");
        return -ENOMEM;
    }

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "[CTS GSTR RAWDATA]");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);

    cts_lock_device(cts_dev);
    cts_set_int_data_types(cts_dev, INT_DATA_TYPE_RAWDATA);
    cts_set_int_data_method(cts_dev, INT_DATA_METHOD_POLLING);
    mdelay(50);

    for (i = 0; i < 5; i++) {
        if (cts_tcs_set_gesture_mode(cts_dev, idle_mode))
            TPD_INFO("<E> Set gesture mode: %d failed, retries: %d\n", idle_mode, i);
		else
			break;
    }
	if (i >= 5) {
		TPD_INFO("<E> Set gesture mode failed\n");
		ret = -EIO;
		goto unlock_device;
	}
    mdelay(20);

    /* Skip 10 frames */
    for (i = 0; i < 10; i++) {
        int rc = cts_tcs_polling_data(cts_dev, (u8 *)gstr_rawdata,
                RAWDATA_BUFFER_SIZE(cts_dev));
        if (rc) {
            TPD_INFO("<E> Get gesture rawdata failed %d\n", rc);
            mdelay(30);
        }
    }

    for (frame = 0; frame < test_frame; frame++) {
        bool data_valid = false;

        for (i = 0; i < 3; i++) {
            ret = cts_tcs_polling_data(cts_dev, (u8 *)gstr_rawdata,
                RAWDATA_BUFFER_SIZE(cts_dev));
            if (ret) {
                TPD_INFO("<E> Get gesture rawdata failed %d\n", r);
                mdelay(30);
				data_valid = false;
            } else {
                data_valid = true;
                break;
            }
        }
        if (!data_valid) {
            ret = -EIO;
            break;
        }

		memset(data_buf, 0, sizeof(data_buf));
		snprintf(data_buf, 64, "[No: %d]\n", frame+1);
		tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
		cts_output_data(cts_dev, cts_testdata, gstr_rawdata, idle_mode ? "gstr rawdata" : "gstr lp rawdata");
		
		TPD_INFO("<I> item limit type: %d\n", item_limit_type);
		if (item_limit_type) {
			if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
				if (idle_mode)
					TPD_INFO("<I> [%s] max: %d, min: %d", "Gstr rawdata",
						*chip_data->p_cts_autotest_offset->cts_gstr_rawdata_max,
						*chip_data->p_cts_autotest_offset->cts_gstr_rawdata_min);
				else
					TPD_INFO("<I> [%s] max: %d, min: %d", "Gstr lp rawdata",
						*chip_data->p_cts_autotest_offset->cts_gstr_lp_rawdata_max,
						*chip_data->p_cts_autotest_offset->cts_gstr_lp_rawdata_min);
				for (r = 0; r < rows; r++) {
					for (c = 0; c < cols; c++) {
						offset = r * cols + c;
		
						if ((gstr_rawdata[offset] < *chip_data->p_cts_autotest_offset->cts_gstr_rawdata_min) &&
							(gstr_rawdata[offset] > *chip_data->p_cts_autotest_offset->cts_gstr_rawdata_max)) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s(frame %d) failed nodes:\n", "Gstr Rawdata", frame+1);
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, gstr_rawdata[offset]);
						}
					}
				}
			} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
				for (r = 0; r < rows; r++) {
					for (c = 0; c < cols; c++) {
						offset = r * cols + c;
				
						if ((gstr_rawdata[offset] < chip_data->p_cts_autotest_offset->cts_gstr_rawdata_min[offset])
							&& (gstr_rawdata[offset] > chip_data->p_cts_autotest_offset->cts_gstr_rawdata_max[offset])) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s(frame %d) failed nodes:\n", "Gstr Rawdata", frame+1);
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, gstr_rawdata[offset]);
						}
					}
				}
			} else {
				TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
			}
		} else {
			if ((chip_data->p_cts_test_para->test_gstr_rawdata_max != 0) &&
				(chip_data->p_cts_test_para->test_gstr_rawdata_min != 0)) {
				for (r = 0; r < rows; r++) {
					for (c = 0; c < cols; c++) {
						offset = r * cols + c;
		
						if ((gstr_rawdata[offset] < chip_data->p_cts_test_para->test_gstr_rawdata_min) &&
							(gstr_rawdata[offset] > chip_data->p_cts_test_para->test_gstr_rawdata_max)) {
							if (failed_cnt == 0) {
								TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
								TPD_INFO("<I> %s(frame %d) failed nodes:\n", "Gstr Rawdata", frame+1);
							}
							failed_cnt++;
							
							TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
								failed_cnt, r, c, gstr_rawdata[offset]);
						}
					}
				}
			} else {
				TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
			}
		}
		if (failed_cnt) {
			frame_failed++;
			failed_cnt = 0;
		}
    }

unlock_device:
    cts_set_int_data_method(cts_dev, INT_DATA_METHOD_NONE);
    cts_set_int_data_types(cts_dev, INT_DATA_TYPE_NONE);
    cts_unlock_device(cts_dev);

	if (gstr_rawdata)
		kfree(gstr_rawdata);

	if (ret || frame_failed) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_GSTR_RAWDATA");
		result = -1;
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_GSTR_RAWDATA");
	}

	TPD_INFO("<I> %s -\n", __func__);

    return result;
}

static int cts_tcs_test_gesture_noise(struct chipone_ts_data *chip_data,
		struct auto_testdata *cts_testdata)
{
	struct cts_device *cts_dev = &chip_data->cts_dev;

	int result = 0;
	int ret = -1;
	int frame = 0;
	int i;
	int r, c;
	u8 rows, cols;
    int  idle_mode;
	int failed_cnt = 0;
	int num_nodes, tsdata_frame_size;
	int offset = 0;
	uint8_t data_buf[64];
    u16 *buffer = NULL;
    int buf_size = 0;
	unsigned int test_frame = 0;
    u16 *curr_rawdata = NULL;
    u16 *max_rawdata = NULL;
    u16 *min_rawdata = NULL;
    u16 *gstr_noise = NULL;
    bool first_frame = true;
	uint16_t item_limit_type = 0xFFFF;

	TPD_INFO("<I> %s +\n", __func__);

    if (chip_data->p_cts_test_para->test_gstr_noise_frames <= 2) {
        TPD_INFO("<I> Rawdata test with too little frame %u\n",
            chip_data->p_cts_test_para->test_rawdata_frames);
        return -EINVAL;
    }

    num_nodes = cts_dev->hwdata->num_row * cts_dev->hwdata->num_col;
    tsdata_frame_size = 2 * num_nodes;

    idle_mode = chip_data->p_cts_test_para->test_work_mode;
	if (idle_mode) {
		rows = cts_dev->fwdata.rows;
		cols = cts_dev->fwdata.cols;
		test_frame = chip_data->p_cts_test_para->test_gstr_noise_frames;
		item_limit_type = chip_data->p_cts_test_para->limit_type_gstr_noise;
	} else {
		rows = cts_dev->fwdata.rows;
		cols = CTS_TEST_LP_DATA_CHANNEL;
		test_frame = chip_data->p_cts_test_para->test_gstr_lp_noise_frames;
		item_limit_type = chip_data->p_cts_test_para->limit_type_gstr_lp_noise;
	}

    buf_size = 4 * tsdata_frame_size;
    buffer = (u16 *)kmalloc(buf_size, GFP_KERNEL);
    if (buffer == NULL) {
        TPD_INFO("<E> Alloc mem for gstr noise failed\n");
        return -ENOMEM;
    }
    curr_rawdata	= buffer;
    max_rawdata		= curr_rawdata + 1 * num_nodes;
    min_rawdata		= curr_rawdata + 2 * num_nodes;
    gstr_noise		= curr_rawdata + 3 * num_nodes;

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", "[CTS GSTR NOISEDATA]");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);

    cts_lock_device(cts_dev);
    cts_set_int_data_types(cts_dev, INT_DATA_TYPE_RAWDATA);
    cts_set_int_data_method(cts_dev, INT_DATA_METHOD_POLLING);
    mdelay(50);

    for (i = 0; i < 5; i++) {
        if (cts_tcs_set_gesture_mode(cts_dev, idle_mode))
            TPD_INFO("<E> Set gesture mode: %d failed, retries: %d\n", idle_mode, i);
		else
			break;
    }
	if (i >= 5) {
		TPD_INFO("<E> Set gesture mode failed\n");
		ret = -EIO;
		goto unlock_device;
	}
    mdelay(20);

    /* Skip 10 frames */
    for (i = 0; i < 10; i++) {
        ret = cts_tcs_polling_data(cts_dev, (u8 *)curr_rawdata,
                RAWDATA_BUFFER_SIZE(cts_dev));
        if (ret) {
            TPD_INFO("<E> Get rawdata failed %d\n", ret);
            mdelay(30);
        }
    }

    for (frame = 0; frame < test_frame; frame++) {
        bool data_valid = false;

        for (i = 0; i < 10; i++) {
            ret = cts_tcs_polling_data(cts_dev, (u8 *)curr_rawdata,
                RAWDATA_BUFFER_SIZE(cts_dev));
            if (ret) {
                TPD_INFO("<E> Get rawdata failed %d\n", ret);
                mdelay(30);
				data_valid = false;
            } else {
                data_valid = true;
                break;
            }
        }

        if (!data_valid) {
            TPD_INFO("<E> Read rawdata failed\n");
            ret = -EIO;
            break;
        }

		memset(data_buf, 0, sizeof(data_buf));
		snprintf(data_buf, 64, "[No: %d]\n", frame+1);
		tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
		cts_output_data(cts_dev, cts_testdata, curr_rawdata, idle_mode ? "gstr rawdata" : "gstr lp rawdata");
		
		if (unlikely(first_frame)) {
			memcpy(max_rawdata, curr_rawdata, tsdata_frame_size);
			memcpy(min_rawdata, curr_rawdata, tsdata_frame_size);
			first_frame = false;
		} else {
			for (i = 0; i < num_nodes; i++) {
				if (curr_rawdata[i] > max_rawdata[i]) {
					max_rawdata[i] = curr_rawdata[i];
				} else if (curr_rawdata[i] < min_rawdata[i]) {
					min_rawdata[i] = curr_rawdata[i];
				}
			}
		}
    }
	for (i = 0; i < num_nodes; i++) {
		gstr_noise[i] = max_rawdata[i] - min_rawdata[i];
	}

	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", idle_mode ? "max gstr rawdata" : "max gstr lp rawdata");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
	cts_output_data(cts_dev, cts_testdata, max_rawdata, idle_mode ? "max gstr rawdata" : "max gstr lp rawdata");
	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", idle_mode ? "min gstr rawdata" : "min gstr lp rawdata");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
	cts_output_data(cts_dev, cts_testdata, min_rawdata, idle_mode ? "min gstr rawdata" : "min gstr lp rawdata");
	memset(data_buf, 0, sizeof(data_buf));
	snprintf(data_buf, 64, "%s\n", idle_mode ? "gstr noise" : "gstr lp noise");
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf, strlen(data_buf), cts_testdata->pos);
	cts_output_data(cts_dev, cts_testdata, gstr_noise, idle_mode ? "gstr noise" : "gstr lp noise");

	TPD_INFO("<I> item limit type: %d\n", item_limit_type);
	if (item_limit_type) {
		if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_CERTAIN) {
			if (idle_mode)
				TPD_INFO("<I> [%s] max: %d, min: %d", "Gstr noisedata",
					*chip_data->p_cts_autotest_offset->cts_gstr_noise_max,
					*chip_data->p_cts_autotest_offset->cts_gstr_noise_min);
			else
				TPD_INFO("<I> [%s] max: %d, min: %d", "Gstr lp noisedata",
					*chip_data->p_cts_autotest_offset->cts_gstr_lp_noise_max,
					*chip_data->p_cts_autotest_offset->cts_gstr_lp_noise_min);
			for (r = 0; r < rows; r++) {
				for (c = 0; c < cols; c++) {
					offset = r * cols + c;

					if ((gstr_noise[offset] < *chip_data->p_cts_autotest_offset->cts_gstr_noise_min) &&
						(gstr_noise[offset] > *chip_data->p_cts_autotest_offset->cts_gstr_noise_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", idle_mode ? "Gstr noise" : "Gstr lp noise");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, gstr_noise[offset]);
					}
				}
			}
		} else if (item_limit_type == CTS_ITEM_LIMIT_TYPE_CSV_NODE) {
			for (r = 0; r < rows; r++) {
				for (c = 0; c < cols; c++) {
					offset = r * cols + c;
			
					if ((gstr_noise[offset] < chip_data->p_cts_autotest_offset->cts_gstr_noise_min[offset])
						&& (gstr_noise[offset] > chip_data->p_cts_autotest_offset->cts_gstr_noise_max[offset])) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", idle_mode ? "Gstr noise" : "Gstr lp noise");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, gstr_noise[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> item limit type: %d invalid\n", item_limit_type);
		}
	} else {
		if ((chip_data->p_cts_test_para->test_gstr_noise_max != 0) &&
			(chip_data->p_cts_test_para->test_gstr_noise_min != 0)) {
			for (r = 0; r < rows; r++) {
				for (c = 0; c < cols; c++) {
					offset = r *cols + c;

					if ((gstr_noise[offset] < chip_data->p_cts_test_para->test_gstr_noise_min) &&
						(gstr_noise[offset] > chip_data->p_cts_test_para->test_gstr_noise_max)) {
						if (failed_cnt == 0) {
							TPD_INFO("<I> %s\n", SPLIT_LINE_STR);
							TPD_INFO("<I> %s failed nodes:\n", idle_mode ? "Gstr noise" : "Gstr lp noise");
						}
						failed_cnt++;
						
						TPD_INFO("<I>	%3d: [%-2d][%-2d] = %u\n",
							failed_cnt, r, c, gstr_noise[offset]);
					}
				}
			}
		} else {
			TPD_INFO("<E> limit para invalid, item limit type: %d\n", item_limit_type);
		}
	}

unlock_device:
    cts_set_int_data_method(cts_dev, INT_DATA_METHOD_NONE);
    cts_set_int_data_types(cts_dev, INT_DATA_TYPE_NONE);
    cts_unlock_device(cts_dev);

    if (buffer) {
        kfree(buffer);
    }

	if (ret || failed_cnt) {
		TPD_INFO("<E> %s: test FAIL\n", "TEST_GSTR_NOISEDATA");
		result = -1;
	} else {
		TPD_INFO("<I> %s: test PASS\n", "TEST_GSTR_NOISEDATA");
	}

	TPD_INFO("<I> %s -\n", __func__);

	return result;
}

/* @type true:baseline; false:diffdata */
static void cts_tcs_get_data_for_oplus(struct seq_file *s, void *chip_data, bool type)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    s16 *rawdata = NULL;
    int ret, r, c;
    char type_str[10];
    bool data_valid = true;

    memcpy(type_str, type ? "baseline" : "diffdata", 8);

    TPD_INFO("<I> Show %s\n", type_str);

    r = cts_dev->fwdata.rows;
    c = cts_dev->fwdata.cols;
    rawdata = kzalloc(r * c * 2, GFP_KERNEL);
    if (rawdata == NULL) {
        TPD_INFO("<E> Allocate memory for %s failed\n", type_str);
        return;
    }

    if (type)
        ret = cts_tcs_get_rawdata(cts_dev, (u8 *)rawdata, r * c * 2);
    else
        ret = cts_tcs_get_real_diff(cts_dev, (u8 *)rawdata, r * c * 2);
    if(ret) {
        TPD_INFO("<E> Get %s failed\n", type_str);
        data_valid = false;
    }

    if (data_valid) {
        for (r = 0; r < cts_dev->fwdata.rows; r++) {
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                seq_printf(s, "%5d", rawdata[r * cts_dev->fwdata.cols + c]);
            }
           seq_printf(s, "\n");
        }
    }

    kfree(rawdata);
}


struct cts_interface tcs_if = {
    .get_fw_ver                 = cts_tcs_get_fw_ver,
    .get_lib_ver                = cts_tcs_get_lib_ver,
    .get_ddi_ver                = cts_tcs_get_ddi_ver,
    .get_res_x                  = cts_tcs_get_res_x,
    .get_res_y                  = cts_tcs_get_res_y,
    .get_rows                   = cts_tcs_get_rows,
    .get_cols                   = cts_tcs_get_cols,
    .get_flip_x                 = cts_tcs_get_flip_x,
    .get_flip_y                 = cts_tcs_get_flip_y,
    .get_swap_axes              = cts_tcs_get_swap_axes,
    .get_int_mode               = cts_tcs_get_int_mode,
    .get_int_keep_time          = cts_tcs_get_int_keep_time,
    .get_rawdata_target         = cts_tcs_get_rawdata_target,
    .get_esd_protection         = cts_tcs_get_esd_protection,

    .get_gestureinfo            = cts_tcs_get_gestureinfo,
    .get_touchinfo              = cts_tcs_get_touchinfo,

    .get_fwid                   = cts_tcs_get_fwid,
    .get_workmode               = cts_tcs_get_workmode,
    .set_workmode               = cts_tcs_set_workmode,
    .set_esd_enable             = cts_tcs_set_esd_enable,
    .set_cneg_enable            = cts_tcs_set_cneg_enable,
    .set_mnt_enable             = cts_tcs_set_mnt_enable,
    .set_pwr_mode               = cts_tcs_set_pwr_mode,
    .set_product_en				= cts_tcs_set_product_en,
    .set_openshort_mode			= cts_tcs_set_openshort_mode,

    .init_int_data              = cts_tcs_init_int_data,
    .get_has_int_data           = cts_tcs_get_has_int_data,
    .get_int_data_method        = cts_tcs_get_int_data_method,
    .get_int_data_types         = cts_tcs_get_int_data_types,
    .calc_int_data_size         = cts_tcs_calc_int_data_size,

    .set_charger_plug           = cts_tcs_set_charger_plug,
    .set_earjack_plug           = cts_tcs_set_earjack_plug,
    .set_panel_direction        = cts_tcs_set_panel_direction,
    .set_game_mode              = cts_tcs_set_game_mode,
    .set_panel_report_rate		= cts_tcs_set_panel_rate,
	.set_smooth_lv_set          = cts_tcs_set_smooth_lv_set,
	.set_sensitive_lv_set       = cts_tcs_set_sensitive_lv_set,
	.set_diaphragm_lv_set       = cts_tcs_set_diaphragm_lv_set,
	.get_water_flag             = cts_tcs_get_water_flag,

    .read_hw_reg                = cts_tcs_read_hw_reg,
    .write_hw_reg               = cts_tcs_write_hw_reg,

    .get_rawdata                = cts_tcs_get_rawdata,
    .get_real_diff              = cts_tcs_get_real_diff,
    .get_manual_diff            = cts_tcs_get_manual_diff,
    .get_cnegdata               = cts_tcs_get_cnegdata,

	.set_black_test_pwr_mode	= cts_tcs_set_black_test_pwr_mode,
    .prepare_black_test         = cts_tcs_prepare_black_test,
    .int_pin_test_item          = cts_tcs_test_int_pin,
    .reset_pin_test_item        = cts_tcs_test_reset_pin,
    .open_test_item             = cts_tcs_test_open,
    .short_test_item            = cts_tcs_test_short,
    .compensate_cap_test_item   = cts_tcs_test_compensate_cap,
    .rawdata_test_item          = cts_tcs_test_rawdata,
    .noise_test_item            = cts_tcs_test_noise,

    .gesture_rawdata_test_item  = cts_tcs_test_gesture_rawdata,
    .gesture_noise_test_item    = cts_tcs_test_gesture_noise,

    .get_data_for_oplus         = cts_tcs_get_data_for_oplus,
};
