#define LOG_TAG         "Tool"

#include "cts_config.h"
#include "cts_core.h"

#define CTS_TOOL_IOCTL_TCS_RW_BUF_MAX_SIZ     1024
#define CTS_TOOL_IOCTL_TCS_RW_OPFLAG_READ     1
#define CTS_TOOL_IOCTL_TCS_RW_OPFLAG_WRITE    2


#pragma pack(1)
/** Tool command structure */
struct cts_tool_cmd {
    u8 cmd;
    u8 flag;
    u8 circle;
    u8 times;
    u8 retry;
    u32 data_len;
    u8 addr_len;
    u8 addr[4];
    u8 tcs[2];
    u8 data[PAGE_SIZE];
};
struct cts_ioctl_tcs_rw_data {
    u32 opflags;
    u32 txlen;
    u32 rxlen;
    u8 __user *txbuf;
    u8 __user *rxbuf;
};
#pragma pack()

#define CTS_TOOL_CMD_HEADER_LENGTH            (16)

enum cts_tool_cmd_code {
    CTS_TOOL_CMD_GET_PANEL_PARAM = 0,
    CTS_TOOL_CMD_GET_DOWNLOAD_STATUS = 2,
    CTS_TOOL_CMD_GET_RAW_DATA = 4,
    CTS_TOOL_CMD_GET_DIFF_DATA = 6,
    CTS_TOOL_CMD_READ_HOSTCOMM = 12,
    CTS_TOOL_CMD_READ_ADC_STATUS = 14,
    CTS_TOOL_CMD_READ_GESTURE_INFO = 16,
    CTS_TOOL_CMD_READ_HOSTCOMM_MULTIBYTE = 18,
    CTS_TOOL_CMD_READ_PROGRAM_MODE_MULTIBYTE = 20,
    CTS_TOOL_CMD_READ_ICTYPE = 22,
    CTS_TOOL_CMD_I2C_DIRECT_READ = 24,
    CTS_TOOL_CMD_GET_DRIVER_INFO = 26,
    CTS_TOOL_CMD_TCS_READ_HW_CMD = 28,
    CTS_TOOL_CMD_TCS_READ_DDI_CMD = 30,
    CTS_TOOL_CMD_TCS_READ_FW_CMD = 32,
    CTS_TOOL_CMD_GET_BASE_DATA = 34,
    CTS_TOOL_CMD_GET_INT_DATAS = 36,

    CTS_TOOL_CMD_UPDATE_PANEL_PARAM_IN_SRAM = 1,
    CTS_TOOL_CMD_DOWNLOAD_FIRMWARE_WITH_FILENAME = 3,
    CTS_TOOL_CMD_DOWNLOAD_FIRMWARE = 5,
    CTS_TOOL_CMD_WRITE_HOSTCOMM = 11,
    CTS_TOOL_CMD_WRITE_HOSTCOMM_MULTIBYTE = 15,
    CTS_TOOL_CMD_WRITE_PROGRAM_MODE_MULTIBYTE = 17,
    CTS_TOOL_CMD_I2C_DIRECT_WRITE = 19,
    CTS_TOOL_CMD_TCS_WRITE_HW_CMD = 21,
    CTS_TOOL_CMD_TCS_WRITE_DDI_CMD = 23,
    CTS_TOOL_CMD_TCS_WRITE_FW_CMD = 25,
    CTS_TOOL_CMD_TCS_ENABLE_GET_RAWDATA_CMD = 27,
    CTS_TOOL_CMD_TCS_DISABLE_GET_RAWDATA_CMD = 29,
    CTS_TOOL_CMD_SET_INT_DATA_TYPE_AND_METHOD = 31,
};

#define CTS_IOCTL_RDWR_REG_FLAG_RD          (0x0001)
// TODO: Flags can specify DDI level 1/2/3, read/write flag

struct cts_rdwr_reg {
    __u32 addr;
    __u32 flags;
    __u8  __user *data;
    __u32 len;
    __u32 delay_ms;
};

#define CTS_IOCTL_RDWR_REG_TYPE_FW          (1)
#define CTS_IOCTL_RDWR_REG_TYPE_HW          (2)
#define CTS_IOCTL_RDWR_REG_TYPE_DDI         (3)

#define CTS_RDWR_REG_IOCTL_MAX_REGS         (128)

struct cts_rdwr_reg_ioctl_data {
    __u8  reg_type;
    __u32 nregs;
    struct cts_rdwr_reg __user *regs;
};


#define CTS_TOOL_IOCTL_RDWR_REG    _IOWR('C', 0x20, struct cts_rdwr_reg_ioctl_data *)
#define CTS_TOOL_IOCTL_TCS_RW      _IOWR('C', 0x30, struct cts_ioctl_tcs_rw_data *)


#define CTS_DRIVER_VERSION_CODE \
    ((CFG_CTS_DRIVER_MAJOR_VERSION << 16) | \
     (CFG_CTS_DRIVER_MINOR_VERSION << 8) | \
     (CFG_CTS_DRIVER_PATCH_VERSION << 0))

static struct cts_tool_cmd cts_tool_cmd;


extern int cts_tcs_tool_xtrans(const struct cts_device *cts_dev,
        u8 *tbuf, size_t tlen, u8 *rbuf, size_t rlen);

#define RAWDATA_BUFFER_SIZE(cts_dev) \
    (cts_dev->fwdata.rows * cts_dev->fwdata.cols * 2)
#define DIFFDATA_BUFFER_SIZE(cts_dev) \
    ((cts_dev->fwdata.rows + 2) * (cts_dev->fwdata.cols + 2) * 2)


static int cts_tool_open(struct inode *inode, struct file *file)
{
    file->private_data = PDE_DATA(inode);
    return 0;
}

static ssize_t cts_tool_read(struct file *file,
        char __user *buffer, size_t count, loff_t *ppos)
{
    struct chipone_ts_data *cts_data;
    struct cts_tool_cmd *cmd;
    struct cts_device *cts_dev;
    struct cts_interface *cts_if;
    int ret = 0;

    cts_data = (struct chipone_ts_data *)file->private_data;
    if (cts_data == NULL) {
        TPD_INFO("<E> Read with private_data = NULL\n");
        return -EIO;
    }

    cmd = &cts_tool_cmd;
    cts_dev = &cts_data->cts_dev;
    cts_if = cts_dev->cts_if;
    cts_lock_device(cts_dev);

    switch (cmd->cmd) {
    case CTS_TOOL_CMD_TCS_READ_HW_CMD:
        ret = cts_if->read_hw_reg(cts_dev, get_unaligned_le32(cmd->addr),
            cmd->data, cmd->data_len);
        if (ret)
            TPD_INFO("<E> read_hw_reg info failed %d\n", ret);
        break;
    case CTS_TOOL_CMD_GET_RAW_DATA:
    case CTS_TOOL_CMD_GET_DIFF_DATA:
        TPD_DEBUG("<D> Get %s data row: %u col: %u len: %u\n",
            cmd->cmd == CTS_TOOL_CMD_GET_RAW_DATA ? "raw" : "diff",
            cmd->addr[1], cmd->addr[0], cmd->data_len);

        if (cmd->cmd == CTS_TOOL_CMD_GET_RAW_DATA) {
            ret = cts_if->get_rawdata(cts_dev, cmd->data,
                    RAWDATA_BUFFER_SIZE(cts_dev));
        } else if (cmd->cmd == CTS_TOOL_CMD_GET_DIFF_DATA) {
            ret = cts_if->get_real_diff(cts_dev, cmd->data,
                    DIFFDATA_BUFFER_SIZE(cts_dev));
        }
        if(ret) {
            TPD_INFO("<E> Get %s data failed %d\n",
                cmd->cmd == CTS_TOOL_CMD_GET_RAW_DATA ? "raw" : "diff", ret);
        }

        break;
    case CTS_TOOL_CMD_GET_INT_DATAS:
        memcpy((u8 *)cmd->data, cts_dev->rtdata.int_data,
                cts_dev->fwdata.int_data_size);
        cmd->data_len = cts_dev->fwdata.int_data_size;
        break;
    case CTS_TOOL_CMD_READ_HOSTCOMM:
        ret = cts_fw_reg_readb(cts_dev,
                get_unaligned_le16(cmd->addr), cmd->data);
        if (ret) {
            TPD_INFO("<E> Read firmware reg addr 0x%04x failed %d\n",
                get_unaligned_le16(cmd->addr), ret);
        } else {
            TPD_DEBUG("<D> Read firmware reg addr 0x%04x, val=0x%02x\n",
                get_unaligned_le16(cmd->addr), cmd->data[0]);
        }
        break;

    case CTS_TOOL_CMD_READ_GESTURE_INFO:
        ret = cts_if->get_gestureinfo(cts_dev, cmd->data);
        if (ret)
            TPD_INFO("<E> Get gesture info failed %d\n", ret);
        break;

    case CTS_TOOL_CMD_READ_HOSTCOMM_MULTIBYTE:
        cmd->data_len = min((size_t)cmd->data_len, sizeof(cmd->data));
        ret = cts_fw_reg_readsb(cts_dev, get_unaligned_le16(cmd->addr),
                cmd->data, cmd->data_len);
        if (ret) {
            TPD_INFO("<E> Read firmware reg addr 0x%04x len %u failed %d\n",
                get_unaligned_le16(cmd->addr), cmd->data_len, ret);
        } else {
            TPD_DEBUG("<D> Read firmware reg addr 0x%04x len %u\n",
                get_unaligned_le16(cmd->addr), cmd->data_len);
        }
        break;

    case CTS_TOOL_CMD_READ_PROGRAM_MODE_MULTIBYTE:
        TPD_DEBUG("<D> Read under program mode addr 0x%06x len %u\n",
            (cmd->flag << 16) | get_unaligned_le16(cmd->addr),
            cmd->data_len);
        ret = cts_enter_program_mode(cts_dev);
        if (ret) {
            TPD_INFO("<E> Enter program mode failed %d\n", ret);
            break;
        }

        ret = cts_sram_readsb(&cts_data->cts_dev, get_unaligned_le16(cmd->addr),
                cmd->data, cmd->data_len);
        if (ret)
            TPD_INFO("<E> Read under program mode I2C xfer failed %d\n", ret);

        ret = cts_enter_normal_mode(cts_dev);
        if (ret)
            TPD_INFO("<E> Enter normal mode failed %d\n", ret);
        break;

    default:
        TPD_INFO("<W> Read unknown command %u\n", cmd->cmd);
        ret = -EINVAL;
        break;
    }

    cts_unlock_device(cts_dev);

    if (ret == 0) {
        ret = copy_to_user(buffer, cmd->data, cmd->data_len);
        if (ret) {
            TPD_INFO("<E> Copy data to user buffer failed %d\n", ret);
            return 0;
        }

        return cmd->data_len;
    }

    return 0;
}

static ssize_t cts_tool_write(struct file *file,
        const char __user * buffer, size_t count, loff_t * ppos)
{
    struct chipone_ts_data *cts_data;
    struct cts_device *cts_dev;
    struct cts_tool_cmd *cmd;
    struct cts_interface *cts_if;
    int ret = 0;

    if (count < CTS_TOOL_CMD_HEADER_LENGTH || count > PAGE_SIZE) {
        TPD_INFO("<E> Write len %zu invalid\n", count);
        return -EFAULT;
    }

    cts_data = (struct chipone_ts_data *)file->private_data;
    if (cts_data == NULL) {
        TPD_INFO("<E> Write with private_data = NULL\n");
        return -EIO;
    }

    cts_dev = &cts_data->cts_dev;
    cts_if = cts_dev->cts_if;
    cmd = &cts_tool_cmd;

    ret = copy_from_user(cmd, buffer, CTS_TOOL_CMD_HEADER_LENGTH);
    if (ret) {
        TPD_INFO("<E> Copy command header from user buffer failed %d\n", ret);
        return -EIO;
    } else {
        ret = CTS_TOOL_CMD_HEADER_LENGTH;
    }

    if (cmd->data_len > PAGE_SIZE) {
        TPD_INFO("<E> Write with invalid count %d\n", cmd->data_len);
        return -EIO;
    }

    if(cmd->cmd & BIT(0)) {
        if(cmd->data_len) {
            ret = copy_from_user(cmd->data,
                    buffer + CTS_TOOL_CMD_HEADER_LENGTH, cmd->data_len);
            if (ret) {
                TPD_INFO("<E> Copy command from user buffer len %u failed %d\n",
                    cmd->data_len, ret);
                return -EIO;
            }
        }
    } else {
        TPD_DEBUG("<D> Write read command(%d) header, prepare read size: %d\n",
            cmd->cmd, cmd->data_len);
        return CTS_TOOL_CMD_HEADER_LENGTH + cmd->data_len;
    }

    //cts_dev = &cts_data->cts_dev;
    cts_lock_device(cts_dev);

    switch (cmd->cmd) {
    case CTS_TOOL_CMD_TCS_WRITE_HW_CMD:
        cts_if->write_hw_reg(cts_dev, get_unaligned_le32(cmd->addr),
                cmd->data, cmd->data_len);
        break;
    case CTS_TOOL_CMD_SET_INT_DATA_TYPE_AND_METHOD:
        cts_set_int_data_types(cts_dev, cmd->data[0]);
        cts_set_int_data_method(cts_dev, cmd->data[1]);
        break;
    case CTS_TOOL_CMD_WRITE_HOSTCOMM:
        TPD_DEBUG("<D> Write firmware reg addr: 0x%04x val=0x%02x\n",
            get_unaligned_le16(cmd->addr), cmd->data[0]);

        ret = cts_fw_reg_writeb(cts_dev,
                get_unaligned_le16(cmd->addr), cmd->data[0]);
        if (ret) {
            TPD_INFO("<E> Write firmware reg addr: 0x%04x val=0x%02x failed %d\n",
                get_unaligned_le16(cmd->addr), cmd->data[0], ret);
        }
        break;
    case CTS_TOOL_CMD_WRITE_HOSTCOMM_MULTIBYTE:
        TPD_DEBUG("<D> Write firmare reg addr: 0x%04x len %u\n",
            get_unaligned_le16(cmd->addr), cmd->data_len);
        ret = cts_fw_reg_writesb(cts_dev, get_unaligned_le16(cmd->addr),
                cmd->data, cmd->data_len);
        if (ret) {
            TPD_INFO("<E> Write firmare reg addr: 0x%04x len %u failed %d\n",
                get_unaligned_le16(cmd->addr), cmd->data_len, ret);
        }
        break;

    case CTS_TOOL_CMD_WRITE_PROGRAM_MODE_MULTIBYTE:
        TPD_DEBUG("<D> Write to addr 0x%06x size %u under program mode\n",
            (cmd->flag << 16) | (cmd->addr[1] << 8) | cmd->addr[0],
            cmd->data_len);
        ret = cts_enter_program_mode(cts_dev);
        if (ret) {
            TPD_INFO("<E> Enter program mode failed %d\n", ret);
            break;
        }

        ret = cts_sram_writesb(cts_dev,
                (cmd->flag << 16) | (cmd->addr[1] << 8) | cmd->addr[0],
                cmd->data, cmd->data_len);
        if (ret)
            TPD_INFO("<E> Write program mode multibyte failed %d\n", ret);

        ret = cts_enter_normal_mode(cts_dev);
        if (ret)
            TPD_INFO("<E> Enter normal mode failed %d\n", ret);
        break;

    default:
        TPD_INFO("<W> Write unknown command %u\n", cmd->cmd);
        ret = -EINVAL;
        break;
    }

    cts_unlock_device(cts_dev);

    return ret ? 0 : cmd->data_len + CTS_TOOL_CMD_HEADER_LENGTH;
}

static int cts_ioctl_rdwr_reg(struct cts_device *cts_dev,
    u8 reg_type, u32 nregs, struct cts_rdwr_reg *regs)
{
    int i, ret = 0;
    u8 fw_esd_protect = 0;

    TPD_INFO("<I> ioctl RDWR_REG type: %u total %u regs\n", reg_type, nregs);

    cts_lock_device(cts_dev);

    if (reg_type == CTS_IOCTL_RDWR_REG_TYPE_DDI) {
        ret = cts_tcs_get_esd_protection(cts_dev, &fw_esd_protect);
        if (ret) {
            TPD_INFO("<E> Get fw esd protection failed %d\n", ret);
            goto unlock_device;
        }

        if (fw_esd_protect) {
            ret = cts_tcs_set_esd_enable(cts_dev, false);
            if (ret) {
                TPD_INFO("<E> Set fw esd protection failed %d\n", ret);
                goto unlock_device;
            }
        }

        ret = cts_dev->hwdata->enable_access_ddi_reg(cts_dev, true);
        if (ret) {
            TPD_INFO("<E> Enable access ddi reg failed %d\n", ret);
            goto recovery_fw_esd_protect;
        }
    }

    for (i = 0; i < nregs; i++) {
        struct cts_rdwr_reg *reg = regs + i;
        u8 *data = NULL;

        TPD_DEBUG("<D>   reg: %p flags: 0x%x data: %p len: %u delay: %u\n",
            reg, reg->flags, reg->data, reg->len, reg->delay_ms);

        if (reg->data == NULL || reg->len == 0) {
            TPD_INFO("<E> Rdwr reg(addr: 0x%06x) with data: %p or len: %u\n",
                reg->addr, reg->data, reg->len);
            ret = -EINVAL;
            goto disable_access_ddi_reg;
        }

        if (reg->flags & CTS_IOCTL_RDWR_REG_FLAG_RD) {
            u8 __user *user_data = reg->data;

            data = kmalloc(reg->len, GFP_KERNEL);
            if (data == NULL) {
                TPD_INFO("<E> Alloc mem for read reg(addr: 0x%06x len: %u) data failed\n",
                    reg->addr, reg->len);
                ret = -ENOMEM;
                goto disable_access_ddi_reg;
            }
            if (reg_type == CTS_IOCTL_RDWR_REG_TYPE_FW) {
                ret = cts_fw_reg_readsb(cts_dev,
                    regs->addr, data, reg->len);
            } else {
                ret = cts_hw_reg_readsb(cts_dev,
                    regs->addr, data, reg->len);
            }
            if (ret) {
                kfree(data);
                TPD_INFO("<E> Read reg from addr: 0x%06x len: %u failed %d\n",
                    reg->addr, reg->len, ret);
                goto disable_access_ddi_reg;
            }
            if (copy_to_user(user_data, data, reg->len)) {
                kfree(data);
                TPD_INFO("<E> Copy reg(addr: 0x%06x len: %u) data to user failed\n",
                    reg->addr, reg->len);
            }
            kfree(data);
        } else {
            data = memdup_user(reg->data, reg->len);
            if (IS_ERR(data)) {
                ret = PTR_ERR(data);
                TPD_INFO("<E> Memdup reg(addr: 0x%06x len: %u) data from user failed\n",
                    reg->addr, reg->len);
                goto disable_access_ddi_reg;
            }
            if (reg_type == CTS_IOCTL_RDWR_REG_TYPE_FW) {
                ret = cts_fw_reg_writesb(cts_dev,
                    regs->addr, data, reg->len);
            } else {
                ret = cts_hw_reg_writesb(cts_dev,
                    regs->addr, data, reg->len);
            }
            kfree(data);
            if (ret) {
                TPD_INFO("<E> Write reg from addr 0x%06x len %u failed %d\n",
                    reg->addr, reg->len, ret);
                goto disable_access_ddi_reg;
            }
        }

        if (reg->delay_ms) {
            mdelay(reg->delay_ms);
        }
    }

disable_access_ddi_reg:
    if (reg_type == CTS_IOCTL_RDWR_REG_TYPE_DDI) {
        int r = cts_dev->hwdata->enable_access_ddi_reg(cts_dev, false);
        if (r) {
            TPD_INFO("<E> Disable access ddi reg failed %d\n", r);
        }
    }

recovery_fw_esd_protect:
    if (reg_type == CTS_IOCTL_RDWR_REG_TYPE_DDI && fw_esd_protect) {
        int r = cts_tcs_set_esd_enable(cts_dev, true);
        if (r) {
            TPD_INFO("<E> Re-Enable fw esd protection failed %d\n", r);
        }
    }

unlock_device:
    cts_unlock_device(cts_dev);

    kfree(regs);

    return ret;
}

static long cts_ioctl_tcs_rw(struct cts_device *cts_dev,
        struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct cts_ioctl_tcs_rw_data rwdata;
    u8 *txbuf = NULL;
    u8 *rxbuf = NULL;

    if (copy_from_user(&rwdata,
            (struct cts_ioctl_tcs_rw_data __user *)arg,
            sizeof(rwdata))) {
        TPD_INFO("<E> Copy ioctl tcs rw arg to kernel failed\n");
        return -EFAULT;
    }

    if ((rwdata.opflags != CTS_TOOL_IOCTL_TCS_RW_OPFLAG_READ) &&
        (rwdata.opflags != CTS_TOOL_IOCTL_TCS_RW_OPFLAG_WRITE))
    {
        TPD_INFO("<E> ioctl tcs rw with invalid opflags %u\n", rwdata.opflags);
        return -EINVAL;
    }

    if (rwdata.txlen > CTS_TOOL_IOCTL_TCS_RW_BUF_MAX_SIZ) {
        TPD_INFO("<E> Send too long: %d\n", rwdata.txlen);
        return -EINVAL;
    }

    if (rwdata.rxlen > CTS_TOOL_IOCTL_TCS_RW_BUF_MAX_SIZ) {
        TPD_INFO("<E> Recv too long: %d\n", rwdata.rxlen);
        return -EINVAL;
    }

    if (!rwdata.txbuf || !rwdata.rxbuf) {
        TPD_INFO("<E> Txbuf of Rxbuf is NULL!\n");
        return -EINVAL;
    }

    txbuf = memdup_user(rwdata.txbuf, rwdata.txlen);
    if (IS_ERR(txbuf)) {
        ret = PTR_ERR(txbuf);
        TPD_INFO("<E> Memdup txbuf failed %d\n", ret);
        txbuf = NULL;
        goto free_buf;
    }

    rxbuf = memdup_user(rwdata.rxbuf, rwdata.rxlen);
    if (IS_ERR(rxbuf)) {
        ret = PTR_ERR(rxbuf);
        TPD_INFO("<E> Memdup rxbuf failed %d\n", ret);
        rxbuf = NULL;
        goto free_buf;
    }

    cts_lock_device(cts_dev);
    ret = cts_tcs_tool_xtrans(cts_dev, txbuf, rwdata.txlen, rxbuf, rwdata.rxlen);
    cts_unlock_device(cts_dev);
    if (ret < 0) {
        TPD_INFO("<E> Trans failed %d\n", ret);
        goto free_buf;
    }

    ret = copy_to_user(rwdata.rxbuf, cts_dev->rtdata.rbuf, rwdata.rxlen);
    if (ret < 0) {
        TPD_INFO("<E> Copy to user failed %d\n", ret);
        goto free_buf;
    }

    ret = 0;

free_buf:
    if (txbuf) {
        kfree(txbuf);
        txbuf = NULL;
    }
    if (rxbuf) {
        kfree(rxbuf);
        rxbuf = NULL;
    }

    return ret;
}


static long cts_tool_ioctl(struct file *file, unsigned int cmd,
        unsigned long arg)
{
    struct chipone_ts_data *cts_data;
    struct cts_device *cts_dev;

    TPD_INFO("<I> ioctl, cmd=0x%08x, arg=0x%08lx\n", cmd, arg);

    cts_data = file->private_data;
    if (cts_data == NULL) {
        TPD_INFO("<E> IOCTL with private data = NULL\n");
        return -EFAULT;
    }

    cts_dev = &cts_data->cts_dev;

    switch (cmd) {
    case CTS_TOOL_IOCTL_RDWR_REG:{
        struct cts_rdwr_reg_ioctl_data ioctl_data;
        struct cts_rdwr_reg *regs_pa;

        if (copy_from_user(&ioctl_data,
                (struct cts_rdwr_reg_ioctl_data __user *)arg,
                sizeof(ioctl_data))) {
            TPD_INFO("<E> Copy ioctl rdwr_reg arg to kernel failed\n");
            return -EFAULT;
        }

        if (ioctl_data.nregs > CTS_RDWR_REG_IOCTL_MAX_REGS) {
            TPD_INFO("<E> ioctl rdwr_reg with too many regs %u\n",
                ioctl_data.nregs);
            return -EINVAL;
        }

        regs_pa = memdup_user(ioctl_data.regs,
            ioctl_data.nregs * sizeof(struct cts_rdwr_reg));
        if (IS_ERR(regs_pa)) {
            int ret = PTR_ERR(regs_pa);
            TPD_INFO("<E> Memdump cts_rdwr_reg to kernel failed %d\n", ret);
            return ret;
        }

        return cts_ioctl_rdwr_reg(cts_dev,
            ioctl_data.reg_type, ioctl_data.nregs, regs_pa);
    }
    case CTS_TOOL_IOCTL_TCS_RW:{
        return cts_ioctl_tcs_rw(cts_dev, file, cmd, arg);
    }
    default:
        TPD_INFO("<E> Unsupported ioctl cmd=0x%08x, arg=0x%08lx\n", cmd, arg);
        break;
    }

    return -ENOTSUPP;
}

static struct proc_ops cts_tool_fops = {
 //   .owner = THIS_MODULE,
 //   .llseek = no_llseek,
    .proc_open  = cts_tool_open,
    .proc_read  = cts_tool_read,
    .proc_write = cts_tool_write,
    .proc_ioctl = cts_tool_ioctl,
};

int cts_tool_init(struct chipone_ts_data *data)
{
    struct proc_dir_entry *entry = NULL;
    TPD_INFO("<I> Init\n");

   entry = proc_create_data(CFG_CTS_TOOL_PROC_FILENAME,
            0666, NULL, &cts_tool_fops, data);
    if (entry == NULL) {
        TPD_INFO("<E> Create proc entry failed\n");
        return -EFAULT;
    }

    return 0;
}

void cts_tool_deinit(struct chipone_ts_data *data)
{
    TPD_INFO("<I> Deinit\n");

    if (data->procfs_entry) {
        remove_proc_entry(CFG_CTS_TOOL_PROC_FILENAME, NULL);
    }
}

