#define LOG_TAG "VENDOR"

#include <linux/firmware.h>

#include "cts_config.h"
#include "cts_core.h"
#include "cts_sysfs.h"
#ifndef REMOVE_OPLUS_FUNCTION
#ifdef CONFIG_TOUCHPANEL_MTK_PLATFORM
#include<mt-plat/mtk_boot_common.h>
#else
#include <soc/oplus/system/boot_mode.h>
#endif
#endif
struct touchpanel_data *tsdata = NULL;
extern struct cts_interface tcs_if;

#ifdef CONFIG_CTS_I2C_HOST
int cts_init_platform_data(struct cts_platform_data *pdata,
        struct i2c_client *i2c_client)
#else
int cts_init_platform_data(struct cts_platform_data *pdata,
        struct spi_device *spi)
#endif
{
    int ret = 0;

    TPD_INFO("<I> Init platform data\n");

#ifdef CONFIG_CTS_I2C_HOST
    pdata->i2c_client = i2c_client;
#else
    pdata->spi_client = spi;
#endif

    mutex_init(&pdata->dev_lock);

#ifdef CONFIG_CTS_I2C_HOST
    pdata->i2c_client = i2c_client;
    pdata->i2c_client->irq = pdata->irq;
#else
    pdata->spi_client = spi;
    pdata->spi_client->irq = pdata->irq;
#endif /* CONFIG_CTS_I2C_HOST */

    spin_lock_init(&pdata->irq_lock);

#ifndef CONFIG_CTS_I2C_HOST
    pdata->spi_speed = CFG_CTS_SPI_SPEED_KHZ;
    cts_plat_spi_setup(pdata);
#endif
    return ret;
}

#ifdef CFG_CTS_HAS_RESET_PIN
int cts_plat_reset_device(struct cts_platform_data *pdata)
{
    TPD_INFO("<I> Reset device\n");

    gpio_set_value(pdata->rst_gpio, 1);
    mdelay(1);
    gpio_set_value(pdata->rst_gpio, 0);
    mdelay(10);
    gpio_set_value(pdata->rst_gpio, 1);
    mdelay(50);
    return 0;
}

int cts_plat_set_reset(struct cts_platform_data *pdata, int val)
{
    TPD_INFO("<I> Set reset-pin to %s\n", val ? "HIGH" : "LOW");
    if (val) {
        gpio_set_value(pdata->rst_gpio, 1);
    } else {
        gpio_set_value(pdata->rst_gpio, 0);
    }
    return 0;
}
#endif /* CFG_CTS_HAS_RESET_PIN */

int cts_plat_get_int_pin(struct cts_platform_data *pdata)
{
    /* MTK Platform can not get INT pin value */
    return -ENOTSUPP;
}

static int cts_get_chip_info(void *chip_data)
{
    TPD_INFO("<E> %s\n", tsdata->panel_data.manufacture_info.version);

    return 0;
}

static int cts_mode_switch(void *chip_data, work_mode mode, int flag)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
    u8 pwr_mode;
    int direct;
    int ret = 0;

    cts_lock_device(cts_dev);
    switch (mode) {
        case MODE_NORMAL:
            break;
        case MODE_SLEEP:
            TPD_INFO("<I> switch MODE_SLEEP, SUSPEND\n");
            //cts_plat_reset_device(cts_data->pdata);
            //msleep(50);
            pwr_mode = 2;
            cts_if->set_pwr_mode(cts_dev, pwr_mode);
            break;
        case MODE_EDGE:
            //direct = cts_data->touch_direction;
            direct = flag;
            TPD_INFO("<I> switch MODE_EDGE, direct:%d\n", direct);
            ret = cts_if->set_panel_direction(cts_dev, direct);
            if (ret) {
                TPD_INFO("<E> Set direct: %d, failed! %d\n", direct, ret);
            }
            break;
        case MODE_GESTURE:
            if (flag) {
                TPD_INFO("<I> switch MODE_GESTURE, SUSPEND_WITH_GESTURE\n");
                //cts_plat_reset_device(cts_data->pdata);
                //msleep(50);
				if (cts_data->tsdata->gesture_test.flag) {
	                TPD_INFO("<I> Gesture test flag: %d\n", cts_data->tsdata->gesture_test.flag);
					ret = cts_if->set_black_test_pwr_mode(cts_dev, 0x01);
					if (ret) {
		                TPD_INFO("<I> Set black test pwr mode failed! %d\n", ret);
					}
				}
                pwr_mode = 3;
                cts_if->set_pwr_mode(cts_dev, pwr_mode);
            }
            break;
        case MODE_CHARGE:
            TPD_INFO("<I> switch MODE_CHARGE, flag: %d\n", flag);
            ret = cts_if->set_charger_plug(cts_dev, flag);
            if (ret)
                TPD_INFO("<E> Set charger mode failed %d\n", ret);
            break;
        case MODE_GAME:
            TPD_INFO("<I> switch MODE_GAME: %s\n", flag ? "In" : "Out");
            ret = cts_if->set_game_mode(cts_dev, flag ? 1 : 0);
            if (ret)
                TPD_INFO("<E> Set dev game mode failed %d\n", ret);
            break;
        case MODE_HEADSET:
            TPD_INFO("<I> switch MODE_HEADSET, flag: %d\n", flag);
            ret = cts_if->set_earjack_plug(cts_dev, flag);
            if (ret)
                TPD_INFO("<E> Set earjack mode failed %d\n", ret);
            break;
        default:
            break;
    }
    cts_unlock_device(cts_dev);

    return ret;
}
static unsigned int cts_trigger_reason(void *chip_data, int gesture_enable,
        int is_suspended)
{


    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
    struct cts_device_touch_info *touch_info = &cts_dev->rtdata.touch_info;
    //struct cts_device_touch_msg *msgs = touch_info->msgs;
    struct cts_device_gesture_info *gesture_info = &cts_dev->rtdata.gesture_info;
    //int num;
    int ret = -1;

    if (cts_dev->rtdata.program_mode) {
        TPD_INFO("<E> IRQ triggered in program mode\n");
        return IRQ_IGNORE;
    }

    if (cts_if == NULL) {
        TPD_INFO("<E> cts_if = NULL, Not init cts_dev->cts_if\n");
        return IRQ_IGNORE;
    }

    cts_lock_device(cts_dev);
    ret = cts_if->get_touchinfo(cts_dev, touch_info);
    cts_unlock_device(cts_dev);
    if (ret < 0) {
        TPD_INFO("<E> Get touch info failed %d\n", ret);
        return IRQ_IGNORE;
    }

    //num = touch_info->num_msg;

    //if (num == 0 || num > CFG_CTS_MAX_TOUCH_NUM) {
    //    return IRQ_IGNORE;
    //}

    if ((gesture_enable == 1) && (is_suspended == 1)) {
        return IRQ_GESTURE;
    } else if (is_suspended == 1) {
        return IRQ_IGNORE;
    }

   // gesture_info = &cts_dev->rtdata.gesture_info;
    memcpy(gesture_info, touch_info, sizeof(struct cts_device_gesture_info));
    //TPD_INFO("palm_gesture_id = 0x%x\n", gesture_info->gesture_id);
    if (((gesture_info->gesture_id & 0xFF) == 0x30) && !(is_suspended == 1)) {
        TPD_INFO("palm_gesture_id = 0x%x\n", gesture_info->gesture_id);
        TPD_INFO("chip_palm_to_sleep_enable\n");
        return IRQ_PALM;
    }

    return IRQ_TOUCH;

}

static int cts_get_touch_points(void *chip_data,
        struct point_info *points, int max_num)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_device_touch_info *touch_info = &cts_dev->rtdata.touch_info;
    struct cts_device_touch_msg *msgs = touch_info->msgs;
    int num;
    int obj_attention = 0;
    //int ret = -1;
    int i;

    // if (cts_dev->rtdata.program_mode) {
    //     TPD_INFO("<E> IRQ triggered in program mode\n");
    //     return -EINVAL;
    // }

    // if (cts_if == NULL) {
    //     TPD_INFO("<E> cts_if = NULL, Not init cts_dev->cts_if\n");
    //     return -EINVAL;
    // }

    // cts_lock_device(cts_dev);
    // ret = cts_if->get_touchinfo(cts_dev, &touch_info);
    // cts_unlock_device(cts_dev);
    // if (ret < 0) {
    //     TPD_INFO("<E> Get touch info failed %d\n", ret);
    //     return ret;
    // }

    num = touch_info->num_msg;

    TPD_DEBUG("<D> Process touch %d msgs\n", num);
    if (num == 0 || num > CFG_CTS_MAX_TOUCH_NUM) {
        return -EINVAL;
    }

    for (i = 0; i < (num > max_num ? max_num : num); i++) {
        u16 x, y;

        x = le16_to_cpu(msgs[i].x);
        y = le16_to_cpu(msgs[i].y);

        TPD_DEBUG("<D>   Process touch msg[%d]: id[%u] ev=%u x=%u y=%u p=%u\n",
                i, msgs[i].id, msgs[i].event, x, y, msgs[i].pressure);
        if ((msgs[i].id < max_num)
        && ((msgs[i].event == CTS_DEVICE_TOUCH_EVENT_DOWN)
        || (msgs[i].event == CTS_DEVICE_TOUCH_EVENT_MOVE)
        || (msgs[i].event == CTS_DEVICE_TOUCH_EVENT_STAY))) {
            points[msgs[i].id].x = x;
            points[msgs[i].id].y = y;
            points[msgs[i].id].z = 1;
            points[msgs[i].id].width_major = 0;
            points[msgs[i].id].touch_major = 0;
            points[msgs[i].id].status = 1;
            obj_attention |= (1 << msgs[i].id);
        }
    }

    return obj_attention;
}

static int cts_get_gesture_info(void *chip_data,
        struct gesture_info *gesture)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    //struct cts_interface *cts_if = cts_dev->cts_if;
    struct cts_device_touch_info *touch_info = &cts_dev->rtdata.touch_info;
    struct cts_device_gesture_info *gesture_info = &cts_dev->rtdata.gesture_info;
    uint32_t gesture_type;
   
    memcpy(gesture_info, touch_info, sizeof(struct cts_device_gesture_info));

    TPD_INFO("<I> Process gesture, id=0x%02x, num_points=%d\n",
            gesture_info->gesture_id, gesture_info->num_points);

    memset(gesture, 0, sizeof(*gesture));
  switch(gesture_info->gesture_id) {
    case CTS_GESTURE_D_TAP: gesture_type = DOU_TAP;          break;
    case CTS_GESTURE_V:     gesture_type = UP_VEE;           break;
    case CTS_GESTURE_M:     gesture_type = M_GESTRUE;        break;
    case CTS_GESTURE_W:     gesture_type = W_GESTURE;        break;
    case CTS_GESTURE_O:     gesture_type = CIRCLE_GESTURE;          break;
    case CTS_GESTURE_RV:    gesture_type = DOWN_VEE;         break;
    //case CTS_GESTURE_UP:    gesture_type = Down2UpSwip;     break;
    //case CTS_GESTURE_DOWN:  gesture_type = Up2DownSwip;     break;
    //case CTS_GESTURE_LEFT:  gesture_type = Right2LeftSwip;  break;
    //case CTS_GESTURE_RIGHT: gesture_type = Left2RightSwip;  break;
    case CTS_GESTURE_DOUBLE: gesture_type = DOU_SWIP;         break;
    case CTS_GESTURE_LR:    gesture_type = RIGHT_VEE;        break;
    case CTS_GESTURE_RR:    gesture_type = LEFT_VEE;         break;
    default: gesture_type = UnkownGesture;                  break;
    }

    gesture->gesture_type = gesture_type;

    if (gesture_info->num_points >= 1) {
        gesture->Point_start.x = gesture_info->points[0].x;
        gesture->Point_start.y = gesture_info->points[0].y;
    }
    if (gesture_info->num_points >= 2) {
        gesture->Point_end.x = gesture_info->points[1].x;
        gesture->Point_end.y = gesture_info->points[1].y;
    }
    if (gesture_info->num_points >= 3) {
        gesture->Point_1st.x = gesture_info->points[2].x;
        gesture->Point_1st.y = gesture_info->points[2].y;
    }
    if (gesture_info->num_points >= 4) {
        gesture->Point_2nd.x = gesture_info->points[3].x;
        gesture->Point_2nd.y = gesture_info->points[3].y;
    }
    if (gesture_info->num_points >= 5) {
        gesture->Point_3rd.x = gesture_info->points[4].x;
        gesture->Point_3rd.y = gesture_info->points[4].y;
    }
    if (gesture_info->num_points >= 6) {
        gesture->Point_4th.x = gesture_info->points[5].x;
        gesture->Point_4th.y = gesture_info->points[5].y;
    }
    return 0;
}

/* Used In Factory Mode, Not Need TP Work!!! */
static int cts_ftm_process(void *chip_data) {
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
    u8 pwr_mode = 2;
    int ret;

    cts_lock_device(cts_dev);
    ret = cts_if->set_pwr_mode(cts_dev, pwr_mode);
    cts_unlock_device(cts_dev);
    if (ret)
        TPD_INFO("<E> Send CMD_SUSPEND failed %d\n", ret);
    return 0;
}

static int cts_read_fw_ddi_version(void *chip_data, struct touchpanel_data *ts)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
    u16 fw_ver;
    char *vendor;
    char *version;
    int ret = -1;

    vendor = ts->panel_data.manufacture_info.manufacture;
    version = ts->panel_data.manufacture_info.version;

    ret = cts_if->get_fw_ver(cts_dev, &fw_ver);
    if (ret) {
        TPD_INFO("<E> get firmware version failed %d\n", ret);
        return ret;
    }

    if (cts_dev->hwdata->hwid == CTS_DEV_HWID_ICNL9916)
        scnprintf(version, 16, "%s_%s_%04X", vendor, "9916", fw_ver);
    else if (cts_dev->hwdata->hwid == CTS_DEV_HWID_ICNL9911C)
        scnprintf(version, 16, "%s_%s_%04X", vendor, "9911C", fw_ver);
    else
        scnprintf(version, 16, "%s_xxx_xxx", vendor);

    TPD_INFO("<I> vendor:%s, version:%s\n", vendor, version);

    return ret;
}

static fw_update_state cts_fw_update(void *chip_data, const struct firmware *fw,
        bool force)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    int ret;

    /* If fw was not NULL, then need to update vfw.data buf. */
    if (fw && cts_data->vfw.data) {
        memcpy(cts_data->vfw.data, fw->data, fw->size);
        cts_data->vfw.name = cts_dev->hwdata->name;
        cts_data->vfw.hwid = cts_dev->hwdata->hwid;
        cts_data->vfw.fwid = cts_dev->hwdata->fwid;
        cts_data->vfw.size = fw->size;
    }

    cts_lock_device(cts_dev);
    ret = cts_update_firmware(cts_dev, &cts_data->vfw, false);
    cts_unlock_device(cts_dev);
    if (ret < 0) {
        return FW_UPDATE_ERROR;
    }

    cts_lock_device(cts_dev);
    ret = cts_read_fw_ddi_version(chip_data, tsdata);
    cts_unlock_device(cts_dev);
    if (ret)
        TPD_INFO("<E> get fw_ddi_version failed %d\n", ret);

    return FW_UPDATE_SUCCESS;
}


static int cts_reset(void *chip_data)
{
    int ret;
    mdelay(10);
    /* cts_plat_reset_device(cts_data->pdata); */

    ret = cts_fw_update(chip_data, NULL, false);
    if (ret == FW_UPDATE_SUCCESS) {
        TPD_INFO("<I> fw_update success!\n");
    } else if (ret == FW_UPDATE_ERROR) {
        TPD_INFO("<E> fw_update failed!\n");
    }

    return 0;
}

static void cts_tp_touch_release(struct touchpanel_data *ts)
{
#ifdef TYPE_B_PROTOCOL
    int i = 0;

    if (ts->report_flow_unlock_support) {
        mutex_lock(&ts->report_mutex);
    }
    for (i = 0; i < ts->max_num; i++) {
        input_mt_slot(ts->input_dev, i);
        input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
    }
    input_report_key(ts->input_dev, BTN_TOUCH, 0);
    input_report_key(ts->input_dev, BTN_TOOL_FINGER, 0);
    input_sync(ts->input_dev);
    if (ts->report_flow_unlock_support) {
        mutex_unlock(&ts->report_mutex);
    }
#else
    input_report_key(ts->input_dev, BTN_TOUCH, 0);
    input_report_key(ts->input_dev, BTN_TOOL_FINGER, 0);
    input_mt_sync(ts->input_dev);
    input_sync(ts->input_dev);
#endif
    TPD_INFO("<I> release all touch point\n");
    ts->view_area_touched = 0;
    ts->touch_count = 0;
    ts->irq_slot = 0;
}

static int cts_esd_handle(void* chip_data)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    int retry = 5;
    int ret;

    TPD_DEBUG("<D> ESD protection work\n");

    cts_lock_device(cts_dev);
    /* ret==0:means esd hanppened */
    ret = cts_plat_is_normal_mode(cts_data->pdata);
    cts_unlock_device(cts_dev);
    if (!ret) {
        TPD_INFO("<E> Handle ESD event!\n");
        do {
            if (cts_reset(chip_data))
                TPD_INFO("<E> Reset chip and update fw failed!\n");
            else
                break;
        } while (retry--);
        ret = -1;
        cts_tp_touch_release(tsdata);
    } else {
        ret = 0;
        TPD_DEBUG("<D> None ESD event!\n");
    }

    return ret;
}

static fw_check_state cts_fw_check(void *chip_data,
        struct resolution_info *resolution_info,
        struct panel_info *panel_data)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
    u16 fwid;

    TPD_INFO("<I> Enter %s!\n", __func__);

    if (cts_if->get_fwid(cts_dev, &fwid) < 0) {
        return FW_ABNORMAL;
    }
    if (fwid == CTS_DEV_FWID_INVALID) {
        return FW_ABNORMAL;
    }

    return FW_NORMAL;
}

static uint8_t cts_get_touch_direction(void *chip_data)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    return cts_data->touch_direction;
}
static void cts_set_touch_direction(void *chip_data, uint8_t dir)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    cts_data->touch_direction = dir;
    TPD_INFO("<I> Set touch_direction:%d\n", dir);
}
static int cts_power_control(void *chip_data, bool enable)
{
    return 0;
}
static void cts_rate_white_list_ctrl(void *chip_data, int value)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
	uint8_t cmd = 1;
	int ret = -1;
	TPD_INFO("write rate, %d(level)\n", value);
	if (cts_data == NULL) {
		TPD_INFO("<E> cts_data = NULL!\n");
		return ;
	}

	switch (value) {
        case 120:  /*120HZ*/
            cmd = 0xA0;
            break;
        case 180:   /*  60HZ*/
            cmd = 0xC0;
            break;
		default:
			TPD_INFO("<I> report rate invalid\n");
		return ;
	}

	ret = cts_if->set_panel_report_rate(cts_dev, cmd);
	if (ret) {
		TPD_INFO("<E> Set report rate");
	}
}
/*smooth ckliu*/
static int cts_smooth_lv_set(void *chip_data, int level)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
	int ret = -1;
	uint8_t cmd = 1;
	//mutex_lock(&chip_info->touch_mutex);
	TPD_INFO("write smooth level, 0x%x(level)\n", level);	
	if (cts_data == NULL) {
		TPD_INFO("<E> cts_data = NULL!\n");
		return -1;
	}

	switch (level) {
		case 1:	/* 一档 */
			cmd = 0x01;
			break;
		case 2:	/* 二挡 */
			cmd = 0x02;
            break;
        case 3:	/* 三挡 */
			cmd = 0x00;
            break;
        case 4:	/* 四挡 */
			cmd = 0x04;    
			break;
        case 5:	/* 五档 */
			cmd = 0x08;
            break;
		default:
			TPD_INFO("<I> report rate invalid\n");
		return -1;
	}

	ret = cts_if->set_smooth_lv_set(cts_dev, cmd);
	//mutex_unlock(&chip_info->touch_mutex);
	return ret;
}


static int cts_sensitive_lv_set(void *chip_data, int level)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
	int ret = -1;
	uint8_t cmd = 1;
	//mutex_lock(&chip_info->touch_mutex);
	TPD_INFO("write sensitive level, 0x%x(level)\n", level);	
	if (cts_data == NULL) {
		TPD_INFO("<E> cts_data = NULL!\n");
		return -1;
	}

	switch (level) {
		case 1:	/* 一档 */
			cmd = 0x01;
			break;
		case 2:	/* 二挡 */
			cmd = 0x02;
            break;
        case 3:	/* 三挡 */
			cmd = 0x00;
            break;
        case 4:	/* 四挡 */
			cmd = 0x04;    
			break;
        case 5:	/* 五档 */
			cmd = 0x08;
            break;
		default:
			TPD_INFO("<I> report rate invalid\n");
		return -1;
	}

	ret = cts_if->set_smooth_lv_set(cts_dev, cmd);
	//mutex_unlock(&chip_info->touch_mutex);
	return ret;
}


static int cts_diaphragm_touch_lv_set(void *chip_data, int level)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
	int ret = -1;
	uint8_t cmd = 1;
	//mutex_lock(&chip_info->touch_mutex);
	TPD_INFO("diaphragm mode, 0x%x(level)\n", level);	
	if (cts_data == NULL) {
		TPD_INFO("<E> cts_data = NULL!\n");
		return -1;
	}

	switch (level) {
        case 0x00: /*关闭*/
            cmd = 0x00;
            break;
		case 0x01:	/* 钢化膜 */
			cmd = 0x01;
			break;
		case 0x02:	/* 防水袋 */
			cmd = 0x02;
            break;
        case 0x03:	/* 全部开启 */
			cmd = 0x03;
            break;
		default:
			TPD_INFO("<I> report rate invalid\n");
		return -1;
	}

	ret = cts_if->set_diaphragm_lv_set(cts_dev, cmd);
	//mutex_unlock(&chip_info->touch_mutex);
	return ret;
}

static void cts_force_water_mode(void *chip_data, bool enable)
{
    TPD_INFO("%s: %s ws force_water mode is not supported.\n", __func__, enable ? "Enter" : "Exit");
}

static void cts_read_water_flag(void *chip_data)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_interface *cts_if = cts_dev->cts_if;
    struct touchpanel_data *ts =spi_get_drvdata(cts_data->spi_client);
    int ret = -1;
    //uint8_t cmd = 1;
    u8 buf[1];
        
    if (cts_data == NULL) {
        TPD_INFO("<E> water cts_data = NULL!\n");
    }
    cts_lock_device(cts_dev);
    ret = cts_if->get_water_flag(cts_dev, buf);
    if (!ret) {
        TPD_INFO("<E> read_water_flag success!\n");
        ts->water_mode = buf[0];
        TPD_INFO("<E> get_water_flag: %d\n", ts->water_mode);
    }
    cts_unlock_device(cts_dev);
}

static struct oplus_touchpanel_operations cts_tp_ops = {
    .get_chip_info						= cts_get_chip_info,
    .mode_switch						= cts_mode_switch,
    .get_touch_points					= cts_get_touch_points,
    .get_gesture_info					= cts_get_gesture_info,
    .ftm_process						= cts_ftm_process,
    .reset								= cts_reset,
    .fw_check							= cts_fw_check,
    .fw_update							= cts_fw_update,
    .trigger_reason						= cts_trigger_reason,
    .esd_handle							= cts_esd_handle,
    .set_touch_direction				= cts_set_touch_direction,
    .get_touch_direction				= cts_get_touch_direction,
    .power_control						= cts_power_control,
    .smooth_lv_set						= cts_smooth_lv_set,
    .sensitive_lv_set					= cts_sensitive_lv_set,
    .rate_white_list_ctrl				= cts_rate_white_list_ctrl,
    .diaphragm_touch_lv_set             = cts_diaphragm_touch_lv_set,
    .get_water_mode                     = cts_read_water_flag,
    .force_water_mode                   = cts_force_water_mode,
};

static void cts_baseline_read(struct seq_file *s, void *chip_data)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    cts_dev->cts_if->get_data_for_oplus(s, chip_data, true);
}

static void cts_delta_read(struct seq_file *s, void *chip_data)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    cts_dev->cts_if->get_data_for_oplus(s, chip_data, false);
}

static struct debug_info_proc_operations debug_info_proc_ops = {
//    .limit_read         = cts_limit_read,
    .baseline_read      = cts_baseline_read,
    .delta_read         = cts_delta_read,
    .main_register_read = NULL,
};
/*
static int cts_update_headfile_fw(void *chip_data, struct panel_info *panel_data)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)chip_data;
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_firmware cts_firmware;
    int ret;

    TPD_INFO("<I> Enter %s!\n", __func__);

    cts_firmware.name = panel_data->fw_name;
    cts_firmware.hwid = cts_dev->hwdata->hwid;
    cts_firmware.fwid = cts_dev->hwdata->fwid;
    cts_firmware.data = (u8 *)panel_data->firmware_headfile.firmware_data;
    cts_firmware.size = panel_data->firmware_headfile.firmware_size;

    TPD_INFO("<I> Fw name:%s!\n", cts_firmware.name);
    TPD_INFO("<I> Fw size:%zu!\n", cts_firmware.size);

    cts_lock_device(cts_dev);
    ret = cts_update_firmware(cts_dev, &cts_firmware, false);
    cts_unlock_device(cts_dev);
    if (ret < 0) {
        return -1;
    }

    cts_read_fw_ddi_version(chip_data, tsdata);

    return 0;
}
*/

static int cts_gstr_rawdata_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
    struct cts_interface *cts_if = chip_info->cts_dev.cts_if;

	chip_info->p_cts_test_para->test_work_mode = 1;

	return cts_if->gesture_rawdata_test_item(chip_info, cts_testdata);
}

static int cts_gstr_noise_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
	struct cts_interface *cts_if = chip_info->cts_dev.cts_if;

	chip_info->p_cts_test_para->test_work_mode = 1;
	
	return cts_if->gesture_noise_test_item(chip_info, cts_testdata);
}

static int cts_gstr_lp_rawdata_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
    struct cts_interface *cts_if = chip_info->cts_dev.cts_if;

	chip_info->p_cts_test_para->test_work_mode = 0;

	return cts_if->gesture_rawdata_test_item(chip_info, cts_testdata);
}

static int cts_gstr_lp_noise_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
	struct cts_interface *cts_if = chip_info->cts_dev.cts_if;

	chip_info->p_cts_test_para->test_work_mode = 0;
	
	return cts_if->gesture_noise_test_item(chip_info, cts_testdata);
}

static int cts_black_screen_test_preoperation(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	int ret = -1;
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
    struct cts_interface *cts_if = chip_info->cts_dev.cts_if;
	struct auto_test_header *ph = NULL;

	uint32_t *p_item_offset = NULL;
	int item_cnt = 0;
	struct auto_test_item_header *item_head = NULL;
	int m = 0;
	int i = 0;
	uint8_t data_buf[128];

    TPD_INFO("<I> Enter %s!\n", __func__);
	
	TPD_INFO("panel_data.test_limit_name - %s \n", tsdata->panel_data.test_limit_name);
	ret = request_firmware(&tsdata->com_test_data.limit_fw,
			       tsdata->panel_data.test_limit_name, tsdata->dev);
	if (ret < 0) {
		TPD_INFO("<E> Request firmware failed - %s (%d)\n", tsdata->panel_data.test_limit_name,
			 ret);
		return 0;
	}

	TPD_INFO("<I> %s: malloc cts_test_para \n", __func__);
	chip_info->p_cts_test_para = NULL;
    chip_info->p_cts_test_para = kzalloc(sizeof(struct cts_autotest_para), GFP_KERNEL);
    if (chip_info->p_cts_test_para == NULL) {
        TPD_INFO("<E> Alloc chip_info->p_cts_test_para failed");
		goto release_data;
    }
	/*
	chip_info->p_cts_test_para = tp_devm_kzalloc(&chip_info->spi_client->dev,
				     sizeof(struct cts_autotest_para), GFP_KERNEL);

	if (!chip_info->p_cts_test_para) {
		goto release_data;
	}*/

	TPD_INFO("<I> %s: malloc cts_autotest_offset \n", __func__);
	chip_info->p_cts_autotest_offset = NULL;
    chip_info->p_cts_autotest_offset = kzalloc(sizeof(struct cts_autotest_offset), GFP_KERNEL);
    if (chip_info->p_cts_autotest_offset == NULL) {
        TPD_INFO("<E> Alloc chip_info->p_cts_autotest_offset failed");
		goto release_data;
    }
	/*chip_info->p_cts_autotest_offset = tp_devm_kzalloc(&chip_info->spi_client->dev,
					   sizeof(struct cts_autotest_offset), GFP_KERNEL);

	if (!chip_info->p_cts_autotest_offset) {
		goto release_data;
	}*/

	ph = (struct auto_test_header *)(tsdata->com_test_data.limit_fw->data);

	p_item_offset = (uint32_t *)(tsdata->com_test_data.limit_fw->data + 16);

	for (i = 0; i < 8 * sizeof(ph->test_item); i++) {
		if ((ph->test_item >> i) & 0x01) {
			item_cnt++;
		}
	}

	memset(data_buf, 0, sizeof(data_buf));
#ifndef CONFIG_REMOVE_OPLUS_FUNCTION
	snprintf(data_buf, 128, "FW Version Name:%s\nTotal test item = %d\n",
			 tsdata->panel_data.manufacture_info.version, item_cnt);
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf,
		      strlen(data_buf), cts_testdata->pos);
#endif

	TPD_INFO("<I> %s: total test item: %d \n", __func__, item_cnt);

	for (m = 0; m < item_cnt; m++) {
		item_head = (struct auto_test_item_header *)(tsdata->com_test_data.limit_fw->data +
				p_item_offset[m]);

		if (item_head->item_limit_type == LIMIT_TYPE_NO_DATA) {
			TPD_INFO("<E> [%d] incorrect item type: LIMIT_TYPE_NO_DATA\n", item_head->item_bit);

		} else if (item_head->item_limit_type == LIMIT_TYPE_CERTAIN_DATA) {
			TPD_INFO("test item bit [%d] \n", item_head->item_bit);

			if (item_head->item_bit == TYPE_GSTR_RAWDATA) {
				chip_info->p_cts_autotest_offset->cts_gstr_rawdata_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_gstr_rawdata_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_gstr_rawdata = item_head->item_limit_type;
			} else if (item_head->item_bit == TYPE_GSTR_NOISE) {
				chip_info->p_cts_autotest_offset->cts_gstr_noise_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_gstr_noise_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_gstr_noise = item_head->item_limit_type;
			} else if (item_head->item_bit == TYPE_GSTR_LP_RAWDATA) {
				chip_info->p_cts_autotest_offset->cts_gstr_lp_rawdata_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_gstr_lp_rawdata_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_gstr_lp_rawdata = item_head->item_limit_type;
			} else if (item_head->item_bit == TYPE_GSTR_LP_NOISE) {
				chip_info->p_cts_autotest_offset->cts_gstr_lp_noise_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_gstr_lp_noise_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_gstr_lp_noise = item_head->item_limit_type;
			}
		} else {
			TPD_INFO("<E> [%d] unknown item type \n", item_head->item_bit);
		}
	}

	TPD_INFO("<I> %s: fill cts_test_para \n", __func__);
	chip_info->p_cts_test_para->test_rawdata_min				= CTS_TEST_RAWDATA_TH_MIN;
	chip_info->p_cts_test_para->test_rawdata_max		        = CTS_TEST_RAWDATA_TH_MAX;
	chip_info->p_cts_test_para->test_rawdata_frames				= CTS_TEST_RAWDATA_FRAMES;
	chip_info->p_cts_test_para->test_noise_min		            = CTS_TEST_NOISE_TH_MIN;
	chip_info->p_cts_test_para->test_noise_max					= CTS_TEST_NOISE_TH_MAX;
	chip_info->p_cts_test_para->test_noise_frames				= CTS_TEST_NOISE_FRAMES;
	chip_info->p_cts_test_para->test_open_min					= CTS_TEST_OPEN_TH_MIN;
	chip_info->p_cts_test_para->test_open_max					= CTS_TEST_OPEN_TH_MAX;
	chip_info->p_cts_test_para->test_short_min					= CTS_TEST_SHORT_TH_MIN;
	chip_info->p_cts_test_para->test_short_max					= CTS_TEST_SHORT_TH_MAX;
	chip_info->p_cts_test_para->test_comp_cap_min				= CTS_TEST_COMP_CAP_TH_MIN;
	chip_info->p_cts_test_para->test_comp_cap_max				= CTS_TEST_COMP_CAP_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_rawdata_min           = CTS_TEST_GSTR_RAWDATA_TH_MIN;
	chip_info->p_cts_test_para->test_gstr_rawdata_max           = CTS_TEST_GSTR_RAWDATA_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_rawdata_frames        = CTS_TEST_GSTR_RAWDATA_FRAMES;
	chip_info->p_cts_test_para->test_gstr_lp_rawdata_min        = CTS_TEST_GSTR_LP_RAWDATA_TH_MIN;
	chip_info->p_cts_test_para->test_gstr_lp_rawdata_max        = CTS_TEST_GSTR_LP_RAWDATA_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_lp_rawdata_frames     = CTS_TEST_GSTR_LP_RAWDATA_FRAMES;
	chip_info->p_cts_test_para->test_gstr_noise_min      	    = CTS_TEST_GSTR_NOISE_TH_MIN;
	chip_info->p_cts_test_para->test_gstr_noise_max 	        = CTS_TEST_GSTR_NOISE_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_noise_frames  	    = CTS_TEST_GSTR_NOISE_FRAMES;
	chip_info->p_cts_test_para->test_gstr_lp_noise_min     	    = CTS_TEST_GSTR_LP_NOISE_TH_MIN;
	chip_info->p_cts_test_para->test_gstr_lp_noise_max 	        = CTS_TEST_GSTR_LP_NOISE_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_lp_noise_frames  	    = CTS_TEST_GSTR_LP_NOISE_FRAMES;

    ret = cts_if->prepare_black_test(&chip_info->cts_dev);
	if (ret) {
		TPD_INFO("<E> Prepare black test failed\n");
		goto release_data;
	}

    TPD_INFO("<I> End %s!\n", __func__);
	return 0;

release_data:
	if (chip_info->p_cts_test_para)
	    kfree(chip_info->p_cts_test_para);
	if (chip_info->p_cts_autotest_offset)
	    kfree(chip_info->p_cts_autotest_offset);
	/*tp_devm_kfree(&chip_info->spi_client->dev, (void **)&chip_info->p_cts_autotest_offset,
		sizeof(struct cts_autotest_offset));
	tp_devm_kfree(&chip_info->spi_client->dev, (void **)&chip_info->p_cts_test_para,
		sizeof(struct cts_autotest_para));*/

	TPD_INFO("<E> %s failed! \n", __func__);
	return -1;
}

static int cts_black_screen_test_endoperation(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;

    TPD_INFO("<I> Enter %s!\n", __func__);
	if (chip_info->p_cts_test_para)
	    kfree(chip_info->p_cts_test_para);
	if (chip_info->p_cts_autotest_offset)
	    kfree(chip_info->p_cts_autotest_offset);
	/*tp_devm_kfree(&chip_info->spi_client->dev,
		      (void **)&chip_info->p_cts_autotest_offset, sizeof(struct cts_autotest_offset));
	tp_devm_kfree(&chip_info->spi_client->dev, (void **)&chip_info->p_cts_test_para,
		      sizeof(struct cts_autotest_para));*/

	if (tsdata->com_test_data.limit_fw) {
		release_firmware(tsdata->com_test_data.limit_fw);
		tsdata->com_test_data.limit_fw = NULL;
	}

    TPD_INFO("<I> End %s!\n", __func__);

	return 0;
}

static int cts_reset_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
    struct cts_interface *cts_if = chip_info->cts_dev.cts_if;

	return cts_if->reset_pin_test_item(chip_info, cts_testdata);
}
/*
static int cts_int_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
    struct cts_interface *cts_if = chip_info->cts_dev.cts_if;

	return cts_if->int_pin_test_item(chip_info, cts_testdata);
}
*/
static int cts_rawdata_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
    struct cts_interface *cts_if = chip_info->cts_dev.cts_if;

	return cts_if->rawdata_test_item(chip_info, cts_testdata);
}

static int cts_noise_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
	struct cts_interface *cts_if = chip_info->cts_dev.cts_if;
	
	return cts_if->noise_test_item(chip_info, cts_testdata);
}

static int cts_open_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
	struct cts_interface *cts_if = chip_info->cts_dev.cts_if;
	
	return cts_if->open_test_item(chip_info, cts_testdata);
}

static int cts_short_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
	struct cts_interface *cts_if = chip_info->cts_dev.cts_if;
	
	return cts_if->short_test_item(chip_info, cts_testdata);
}

static int cts_comp_cap_test(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
	struct cts_interface *cts_if = chip_info->cts_dev.cts_if;
	
	return cts_if->compensate_cap_test_item(chip_info, cts_testdata);
}

static int cts_autotest_preoperation(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	int ret = -1;
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;
	struct auto_test_header *ph = NULL;

	uint32_t *p_item_offset = NULL;
	int item_cnt = 0;
	struct auto_test_item_header *item_head = NULL;
	int m = 0;
	int i = 0;
	uint8_t data_buf[128];

    TPD_INFO("<I> Enter %s!\n", __func__);
	
	TPD_INFO("panel_data.test_limit_name - %s \n", tsdata->panel_data.test_limit_name);
	ret = request_firmware(&tsdata->com_test_data.limit_fw,
			       tsdata->panel_data.test_limit_name, tsdata->dev);
	if (ret < 0) {
		TPD_INFO("<E> Request firmware failed - %s (%d)\n", tsdata->panel_data.test_limit_name,
			 ret);
		return 0;
	}

	TPD_INFO("<I> %s: malloc cts_test_para \n", __func__);
	chip_info->p_cts_test_para = NULL;
	chip_info->p_cts_test_para = tp_devm_kzalloc(&chip_info->spi_client->dev,
				     sizeof(struct cts_autotest_para), GFP_KERNEL);

	if (!chip_info->p_cts_test_para) {
		goto release_data;
	}

	TPD_INFO("<I> %s: malloc cts_autotest_offset \n", __func__);
	chip_info->p_cts_autotest_offset = NULL;
	chip_info->p_cts_autotest_offset = tp_devm_kzalloc(&chip_info->spi_client->dev,
					   sizeof(struct cts_autotest_offset), GFP_KERNEL);

	if (!chip_info->p_cts_autotest_offset) {
		goto release_data;
	}

	ph = (struct auto_test_header *)(tsdata->com_test_data.limit_fw->data);

	p_item_offset = (uint32_t *)(tsdata->com_test_data.limit_fw->data + 16);

	TPD_INFO("@@ item offset1 num: 0x%x\n", p_item_offset[0]);
	TPD_INFO("@@ item offset2 num: 0x%x\n", p_item_offset[1]);
	TPD_INFO("@@ item offset3 num: 0x%x\n", p_item_offset[2]);
	TPD_INFO("@@ item offset4 num: 0x%x\n", p_item_offset[3]);
	TPD_INFO("@@ item offset5 num: 0x%x\n", p_item_offset[4]);
	TPD_INFO("@@ item offset6 num: 0x%x\n", p_item_offset[5]);
	TPD_INFO("@@ item offset7 num: 0x%x\n", p_item_offset[6]);
	TPD_INFO("@@ item offset8 num: 0x%x\n", p_item_offset[7]);
	TPD_INFO("@@ item offset9 num: 0x%x\n", p_item_offset[8]);
	TPD_INFO("@@ item offset10 num: 0x%x\n", p_item_offset[9]);
	TPD_INFO("@@ item offset11 num: 0x%x\n", p_item_offset[10]);

	for (i = 0; i < 8 * sizeof(ph->test_item); i++) {
		if ((ph->test_item >> i) & 0x01) {
			item_cnt++;
		}
	}

	memset(data_buf, 0, sizeof(data_buf));
#ifndef CONFIG_REMOVE_OPLUS_FUNCTION
	snprintf(data_buf, 128, "FW Version Name:%s\nTotal test item = %d\n",
			 tsdata->panel_data.manufacture_info.version, item_cnt);
	tp_test_write(cts_testdata->fp, cts_testdata->length, data_buf,
		      strlen(data_buf), cts_testdata->pos);
#endif

	TPD_INFO("<I> %s: total test item: %d \n", __func__, item_cnt);

	for (m = 0; m < item_cnt; m++) {
		item_head = (struct auto_test_item_header *)(tsdata->com_test_data.limit_fw->data +
				p_item_offset[m]);

		if (item_head->item_limit_type == LIMIT_TYPE_NO_DATA) {
			TPD_INFO("<E> [%d] incorrect item type: LIMIT_TYPE_NO_DATA\n", item_head->item_bit);

		} else if (item_head->item_limit_type == LIMIT_TYPE_CERTAIN_DATA) {
			TPD_INFO("test item bit [%d] \n", item_head->item_bit);

			if (item_head->item_bit == TYPE_RAWDATA) {
				chip_info->p_cts_autotest_offset->cts_rawdata_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_rawdata_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_rawdata = item_head->item_limit_type;
			} else if (item_head->item_bit == TYPE_NOISE) {
				chip_info->p_cts_autotest_offset->cts_noise_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_noise_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_noise = item_head->item_limit_type;
			} else if (item_head->item_bit == TYPE_OPEN) {
				chip_info->p_cts_autotest_offset->cts_open_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_open_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_open = item_head->item_limit_type;
			} else if (item_head->item_bit == TYPE_SHORT) {
				chip_info->p_cts_autotest_offset->cts_short_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_short_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_short = item_head->item_limit_type;
			} else if (item_head->item_bit == TYPE_COMP_CAP) {
				chip_info->p_cts_autotest_offset->cts_comp_cap_min = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->floor_limit_offset);
				chip_info->p_cts_autotest_offset->cts_comp_cap_max = (int32_t *)(
							tsdata->com_test_data.limit_fw->data + item_head->top_limit_offset);
				chip_info->p_cts_test_para->limit_type_comp_cap = item_head->item_limit_type;
			}
		} else {
			TPD_INFO("<E> [%d] unknown item type \n", item_head->item_bit);
		}
	}

	TPD_INFO("<I> %s: fill cts_test_para \n", __func__);
	chip_info->p_cts_test_para->test_rawdata_min				= CTS_TEST_RAWDATA_TH_MIN;
	chip_info->p_cts_test_para->test_rawdata_max		        = CTS_TEST_RAWDATA_TH_MAX;
	chip_info->p_cts_test_para->test_rawdata_frames				= CTS_TEST_RAWDATA_FRAMES;
	chip_info->p_cts_test_para->test_noise_min		            = CTS_TEST_NOISE_TH_MIN;
	chip_info->p_cts_test_para->test_noise_max					= CTS_TEST_NOISE_TH_MAX;
	chip_info->p_cts_test_para->test_noise_frames				= CTS_TEST_NOISE_FRAMES;
	chip_info->p_cts_test_para->test_open_min					= CTS_TEST_OPEN_TH_MIN;
	chip_info->p_cts_test_para->test_open_max					= CTS_TEST_OPEN_TH_MAX;
	chip_info->p_cts_test_para->test_short_min					= CTS_TEST_SHORT_TH_MIN;
	chip_info->p_cts_test_para->test_short_max					= CTS_TEST_SHORT_TH_MAX;
	chip_info->p_cts_test_para->test_comp_cap_min				= CTS_TEST_COMP_CAP_TH_MIN;
	chip_info->p_cts_test_para->test_comp_cap_max				= CTS_TEST_COMP_CAP_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_rawdata_min           = CTS_TEST_GSTR_RAWDATA_TH_MIN;
	chip_info->p_cts_test_para->test_gstr_rawdata_max           = CTS_TEST_GSTR_RAWDATA_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_rawdata_frames        = CTS_TEST_GSTR_RAWDATA_FRAMES;
	chip_info->p_cts_test_para->test_gstr_lp_rawdata_min        = CTS_TEST_GSTR_LP_RAWDATA_TH_MIN;
	chip_info->p_cts_test_para->test_gstr_lp_rawdata_max        = CTS_TEST_GSTR_LP_RAWDATA_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_lp_rawdata_frames     = CTS_TEST_GSTR_LP_RAWDATA_FRAMES;
	chip_info->p_cts_test_para->test_gstr_noise_min      	    = CTS_TEST_GSTR_NOISE_TH_MIN;
	chip_info->p_cts_test_para->test_gstr_noise_max 	        = CTS_TEST_GSTR_NOISE_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_noise_frames  	    = CTS_TEST_GSTR_NOISE_FRAMES;
	chip_info->p_cts_test_para->test_gstr_lp_noise_min     	    = CTS_TEST_GSTR_LP_NOISE_TH_MIN;
	chip_info->p_cts_test_para->test_gstr_lp_noise_max 	        = CTS_TEST_GSTR_LP_NOISE_TH_MAX;
	chip_info->p_cts_test_para->test_gstr_lp_noise_frames  	    = CTS_TEST_GSTR_LP_NOISE_FRAMES;

    TPD_INFO("<I> End %s!\n", __func__);
	return 0;

release_data:
	tp_devm_kfree(&chip_info->spi_client->dev, (void **)&chip_info->p_cts_autotest_offset,
		sizeof(struct cts_autotest_offset));
	tp_devm_kfree(&chip_info->spi_client->dev, (void **)&chip_info->p_cts_test_para,
		sizeof(struct cts_autotest_para));

	TPD_INFO("<E> %s failed! \n", __func__);
	return -1;
}

static int cts_autotest_endoperation(struct seq_file *s, void *chip_data,
	struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info)
{
	struct chipone_ts_data *chip_info = (struct chipone_ts_data *)chip_data;

    TPD_INFO("<I> Enter %s!\n", __func__);
	tp_devm_kfree(&chip_info->spi_client->dev,
		      (void **)&chip_info->p_cts_autotest_offset, sizeof(struct cts_autotest_offset));
	tp_devm_kfree(&chip_info->spi_client->dev, (void **)&chip_info->p_cts_test_para,
		      sizeof(struct cts_autotest_para));

	if (tsdata->com_test_data.limit_fw) {
		release_firmware(tsdata->com_test_data.limit_fw);
		tsdata->com_test_data.limit_fw = NULL;
	}

    TPD_INFO("<I> End %s!\n", __func__);

	return 0;
}

static struct cts_auto_test_operations cts_test_ops = {
	.auto_test_preoperation  = cts_autotest_preoperation,
	.test1                   = cts_reset_test,
	//.test2                   = cts_int_test,
	.test2                   = NULL,
	.test3                   = cts_rawdata_test,
	.test4                   = cts_noise_test,
	.test5                   = cts_open_test,
	.test6                   = cts_short_test,
	.test7                   = cts_comp_cap_test,
	.auto_test_endoperation  = cts_autotest_endoperation,

	.black_screen_test_preoperation	= cts_black_screen_test_preoperation,
	.black_screen_test1				= cts_gstr_rawdata_test,
	.black_screen_test2				= cts_gstr_lp_rawdata_test,
	.black_screen_test3				= cts_gstr_noise_test,
	.black_screen_test4				= cts_gstr_lp_noise_test,
	.black_screen_test_endoperation	= cts_black_screen_test_endoperation,
};

struct engineer_test_operations cts_engineer_test_ops = {
	.auto_test                  = cts_auto_test,
	.black_screen_test          = cts_black_screen_autotest,
};


#ifdef CONFIG_CTS_I2C_HOST
static int cts_driver_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
#else
static int cts_driver_probe(struct spi_device *client)
#endif
{
    int ret = 0;
	struct chipone_ts_data *cts_data = NULL;

    TPD_INFO("<I> Enter cts_driver_probe\n");

#ifdef CONFIG_CTS_I2C_HOST
    TPD_INFO("<I> Probe i2c client: name='%s' addr=0x%02x flags=0x%02x irq=%d\n",
        client->name, client->addr, client->flags, client->irq);

#if !defined(CONFIG_MTK_PLATFORM)
    if (client->addr != CTS_DEV_NORMAL_MODE_I2CADDR) {
        TPD_INFO("<E> Probe i2c addr 0x%02x != driver config addr 0x%02x\n",
            client->addr, CTS_DEV_NORMAL_MODE_I2CADDR);
        return -ENODEV;
    };
#endif

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        TPD_INFO("<E> Check functionality failed\n");
        return -ENODEV;
    }
#endif

    cts_data = (struct chipone_ts_data *)kzalloc(sizeof(struct chipone_ts_data), GFP_KERNEL);
    if (cts_data == NULL) {
        TPD_INFO("<E> Allocate chipone_ts_data failed\n");
        return -ENOMEM;
    }

    cts_data->pdata = (struct cts_platform_data *)kzalloc(
            sizeof(struct cts_platform_data), GFP_KERNEL);
    if (cts_data->pdata == NULL) {
        TPD_INFO("<E> Allocate cts_platform_data failed\n");
        ret = -ENOMEM;
        goto err_free_cts_data;
    }

#ifdef CONFIG_CTS_I2C_HOST
    cts_data->i2c_client = client;
    cts_data->device = &client->dev;
#else
    cts_data->spi_client = client;
    cts_data->device = &client->dev;
#endif

    ret = cts_init_platform_data(cts_data->pdata, client);
    if (ret) {
        TPD_INFO("<E> cts_init_platform_data err\n");
        goto err_free_pdata;
    }

    cts_data->cts_dev.pdata = cts_data->pdata;
    cts_data->pdata->cts_dev = &cts_data->cts_dev;

    tsdata = common_touch_data_alloc();
    if (tsdata == NULL) {
        TPD_INFO("<E> ts kzalloc error\n");
        goto err_free_pdata;
    }

    cts_data->vfw.data = vmalloc(120 * 1024);
    if (!cts_data->vfw.data) {
        TPD_INFO("<E> Alloc vfw.data failed\n");
        goto err_free_tsdata;
    }

    tsdata->s_client = cts_data->spi_client;
    tsdata->irq = cts_data->pdata->irq;
    tsdata->dev = &cts_data->spi_client->dev;
    tsdata->chip_data = cts_data;
    tsdata->ts_ops = &cts_tp_ops;
    tsdata->debug_info_ops = &debug_info_proc_ops;
	tsdata->engineer_ops = &cts_engineer_test_ops;
	tsdata->com_test_data.chip_test_ops = &cts_test_ops;

    cts_data->tsdata = tsdata;

#ifdef CONFIG_CTS_I2C_HOST
    i2c_set_clientdata(client, cts_data->tsdata);
#else
    spi_set_drvdata(client, cts_data->tsdata);
#endif

	/* init para */
	TPD_INFO("<I> Init cts_interface");
	cts_data->cts_dev.cts_if = &tcs_if;
	if (cts_init_trans_buf(&cts_data->cts_dev)) {
		TPD_INFO("<E> Init tbuf/rbuf failed!");
		goto err_free_vfw_data;
	}
    ret = register_common_touch_device(tsdata);
    if (ret < 0) {
        TPD_INFO("<E> register touch device failed: ret=%d\n", ret);
        goto err_free_common_touch;
    }

    disable_irq_nosync(tsdata->irq);

    cts_data->pdata->rst_gpio = tsdata->hw_res.reset_gpio;

    ret = cts_plat_reset_device(cts_data->pdata);
    if (ret < 0) {
        TPD_INFO("<E> Reset device failed %d\n", ret);
    }

    ret = cts_probe_device(&cts_data->cts_dev);
    if (ret) {
        TPD_INFO("<E> Probe device failed %d\n", ret);
        goto err_free_common_touch;
    }
/*
    ret = cts_update_headfile_fw(tsdata->chip_data, &tsdata->panel_data);
    if (ret) {
        TPD_INFO("<E> update head firmware failed\n");
        //goto err_free_common_touch;
    }
*/
    enable_irq(tsdata->irq);

    ret = cts_tool_init(cts_data);
    if (ret) {
        TPD_INFO("<W> Init tool node failed %d\n", ret);
        //goto err_free_common_touch;
    }

    ret = cts_sysfs_add_device(&client->dev);
    if (ret) {
        TPD_INFO("<W> Add sysfs entry for device failed %d\n", ret);
        //goto err_tool_init;
    }

    return 0;
/**
THIS PART WILL NEVER RUN, IC ID MUST BE DETECTED
err_sysfs_add_device:
    cts_sysfs_remove_device(&client->dev);
err_tool_init:
    cts_tool_deinit(cts_data);*/
    
err_free_common_touch:
	cts_deinit_trans_buf(&cts_data->cts_dev);

err_free_vfw_data:
    tsdata->s_client = NULL;
    spi_set_drvdata(client, NULL);
    vfree(cts_data->vfw.data);
    cts_data->vfw.data = NULL;
err_free_tsdata:
    common_touch_data_free(tsdata);
    tsdata = NULL;
err_free_pdata:
    kfree(cts_data->pdata);
    cts_data->pdata = NULL;
err_free_cts_data:
    kfree(cts_data);
    cts_data = NULL;

    TPD_INFO("<E> Probe failed %d\n", ret);

    // If return ret, kernel will be crashed.
    //return ret;
    return 0;
}

#ifdef CONFIG_CTS_I2C_HOST
static int cts_driver_remove(struct i2c_client *client)
#else
static int cts_driver_remove(struct spi_device *client)
#endif
{
    struct chipone_ts_data *cts_data;

    TPD_INFO("<I> Remove\n");

#ifdef CONFIG_CTS_I2C_HOST
    cts_data = (struct chipone_ts_data *)i2c_get_clientdata(client);
#else
    cts_data = (struct chipone_ts_data *)spi_get_drvdata(client);
#endif
    if (cts_data) {
        cts_sysfs_remove_device(&client->dev);

        cts_tool_deinit(cts_data);

        tsdata->s_client = NULL;
        spi_set_drvdata(client, NULL);

        if (cts_data->vfw.data) {
            vfree(cts_data->vfw.data);
            cts_data->vfw.data = NULL;
        }

        if (tsdata) {
            common_touch_data_free(tsdata);
            tsdata = NULL;
        }

        if (cts_data->pdata) {
            kfree(cts_data->pdata);
            cts_data->pdata = NULL;
        }

        kfree(cts_data);
        cts_data = NULL;
    } else {
        TPD_INFO("<W> Chipone i2c driver remove while NULL chipone_ts_data\n");
        return -EINVAL;
    }

    return 0;
}


static const struct of_device_id cts_tp_match_table[] = {
#ifdef CONFIG_TOUCHPANEL_MULTI_NOFLASH
	{ .compatible = "oplus,tp_noflash", },
#else
	{ .compatible = TPD_DEVICE, },
#endif
	{ },
};

static const struct spi_device_id tp_id[] = {
#ifdef CONFIG_TOUCHPANEL_MULTI_NOFLASH
	{ "oplus,tp_noflash", 0 },
#else
	{ TPD_DEVICE, 0 },
#endif
	{ },
};


static int chipone_tpd_suspend(struct device *dev)
{
    struct touchpanel_data *ts = dev_get_drvdata(dev);
    TPD_INFO("<I> tp_i2c_suspend\n");
    tp_pm_suspend(ts);
    return 0;
}

static int chipone_tpd_resume(struct device *dev)
{
    struct touchpanel_data *ts = dev_get_drvdata(dev);
    TPD_INFO("<I> tp_i2c_resume\n");
    tp_pm_resume(ts);
    return 0;
}

static const struct dev_pm_ops tp_pm_ops = {
    .suspend = chipone_tpd_suspend,
    .resume = chipone_tpd_resume,
};

static struct spi_driver cts_spi_driver = {

    .probe = cts_driver_probe,
    .remove = cts_driver_remove,
    .id_table = tp_id,
    .driver = {
        .name = TPD_DEVICE,
        .of_match_table = cts_tp_match_table,
        .pm = &tp_pm_ops,
    },
    
};
 
static int __init cts_driver_init(void)
{
    TPD_INFO("<I> Chipone-tddi driver %s\n", CFG_CTS_DRIVER_VERSION);

    /* If matched, means got ic id */
    if (!tp_judge_ic_match(TPD_DEVICE)) {
        TPD_INFO("<E> %s mismatched\n", TPD_DEVICE);
        return 0;
    }

    TPD_INFO("<I> %s matched\n", TPD_DEVICE);

#ifndef CONFIG_REMOVE_OPLUS_FUNCTION
	get_oem_verified_boot_state();
#endif
	if (spi_register_driver(&cts_spi_driver) != 0) {
		TPD_INFO("unable to add spi driver.\n");
		return 0;
	}

	return 0;
}

static void __exit cts_driver_exit(void)
{
    TPD_INFO("<I> Chipone-tddi driver exit\n");

#ifdef CONFIG_CTS_I2C_HOST
    i2c_del_driver(&cts_i2c_driver);
#else
    spi_unregister_driver(&cts_spi_driver);
#endif
}

module_init(cts_driver_init);
module_exit(cts_driver_exit);

MODULE_DESCRIPTION("Chipone-tddi Driver for MTK platform");
MODULE_VERSION(CFG_CTS_DRIVER_VERSION);
MODULE_AUTHOR("Cai Yang <ycai@chiponeic.com>");
MODULE_LICENSE("GPL");
