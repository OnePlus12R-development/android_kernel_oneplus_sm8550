#define LOG_TAG         "Sysfs"

#include "cts_config.h"
#include "cts_core.h"
#include "cts_sfctrl.h"
#include "cts_spi_flash.h"

#ifdef CONFIG_CTS_SYSFS

#define MAX_ARG_NUM                 (100)
#define MAX_ARG_LENGTH              (1024)

static char cmdline_param[MAX_ARG_LENGTH + 1];
int  argc;
char *argv[MAX_ARG_NUM];

static struct chipone_ts_data *cts_dev_get_drvdata(struct device *dev)
{
    struct touchpanel_data * ts = dev_get_drvdata(dev);
    return (struct chipone_ts_data *)ts->chip_data;
}


static int parse_arg(const char *buf, size_t count)
{
    char *p;

    memcpy(cmdline_param, buf, min((size_t)MAX_ARG_LENGTH, count));
    cmdline_param[count] = '\0';

    argc = 0;
    p = strim(cmdline_param);
    if (p == NULL || p[0] == '\0') {
        return 0;
    }

    while (p && p[0] != '\0' && argc < MAX_ARG_NUM) {
        argv[argc++] = strsep(&p, " ,");
    }

    return argc;
}

static ssize_t read_hw_reg_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#define PRINT_ROW_SIZE          (16)
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u32 addr, size, i, remaining;
    u8 *data = NULL;
    ssize_t count = 0;
    int ret;

    TPD_INFO("<I> Read hw register\n");

    if (argc != 2) {
        return scnprintf(buf, PAGE_SIZE,
            "Invalid num args %d\n"
            "  1. echo addr size > read_hw_reg\n"
            "  2. cat read_hw_reg\n", argc);
    }

    ret = kstrtou32(argv[0], 0, &addr);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid address: %s\n", argv[0]);
    }
    ret = kstrtou32(argv[1], 0, &size);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid size: %s\n", argv[1]);
    }

    data = (u8 *)kmalloc(size, GFP_KERNEL);
    if (data == NULL) {
        return scnprintf(buf, PAGE_SIZE, "Allocate buffer for read data failed\n");
    }

    TPD_INFO("<I> Read hw register from 0x%08x size %u\n", addr, size);

    cts_lock_device(cts_dev);
    ret = cts_dev->cts_if->read_hw_reg(cts_dev, addr, data, size);
    cts_unlock_device(cts_dev);
    if (ret) {
        count = scnprintf(buf, PAGE_SIZE, "Read hw register failed %d", ret);
        goto err_free_data;
    }

    remaining = size;
    for (i = 0; i < size && count <= PAGE_SIZE; i += PRINT_ROW_SIZE) {
        size_t linelen = min((size_t)remaining, (size_t)PRINT_ROW_SIZE);
        remaining -= PRINT_ROW_SIZE;

        count += scnprintf(buf + count, PAGE_SIZE - count, "%04x-%04x: ",
                (u16)(addr >> 16), (u16)addr);

        /* Lower version kernel return void */
        hex_dump_to_buffer(data + i, linelen, PRINT_ROW_SIZE, 1,
                    buf + count, PAGE_SIZE - count, true);
        count += strlen(buf + count);

        if (count < PAGE_SIZE) {
            buf[count++] = '\n';
            addr += PRINT_ROW_SIZE;
        } else {
            break;
        }
    }

err_free_data:
    kfree(data);

    return count;
#undef PRINT_ROW_SIZE
}

/* echo addr size > read_reg */
static ssize_t read_hw_reg_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return (argc == 0 ? 0 : count);
}

static DEVICE_ATTR(read_hw_reg, S_IRUSR | S_IWUSR, read_hw_reg_show, read_hw_reg_store);

static ssize_t write_hw_reg_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u32 addr;
    int i, ret;
    u8 *data = NULL;

    parse_arg(buf, count);

    TPD_INFO("<I> Write hw register\n");

    if (argc < 2) {
        TPD_INFO("<E> Too few args %d\n", argc);
        return -EFAULT;
    }

    ret = kstrtou32(argv[0], 0, &addr);
    if (ret) {
        TPD_INFO("<E> Invalid address %s\n", argv[0]);
        return -EINVAL;
    }

    data = (u8 *)kmalloc(argc - 1, GFP_KERNEL);
    if (data == NULL) {
        TPD_INFO("<E> Allocate buffer for write data failed\n");
        return -ENOMEM;
    }

    for (i = 1; i < argc; i++) {
        ret = kstrtou8(argv[i], 0, data + i - 1);
        if (ret) {
            TPD_INFO("<E> Invalid value %s\n", argv[i]);
            goto free_data;
        }
    }

    TPD_INFO("<I> Write hw register from 0x%08x size %u\n", addr, argc - 1);

    cts_lock_device(cts_dev);
    ret = cts_dev->cts_if->write_hw_reg(cts_dev, addr, data, argc - 1);
    cts_unlock_device(cts_dev);
    if (ret)
        TPD_INFO("<E> Write hw register failed %d\n", ret);

free_data:
    kfree(data);

    return (ret < 0 ? ret : count);
}

static DEVICE_ATTR(write_hw_reg, S_IWUSR, NULL, write_hw_reg_store);


static ssize_t curr_firmware_version_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Current firmware version: %04x\n",
        cts_data->cts_dev.fwdata.version);
}
static DEVICE_ATTR(curr_version, S_IRUGO, curr_firmware_version_show, NULL);

static ssize_t curr_ddi_version_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Current ddi version: %02x\n",
        cts_data->cts_dev.fwdata.ddi_version);
}
static DEVICE_ATTR(curr_ddi_version, S_IRUGO, curr_ddi_version_show, NULL);

static ssize_t rows_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Num rows: %u\n",
        cts_data->cts_dev.fwdata.rows);
}
static DEVICE_ATTR(rows, S_IRUGO, rows_show, NULL);

static ssize_t cols_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Num cols: %u\n",
        cts_data->cts_dev.fwdata.cols);
}
static DEVICE_ATTR(cols, S_IRUGO, cols_show, NULL);

static ssize_t res_x_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "X Resolution: %u\n",
        cts_data->cts_dev.fwdata.res_x);
}
static DEVICE_ATTR(res_x, S_IRUGO, res_x_show, NULL);

static ssize_t res_y_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Y Resolution: %u\n",
        cts_data->cts_dev.fwdata.res_y);
}
static DEVICE_ATTR(res_y, S_IRUGO, res_y_show, NULL);

static ssize_t updating_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Updating: %s\n",
        cts_data->cts_dev.rtdata.updating ? "Y" : "N");
}
static DEVICE_ATTR(updating, S_IRUGO, updating_show, NULL);

static struct attribute *cts_dev_firmware_atts[] = {
    &dev_attr_curr_version.attr,
    &dev_attr_curr_ddi_version.attr,
    &dev_attr_rows.attr,
    &dev_attr_cols.attr,
    &dev_attr_res_x.attr,
    &dev_attr_res_y.attr,
    &dev_attr_updating.attr,
    NULL
};

static const struct attribute_group cts_dev_firmware_attr_group = {
    .name  = "cts_firmware",
    .attrs = cts_dev_firmware_atts,
};

static ssize_t ic_type_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "IC Type : %s\n",
        cts_data->cts_dev.hwdata->name);
}
static DEVICE_ATTR(ic_type, S_IRUGO, ic_type_show, NULL);

static ssize_t program_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Program mode: %s\n",
        cts_data->cts_dev.rtdata.program_mode ? "Y" : "N");
}
static ssize_t program_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    int ret;
    parse_arg(buf, count);

    if (argc != 1) {
        TPD_INFO("<E> Invalid num args %d\n", argc);
        return -EFAULT;
    }

    if (*argv[0] == '1' || tolower(*argv[0]) == 'y') {
        ret = cts_enter_program_mode(&cts_data->cts_dev);
        if (ret) {
            TPD_INFO("<E> Enter program mode failed %d\n", ret);
            return ret;
        }
    } else if (*argv[0] == '0' || tolower(*argv[0]) == 'n') {
        ret = cts_enter_normal_mode(&cts_data->cts_dev);
        if (ret) {
            TPD_INFO("<E> Exit program mode failed %d\n", ret);
            return ret;
        }
    } else {
        TPD_INFO("<E> Invalid args\n");
    }

    return count;
}
static DEVICE_ATTR(program_mode, S_IWUSR | S_IRUGO,
        program_mode_show, program_mode_store);

static ssize_t compensate_cap_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u8 *cap = NULL;
    int ret;
    ssize_t count = 0;
    int r, c, min, max, max_r, max_c, min_r, min_c, sum, average;
    int size;

    TPD_INFO("<I> Read '%s'\n", attr->attr.name);

    size = cts_dev->hwdata->num_row * cts_dev->hwdata->num_col;
    cap = kzalloc(size, GFP_KERNEL);
    if (cap == NULL) {
        return scnprintf(buf, PAGE_SIZE,
            "Allocate mem for compensate cap failed\n");
    }

    cts_lock_device(cts_dev);
    ret = cts_dev->cts_if->get_cnegdata(cts_dev, cap, size);
    cts_unlock_device(cts_dev);
    if (ret) {
        kfree(cap);
        return scnprintf(buf, PAGE_SIZE,
            "Get compensate cap failed %d\n", ret);
    }

    max = min = cap[0];
    sum = 0;
    max_r = max_c = min_r = min_c = 0;
    for (r = 0; r < cts_dev->hwdata->num_row; r++) {
        for (c = 0; c < cts_dev->hwdata->num_col; c++) {
            u16 val = cap[r * cts_dev->hwdata->num_col + c];
            sum += val;
            if (val > max) {
                max = val;
                max_r = r;
                max_c = c;
            } else if (val < min) {
                min = val;
                min_r = r;
                min_c = c;
            }
        }
    }
    average = sum / (cts_dev->hwdata->num_row * cts_dev->hwdata->num_col);

    count += scnprintf(buf + count, PAGE_SIZE - count,
        "----------------------------------------------------------------------------\n"
        " Compensatete Cap MIN: [%d][%d]=%u, MAX: [%d][%d]=%u, AVG=%u\n"
        "---+------------------------------------------------------------------------\n"
        "   |", min_r, min_c, min, max_r, max_c, max, average);
    for (c = 0; c < cts_dev->hwdata->num_col; c++) {
        count += scnprintf(buf + count, PAGE_SIZE - count, " %3u", c);
    }
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "\n"
        "---+------------------------------------------------------------------------\n");

    for (r = 0; r < cts_dev->hwdata->num_row; r++) {
        count += scnprintf(buf + count, PAGE_SIZE - count, "%2u |", r);
        for (c = 0; c < cts_dev->hwdata->num_col; c++) {
            count += scnprintf(buf + count, PAGE_SIZE - count,
                " %3u", cap[r * cts_dev->hwdata->num_col + c]);
       }
       buf[count++] = '\n';
    }
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "---+------------------------------------------------------------------------\n");

    kfree(cap);

    return count;
}
static DEVICE_ATTR(compensate_cap, S_IRUGO, compensate_cap_show, NULL);

#ifdef CFG_CTS_HAS_RESET_PIN
static ssize_t reset_pin_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;

    TPD_INFO("<I> Write RESET-PIN: %s\n", (buf[0] == '1') ? "HIGH" : "LOW");

    cts_plat_set_reset(cts_dev->pdata, (buf[0] == '1') ? 1 : 0);
    return count;
}
static DEVICE_ATTR(reset_pin, S_IWUSR, NULL, reset_pin_store);
#endif

static ssize_t irq_info_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct irq_desc *desc;

    TPD_INFO("<I> Read IRQ-INFO\n");

    desc = irq_to_desc(cts_data->pdata->irq);
    if (desc == NULL) {
        return scnprintf(buf, PAGE_SIZE, "IRQ: %d descriptor not found\n",
            cts_data->pdata->irq);
    }

    return scnprintf(buf, PAGE_SIZE,
        "IRQ num: %d, depth: %u, "
        "count: %u, unhandled: %u, last unhandled eslape: %lu\n",
        cts_data->pdata->irq, desc->depth,
        desc->irq_count, desc->irqs_unhandled,
        desc->last_unhandled);
}
static DEVICE_ATTR(irq_info, S_IRUGO, irq_info_show, NULL);

static ssize_t int_data_types_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "%#04x\n",
            cts_data->cts_dev.fwdata.int_data_types);
}

static ssize_t int_data_types_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = dev_get_drvdata(dev);
    u16 type = 0;
    int ret = 0;

    if (buf[0] == '1')
        type = 1;
    else if (buf[0] == '0')
        type = 0;
    else
        return -EIO;

    cts_lock_device(&cts_data->cts_dev);
    ret = cts_set_int_data_types(&cts_data->cts_dev, type);
    cts_unlock_device(&cts_data->cts_dev);
    if (ret)
        return -EIO;
    return count;
}
static DEVICE_ATTR(int_data_types, S_IWUSR | S_IRUGO,
        int_data_types_show, int_data_types_store);

static ssize_t int_data_method_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "%d\n",
            cts_data->cts_dev.fwdata.int_data_method);
}

static ssize_t int_data_method_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = dev_get_drvdata(dev);
    u8 method = 0;
    int ret = 0;

    if (buf[0] == '1')
        method = 1;
    else if (buf[0] == '0')
        method = 0;
    else
        return -EIO;

    cts_lock_device(&cts_data->cts_dev);
    ret = cts_set_int_data_method(&cts_data->cts_dev, method);
    cts_unlock_device(&cts_data->cts_dev);
    if (ret)
        return -EIO;
    return count;
}

static DEVICE_ATTR(int_data_method, S_IWUSR | S_IRUGO,
        int_data_method_show, int_data_method_store);


static struct attribute *cts_dev_misc_atts[] = {
    &dev_attr_ic_type.attr,
    &dev_attr_program_mode.attr,
#ifdef CFG_CTS_HAS_RESET_PIN
    &dev_attr_reset_pin.attr,
#endif
    &dev_attr_irq_info.attr,
    &dev_attr_compensate_cap.attr,
    &dev_attr_read_hw_reg.attr,
    &dev_attr_write_hw_reg.attr,
    &dev_attr_int_data_types.attr,
    &dev_attr_int_data_method.attr,
    NULL
};

static const struct attribute_group cts_dev_misc_attr_group = {
    .name  = "misc",
    .attrs = cts_dev_misc_atts,
};

static const struct attribute_group *cts_dev_attr_groups[] = {
    &cts_dev_firmware_attr_group,
    &cts_dev_misc_attr_group,
    NULL
};

int cts_sysfs_add_device(struct device *dev)
{
    int ret = 0, i;

    TPD_INFO("<I> Add device attr groups\n");

    // Low version kernel NOT support sysfs_create_groups()
    for (i = 0; cts_dev_attr_groups[i]; i++) {
        ret = sysfs_create_group(&dev->kobj, cts_dev_attr_groups[i]);
        if (ret) {
            while (--i >= 0) {
                sysfs_remove_group(&dev->kobj, cts_dev_attr_groups[i]);
            }
            break;
        }
    }

    if (ret) {
        TPD_INFO("<E> Add device attr failed %d\n", ret);
        return ret;
    }

    ret = sysfs_create_link(NULL, &dev->kobj, "chipone-tddi");
    if (ret) {
        TPD_INFO("<E> Create sysfs link error:%d\n", ret);
    }
    return 0;
}

void cts_sysfs_remove_device(struct device *dev)
{
    int i;

    TPD_INFO("<I> Remove device attr groups\n");

    sysfs_remove_link(NULL, "chipone-tddi");
    // Low version kernel NOT support sysfs_remove_groups()
    for (i = 0; cts_dev_attr_groups[i]; i++) {
        sysfs_remove_group(&dev->kobj, cts_dev_attr_groups[i]);
    }
}

#endif /* CONFIG_CTS_SYSFS */
