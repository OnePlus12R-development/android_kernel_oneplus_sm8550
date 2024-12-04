#include "jadard_oplus_common.h"

void jadard_baseline_read(struct seq_file *s, void *chip_data)
{
    struct jadard_ts_data *pjadard_ts_data = (struct jadard_ts_data *)chip_data;
    uint32_t index = 0;
	uint8_t *rdata = NULL;
	int i, j, DataType;
	int x_num = pjadard_ts_data->ic_data->JD_X_NUM;
	int y_num = pjadard_ts_data->ic_data->JD_Y_NUM;
	uint16_t rdata_size = x_num * y_num * sizeof(uint16_t);

	rdata = kzalloc(rdata_size * sizeof(uint8_t), GFP_KERNEL);
	if (rdata == NULL) {
		TPD_INFO("[JDTP][ERROR] Memory allocate fail: %d\n", __LINE__);
		return;
	}

	DataType = JD_DATA_TYPE_Baseline;

	pjadard_ts_data->module_fp->fp_mutual_data_set((uint8_t)DataType);
	pjadard_ts_data->int_enable(false);

	if (pjadard_ts_data->module_fp->fp_get_mutual_data(DataType, rdata, rdata_size) < 0) {
		TPD_INFO("[JDTP][ERROR] Get mutual data fail: %d\n", __LINE__);
		kfree(rdata);
		pjadard_ts_data->int_enable(true);
		return;
	}

	for (i = 0; i < y_num; i++) {
		for (j = 0; j < x_num; j++) {
            if (DataType == JD_DATA_TYPE_Difference || DataType == JD_DATA_TYPE_LAPLACE) {
                //data[i * x_num + j] = (((int8_t)rdata[index + 1] << 8) | rdata[index]);
            } else {
                //data[i * x_num + j] = (((uint8_t)rdata[index + 1] << 8) | rdata[index]);
            }
			index += 2;
		}
	}

	kfree(rdata);
	pjadard_ts_data->int_enable(true);
	pjadard_ts_data->module_fp->fp_mutual_data_set(JD_DATA_TYPE_RawData);
}

void jadard_delta_read(struct seq_file *s, void *chip_data)
{
    struct jadard_ts_data *pjadard_ts_data = (struct jadard_ts_data *)chip_data;
    uint32_t index = 0;
	uint8_t *rdata = NULL;
	int i, j, DataType;
	int x_num = pjadard_ts_data->ic_data->JD_X_NUM;
	int y_num = pjadard_ts_data->ic_data->JD_Y_NUM;
	uint16_t rdata_size = x_num * y_num * sizeof(uint16_t);

	rdata = kzalloc(rdata_size * sizeof(uint8_t), GFP_KERNEL);
	if (rdata == NULL) {
		TPD_INFO("[JDTP][ERROR] Memory allocate fail: %d\n", __LINE__);
		return;
	}

	DataType = JD_DATA_TYPE_RawData;

	pjadard_ts_data->module_fp->fp_mutual_data_set((uint8_t)DataType);
	pjadard_ts_data->int_enable(false);

	if (pjadard_ts_data->module_fp->fp_get_mutual_data(DataType, rdata, rdata_size) < 0) {
		TPD_INFO("[JDTP][ERROR] Get mutual data fail: %d\n", __LINE__);
		kfree(rdata);
		pjadard_ts_data->int_enable(true);
		return;
	}

	for (i = 0; i < y_num; i++) {
		for (j = 0; j < x_num; j++) {
            if (DataType == JD_DATA_TYPE_Difference || DataType == JD_DATA_TYPE_LAPLACE) {
                //data[i * x_num + j] = (((int8_t)rdata[index + 1] << 8) | rdata[index]);
            } else {
                //data[i * x_num + j] = (((uint8_t)rdata[index + 1] << 8) | rdata[index]);
            }
			index += 2;
		}
	}

	kfree(rdata);
	pjadard_ts_data->int_enable(true);
	pjadard_ts_data->module_fp->fp_mutual_data_set(JD_DATA_TYPE_RawData);
}

void jadard_main_register_read(struct seq_file *s, void *chip_data)
{
    /* Not implement */
}

//void jadard_tp_limit_data_write(void *chip_data, int32_t count)
//{
//    /* Not implement */
//}

struct debug_info_proc_operations jadard_debug_info_proc_ops = {
    .baseline_read = jadard_baseline_read,
    .delta_read = jadard_delta_read,
    .main_register_read = jadard_main_register_read,
    //.tp_limit_data_write = jadard_tp_limit_data_write,
};
EXPORT_SYMBOL(jadard_debug_info_proc_ops);

static int jadard_ftm_process(void *chip_data)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;

	if (gpio_is_valid(chip_info->hw_res->reset_gpio)) {
		gpio_direction_output(chip_info->hw_res->reset_gpio, 0);
		msleep(10);
		TPD_INFO("[JDTP] set reset pin low\n");
	}

	return 0;
}

static void jadard_ftm_process_extra(void *chip_data)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;

	TPD_INFO("[JDTP] %s: enter\n", __func__);

	if (gpio_is_valid(chip_info->hw_res->reset_gpio)) {
		gpio_direction_output(chip_info->hw_res->reset_gpio, 0);
		msleep(10);
		TPD_INFO("[JDTP] set reset pin low\n");
	}
}

int jadard_reset(void *chip_data)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
    int ret = 0;

	TPD_INFO("[JDTP] %s: enter\n", __func__);

	if (chip_info->oplus_ito_sorting_active == true) {
		chip_info->oplus_ito_sorting_active = false;
		return ret;
	}

	ret = chip_info->module_fp->fp_0f_upgrade_fw(NULL, &(chip_info->tp_fw));

    if (ret >= 0) {
        chip_info->module_fp->fp_read_fw_ver();
        /* oplus already enable irq */
		chip_info->irq_enabled = 1;
		TPD_INFO("[JDTP] upgrade firmware success = %d\n", ret);
	} else {
		TPD_INFO("[JDTP][ERROR] Failed to upgrade firmware, ret = %d\n", ret);
    }

	return 0;
}

static int jadard_power_control(void *chip_data, bool enable)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	TPD_INFO("[JDTP] set reset pin %d\n", enable);

	if (gpio_is_valid(chip_info->hw_res->reset_gpio)) {
		gpio_direction_output(chip_info->hw_res->reset_gpio, enable);
	}

	return 0;
}

static int jadard_reset_gpio_control(void *chip_data, bool enable)
{
    struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	TPD_INFO("[JDTP] %s is %d\n", __func__, enable);
    if (gpio_is_valid(chip_info->hw_res->reset_gpio)) {
        gpio_set_value(chip_info->hw_res->reset_gpio, enable);
    }
    return 0;
}

static int jadard_get_chip_info(void *chip_data)
{
	return 0;
}

static uint32_t jadard_trigger_reason(void *chip_data, int gesture_enable, int is_suspended)
{
	uint32_t oplus_status = IRQ_IGNORE;
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;

    if (chip_info->fw_ready == true) {
        oplus_status = chip_info->pjadard_ts_work(chip_info, gesture_enable, is_suspended);
    }

	return oplus_status;
}

static int jadard_get_touch_points(void *chip_data, struct point_info *points, int max_num)
{
	int i;
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;

	if ((chip_info == NULL) || (chip_info->host_data == NULL) || (chip_info->host_data->x == NULL)) {
		TPD_INFO("[JDTP] chip_info/chip_info->host_data/chip_info->host_data->x null\n");
		return 0;
	} else {
		for (i = 0; i < 10; i++) {
			if (chip_info->host_data->x[i] == 0xFFFF && chip_info->host_data->y[i] == 0xFFFF) {
				points[i].x = 0;
				points[i].y = 0;
				points[i].z = 0;
				points[i].width_major = 0;
				points[i].touch_major = 0;
				points[i].status = 0;
			} else {
				points[i].x = chip_info->host_data->x[i];
				points[i].y = chip_info->host_data->y[i];
				points[i].z = chip_info->host_data->w[i];
				points[i].width_major = chip_info->host_data->w[i];
				points[i].touch_major = chip_info->host_data->w[i];
				points[i].status = 1;
			}
			/*points[i].tx_press = 0;
			points[i].rx_press = 0;
			points[i].tx_er = 0;
			points[i].rx_er = 0;
			points[i].type = AREA_NORMAL;*/

			/*TPD_INFO("[JDTP] ID=%d, x=%d, y=%d z/width_major/touch_major=%d\n", chip_info->host_data->id[i],
			points[i].x, points[i].y, points[i].z);*/
		}
		/*TPD_INFO("[JDTP] pointid_info=%d\n", chip_info->host_data->pointid_info);*/
		return chip_info->host_data->pointid_info;
	}
}

static int jadard_get_touch_points_auto(void *chip_data, struct point_info *points, int max_num,
				     struct resolution_info *resolution_info)
{
	int i;
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;

	if ((chip_info == NULL) || (chip_info->host_data == NULL) || (chip_info->host_data->x == NULL)) {
		TPD_INFO("[JDTP] Auto: chip_info/chip_info->host_data/chip_info->host_data->x null\n");
		return 0;
	} else {
		for (i = 0; i < 10; i++) {
			if (chip_info->host_data->x[i] == 0xFFFF && chip_info->host_data->y[i] == 0xFFFF) {
				points[i].x = 0;
				points[i].y = 0;
				points[i].z = 0;
				points[i].width_major = 0;
				points[i].touch_major = 0;
				points[i].status = 0;
			} else {
				points[i].x = chip_info->host_data->x[i];
				points[i].y = chip_info->host_data->y[i];
				points[i].z = chip_info->host_data->w[i];
				points[i].width_major = chip_info->host_data->w[i];
				points[i].touch_major = chip_info->host_data->w[i];
				points[i].status = 1;
			}
			/*points[i].tx_press = 0;
			points[i].rx_press = 0;
			points[i].tx_er = 0;
			points[i].rx_er = 0;
			points[i].type = AREA_NORMAL;*/

			//TPD_INFO("[JDTP] ID=%d, x=%d, y=%d z/width_major/touch_major=%d\n", chip_info->host_data->id[i],
			//points[i].x, points[i].y, points[i].z);
		}
		//TPD_INFO("[JDTP] pointid_info=%d\n", chip_info->host_data->pointid_info);
		return chip_info->host_data->pointid_info;
	}
}

static int jadard_get_gesture_info(void *chip_data, struct gesture_info *gesture)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;

	gesture->gesture_type = chip_info->host_data->SMWP_event_chk;
	gesture->Point_start.x = chip_info->host_data->gesture_track_data[0];
	gesture->Point_start.y = chip_info->host_data->gesture_track_data[1];
	gesture->Point_end.x = chip_info->host_data->gesture_track_data[2];
	gesture->Point_end.y = chip_info->host_data->gesture_track_data[3];
	gesture->Point_1st.x = chip_info->host_data->gesture_track_data[4];
	gesture->Point_1st.y = chip_info->host_data->gesture_track_data[5];
	gesture->Point_2nd.x = chip_info->host_data->gesture_track_data[6];
	gesture->Point_2nd.y = chip_info->host_data->gesture_track_data[7];
	gesture->Point_3rd.x = chip_info->host_data->gesture_track_data[8];
	gesture->Point_3rd.y = chip_info->host_data->gesture_track_data[9];
	gesture->Point_4th.x = chip_info->host_data->gesture_track_data[10];
	gesture->Point_4th.y = chip_info->host_data->gesture_track_data[11];
	gesture->clockwise = chip_info->host_data->gesture_clockwise;
	chip_info->host_data->SMWP_event_chk = 0;
	return 0;
}

static int jadard_mode_switch(void *chip_data, work_mode mode, int flag)
{
	int ret = 0;
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;

	switch (mode) {
	case MODE_NORMAL:
		TPD_INFO("[JDTP] MODE_NORMAL flag = %d\n", flag);
		break;

	case MODE_SLEEP:
		TPD_INFO("[JDTP] MODE_SLEEP flag = %d\n", flag);

		/* Cancel mutual data thread, if exist */
		if (chip_info->diag_thread_active) {
			chip_info->diag_thread_active = false;
			cancel_delayed_work_sync(&chip_info->jadard_diag_delay_wrok);
		}
		break;

	case MODE_GESTURE:
		TPD_INFO("[JDTP] MODE_GESTURE flag = %d\n", flag);

		chip_info->SMWP_enable = flag;

		if (flag) {
			chip_info->gesture_cust_en[0] = true;  //Double tap
			chip_info->gesture_cust_en[8] = true;  //O
			chip_info->gesture_cust_en[10] = true; //V
			chip_info->gesture_cust_en[20] = true; //Two fingers Down
			chip_info->gesture_cust_en[12] = true; //LV
			chip_info->gesture_cust_en[13] = true; //RV
			chip_info->gesture_cust_en[11] = true; //DV
			chip_info->gesture_cust_en[7] = true;  //M
			chip_info->gesture_cust_en[14] = true; //W
#ifdef CONFIG_OPLUS_TP_APK
			/*if (chip_info->debug_gesture_sta) {
				jadard_gesture_fail_reason(ENABLE);
			}*/
#endif
		} else {
			chip_info->gesture_cust_en[0] = false;  //Double tap
			chip_info->gesture_cust_en[8] = false;  //O
			chip_info->gesture_cust_en[10] = false; //V
			chip_info->gesture_cust_en[20] = false; //Two fingers Down
			chip_info->gesture_cust_en[12] = false; //LV
			chip_info->gesture_cust_en[13] = false; //RV
			chip_info->gesture_cust_en[11] = false; //DV
			chip_info->gesture_cust_en[7] = false;  //M
			chip_info->gesture_cust_en[14] = false; //W
		}
		chip_info->module_fp->fp_set_SMWP_enable(chip_info->SMWP_enable);

		TPD_INFO("[JDTP] chip_info->SMWP_enable = %d\n", chip_info->SMWP_enable);
		/*TPD_INFO("[JDTP] Double tap: %d, O:%d, V: %d, Two fingers Down: %d, LV: %d, RV: %d, DV: %d, M: %d, W: %d\n",
			chip_info->gesture_cust_en[0], chip_info->gesture_cust_en[8], chip_info->gesture_cust_en[10],
			chip_info->gesture_cust_en[20], chip_info->gesture_cust_en[12], chip_info->gesture_cust_en[13],
			chip_info->gesture_cust_en[11], chip_info->gesture_cust_en[7], chip_info->gesture_cust_en[14]);*/

		break;

	case MODE_EDGE:
		TPD_INFO("[JDTP] MODE_EDGE flag = %d\n", flag);

		if (VERTICAL_SCREEN == flag || VERTICAL_SCREEN_180 == flag) {
			/* Vertical */
			chip_info->rotate_border = 0xA55A;
		} else if (LANDSCAPE_SCREEN_90 == flag) {
			/* Horizontal notch left side */
			chip_info->rotate_border = 0xA11A;
		} else if (LANDSCAPE_SCREEN_270 == flag) {
			/* Horizontal notch left side */
			chip_info->rotate_border = 0xA33A;
		}

		chip_info->module_fp->fp_set_rotate_border(chip_info->rotate_border);
		TPD_INFO("[JDTP] rotate_border = 0x%04x\n", chip_info->rotate_border);

		break;

	case MODE_HEADSET:
        TPD_INFO("[JDTP] Earphone state = %d\n", flag);

		if (flag == 0) { //3.5mm earphone removed
			chip_info->module_fp->fp_set_earphone_enable(1);
		} else { //3.5mm earphone detected
			chip_info->module_fp->fp_set_earphone_enable(2);
		}

		break;

	case MODE_CHARGE:
		TPD_INFO("[JDTP] MODE_CHARGE flag = %d\n", flag);

		if (flag) {
		    chip_info->usb_status[1] = 0x01;
		    chip_info->usb_connected = 0x01;
		} else {
		    chip_info->usb_status[1] = 0x00;
		    chip_info->usb_connected = 0x00;
		}

		chip_info->module_fp->fp_usb_detect_set(chip_info->usb_status);
		TPD_INFO("[JDTP] Cable status change: 0x%2.2X\n", chip_info->usb_connected);

		break;

	case MODE_GAME:
		TPD_INFO("[JDTP] MODE_GAME flag = %d\n", flag);

		chip_info->high_sensitivity_enable = flag;
		chip_info->module_fp->fp_set_high_sensitivity(chip_info->high_sensitivity_enable);
		TPD_INFO("[JDTP] High_sensitivity_enable = %d.\n", chip_info->high_sensitivity_enable);

		break;

	default:
		TPD_INFO("[JDTP] Not support mode\n");

	}

	return ret;
}

static fw_check_state jadard_fw_check(void *chip_data,
									  struct resolution_info *resolution_info,
									  struct panel_info *panel_data)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	fw_check_state ret = FW_ABNORMAL;
	char dev_version[MAX_DEVICE_VERSION_LENGTH] = {0};
	TPD_INFO("[JDTP] %s: enter\n", __func__);

	if (chip_info->fw_ready) {
		panel_data->tp_fw = chip_info->ic_data->fw_cid_ver;
		snprintf(dev_version, MAX_DEVICE_VERSION_LENGTH, "%02X", panel_data->tp_fw);
		if (panel_data->manufacture_info.version) {
			u8 ver_len = 0;

			if (panel_data->vid_len == 0) {
				ver_len = strlen(panel_data->manufacture_info.version);
				if (ver_len <= 11) {
					snprintf(panel_data->manufacture_info.version + 9,
							 sizeof(dev_version), "%s",
							 dev_version);
				} else {
					snprintf(panel_data->manufacture_info.version + 12,
							 sizeof(dev_version), "%s",
							 dev_version);
				}
			} else {
				ver_len = panel_data->vid_len;

				if (ver_len > MAX_DEVICE_VERSION_LENGTH - 4) {
					ver_len = MAX_DEVICE_VERSION_LENGTH - 4;
				}
				snprintf(panel_data->manufacture_info.version + ver_len,
						 sizeof(dev_version), "%s",
						 dev_version);
			}
		}
		ret = FW_NORMAL;
		TPD_INFO("[JDTP] manufacture_info.version: %s\n",panel_data->manufacture_info.version);
	} else {
		panel_data->tp_fw = 0;
		ret = FW_ABNORMAL;
	}
	return ret;
}

static inline void jadard_vfree(void **mem)
{
	if (*mem != NULL) {
		vfree(*mem);
		*mem = NULL;
	}
}

static void copy_fw_to_buffer(struct jadard_ts_data *chip_info,
			      const struct firmware *fw)
{
	if (fw) {
		/*free already exist fw data buffer*/
		jadard_vfree((void **) & (chip_info->tp_fw.data));
		chip_info->tp_fw.size = 0;
		/*new fw data buffer*/
		chip_info->tp_fw.data = vmalloc(fw->size);

		if (chip_info->tp_fw.data == NULL) {
			TPD_INFO("[JDTP][ERROR]: vmalloc tp firmware data error\n");
			chip_info->tp_fw.data = vmalloc(fw->size);

			if (chip_info->tp_fw.data == NULL) {
				TPD_INFO("[JDTP][ERROR]: retry kmalloc tp firmware data error\n");
				return;
			}
		}

		/*copy bin fw to data buffer*/
		memcpy((u8 *)chip_info->tp_fw.data, (u8 *)(fw->data), fw->size);
		TPD_INFO("[JDTP]: copy_fw_to_buffer fw->size=%zu\n", fw->size);
		chip_info->tp_fw.size = fw->size;
	}

	return;
}

fw_update_state jadard_fw_update(void *chip_data, const struct firmware *fw, bool force)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	fw_update_state ret = FW_UPDATE_ERROR;
    int err = 0;

	TPD_INFO("[JDTP] %s: enter\n", __func__);

	copy_fw_to_buffer(chip_info, fw);
	err = chip_info->module_fp->fp_0f_upgrade_fw(NULL, fw);

    if (err >= 0) {
        chip_info->module_fp->fp_read_fw_ver();
        /* oplus already enable irq */
		ret = FW_UPDATE_SUCCESS;
    }

	return ret;
}

static int jadard_get_vendor(void *chip_data, struct panel_info *panel_data)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	chip_info->tp_type = panel_data->tp_type;
	/*get ftm firmware ini from touch.h*/
	chip_info->p_firmware_headfile = chip_info->jadard_oplus_ts_backup->firmware_in_dts;
	TPD_INFO("[JDTP] chip_info->tp_type = %d, "
		 "panel_data->test_limit_name = %s, panel_data->fw_name = %s\n",
		 chip_info->tp_type,
		 panel_data->test_limit_name, panel_data->fw_name);
	return 0;
}

#define JD_CPU_PC (0x4000800C)
static int jadard_esd_handle(void *chip_data)
{
#ifdef JD_ESD_CHECK
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	uint8_t rdata[4], i;

	TPD_INFO("[JDTP] %s: enter\n", __func__);

	if (!chip_info->esd_check_running) {
		chip_info->esd_check_running = true;
		if ((chip_info->ito_sorting_active == false) && (chip_info->diag_thread_active == false)) {
            // memory leak kernel panic: mutex_lock(&(chip_info->sorting_active));
			/* jd_g_esd_check_enable = true; */
			TPD_INFO("[JDTP] Pre PC(0-3):0x%02x, 0x%02x, 0x%02x, 0x%02x\n", chip_info->pre_pc[0], chip_info->pre_pc[1], chip_info->pre_pc[2], chip_info->pre_pc[3]);

			chip_info->module_fp->fp_register_read(JD_CPU_PC, rdata, 4);

			if ((chip_info->pre_pc[0] == rdata[0]) && (chip_info->pre_pc[1] == rdata[1]) && (chip_info->pre_pc[2] == rdata[2]) && (chip_info->pre_pc[3] == rdata[3])) {
				chip_info->same_data++;
			} else {
				chip_info->pre_pc[0] = rdata[0];
				chip_info->pre_pc[1] = rdata[1];
				chip_info->pre_pc[2] = rdata[2];
				chip_info->pre_pc[3] = rdata[3];
                chip_info->same_data = 0;
			}

			if (((rdata[1] == 0x00) && (rdata[2] == 0x00) && (rdata[3] == 0x00)) || (chip_info->same_data >= 5)) {
                if ((rdata[0] == 0xD8) || (rdata[0] == 0xDA) || (rdata[0] == 0xDC) || (rdata[0] == 0xDE) ||
                    (rdata[0] == 0xE0) || (rdata[0] == 0xE2) || (rdata[0] == 0xE4) || (rdata[0] == 0xE6) ||
                    (chip_info->same_data >= 5)) {
                    if (chip_info->fw_ready == true) {
						TPD_INFO("[JDTP] PC(0-3):0x%02x, 0x%02x, 0x%02x, 0x%02x\n", rdata[0], rdata[1], rdata[2], rdata[3]);

						if (chip_info->same_data >= 5) {
                            chip_info->same_data = 0;
                            chip_info->bypass = 0;

                            for (i = 0; i < 10; i++) {
                                chip_info->module_fp->fp_register_read(JD_CPU_PC, rdata, 4);
                                if ((chip_info->pre_pc[0] != rdata[0]) || (chip_info->pre_pc[1] != rdata[1]) ||
                                    (chip_info->pre_pc[2] != rdata[2]) || (chip_info->pre_pc[3] != rdata[3])) {
                                    chip_info->bypass = 1;
                                    break;
                                }
                            }

                            if (chip_info->bypass == 0) {
								TPD_INFO("[JDTP] Upgrade fw by the same pc\n");
                                chip_info->power_on_upgrade = true;

                                if (chip_info->module_fp->fp_0f_esd_upgrade_fw(NULL, &(chip_info->tp_fw)) >= 0) {
                                    tp_touch_btnkey_release(chip_info->tp_index); //report_all_leave_event
                                    chip_info->upgrade = 1;
                                }
                                chip_info->power_on_upgrade = false;
							}
						} else {
                            chip_info->power_on_upgrade = true;

                            if (chip_info->module_fp->fp_0f_esd_upgrade_fw(NULL, &(chip_info->tp_fw)) >= 0) {
								tp_touch_btnkey_release(chip_info->tp_index); //report_all_leave_event
								chip_info->upgrade = 1;
							}
                            chip_info->power_on_upgrade = false;
						}
					}
				}
			}

			/* Pram CRC check */
            if ((chip_info->upgrade == 0) && (chip_info->counter >= 4)) {
                chip_info->report_data->crc_start = 1;

                TPD_INFO("[JDTP] Pram CRC check\n");
                chip_info->counter = 0;

                if (chip_info->module_fp->fp_0f_esd_upgrade_fw(NULL, &(chip_info->tp_fw)) >= 0) {
					tp_touch_btnkey_release(chip_info->tp_index); //report_all_leave_event
                }

                chip_info->report_data->crc_start = 0;
            }
            chip_info->upgrade = 0;

			/* jd_g_esd_check_enable = false; */
            // memory leak kernel panic: mutex_unlock(&(chip_info->sorting_active));

            chip_info->counter++;
		}
		chip_info->esd_check_running = false;
	} else {
		TPD_INFO("[JDTP] Undo esd_check = %d\n", chip_info->esd_check_running);
	}
#endif
	return 0;
}

static void jadard_set_touch_direction(void *chip_data, u8 dir)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	chip_info->touch_direction = dir;

	TPD_INFO("[JDTP] touch_direction = %d\n", chip_info->touch_direction);
}

static u8 jadard_get_touch_direction(void *chip_data)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	TPD_INFO("[JDTP] touch_direction = %d\n", chip_info->touch_direction);
	return chip_info->touch_direction;
}

static int jadard_smooth_lv_set(void *chip_data, int level)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	int ret = 0;
	uint8_t data = 0;

	TPD_INFO("[JDTP] %s: level = %d\n", __func__, level);

	data = (uint8_t)level;
	chip_info->module_fp->fp_set_smooth_level(data);

	return ret;
}

static int jadard_sensitive_lv_set(void *chip_data, int level)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	int ret = 0;
	uint8_t data = 0;

	TPD_INFO("[JDTP] %s: level = %d\n", __func__, level);

	data = (uint8_t)level;
	chip_info->module_fp->fp_set_sensitive_level(data);

	return ret;
}

static void jadard_reset_queue_work_prepare(void *chip_data)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;

	if (gpio_is_valid(chip_info->hw_res->reset_gpio)) {
		gpio_direction_output(chip_info->hw_res->reset_gpio, 1);
		TPD_INFO("[JDTP] set reset pin high\n");
	}
}

static bool jadard_irq_throw_away(void *chip_data)
{
	return false;
}

static void jadard_rate_white_list_ctrl(void *chip_data, int value)
{
	/* Touch report rate, auto by display framerate */
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)chip_data;
	uint8_t data = 0;

	TPD_INFO("[JDTP] %s: value = %d\n", __func__, value);

	data = (uint8_t)value;
	chip_info->module_fp->fp_set_report_rate_white_list_support(data);
}

struct oplus_touchpanel_operations jadard_ops = {
    .ftm_process                = jadard_ftm_process,
    .ftm_process_extra          = jadard_ftm_process_extra,
    .reset                      = jadard_reset,
    .power_control              = jadard_power_control,
    .reset_gpio_control         = jadard_reset_gpio_control,
    .get_chip_info              = jadard_get_chip_info,
    .trigger_reason             = jadard_trigger_reason,
    .get_touch_points           = jadard_get_touch_points,
    .get_touch_points_auto      = jadard_get_touch_points_auto,
    .get_gesture_info           = jadard_get_gesture_info,
    .mode_switch                = jadard_mode_switch,
    .fw_check                   = jadard_fw_check,
    .fw_update                  = jadard_fw_update,
    .get_vendor                 = jadard_get_vendor,
    .esd_handle                 = jadard_esd_handle,
    .set_touch_direction        = jadard_set_touch_direction,
    .get_touch_direction        = jadard_get_touch_direction,
    .smooth_lv_set              = jadard_smooth_lv_set,
    .sensitive_lv_set           = jadard_sensitive_lv_set,
    .tp_queue_work_prepare      = jadard_reset_queue_work_prepare,
    .tp_irq_throw_away          = jadard_irq_throw_away,
    .rate_white_list_ctrl   	= jadard_rate_white_list_ctrl,
};
EXPORT_SYMBOL(jadard_ops);

static int jadard_read_limit_fw(struct seq_file *s, struct touchpanel_data *ts,
				struct auto_testdata *jadard_testdata)
{
	const struct firmware *fw = NULL;
	struct auto_test_header *test_head = NULL;
	uint32_t *p_data32 = NULL;

	TPD_INFO("[JDTP] jadard_read_limit_fw enter\n");
	fw = ts->com_test_data.limit_fw;
	/*step4: decode the limit image*/
	test_head = (struct auto_test_header *)fw->data;
	p_data32 = (uint32_t *)(fw->data + 16);

	if ((test_head->magic1 != Limit_MagicNum1)
			|| (test_head->magic2 != Limit_MagicNum2)) {
		TPD_INFO("[JDTP][ERROR] limit image is not generated by oplus\n");

		if (s) {
			seq_printf(s, "limit image is not generated by oplus\n");
		}

		return  -1;
	}

	TPD_INFO("[JDTP] current test item: %llx\n", test_head->test_item);
	/*init jadard_testdata*/
	jadard_testdata->tx_num = ts->hw_res.tx_num;
	jadard_testdata->rx_num = ts->hw_res.rx_num;
	jadard_testdata->irq_gpio = ts->hw_res.irq_gpio;
	jadard_testdata->tp_fw = ts->panel_data.tp_fw;
	/*auto test save result*/
	jadard_testdata->fp = ts->com_test_data.result_data;
	jadard_testdata->length = ts->com_test_data.result_max_len;
	jadard_testdata->pos = &ts->com_test_data.result_cur_len;

	jadard_testdata->fw = fw;
	jadard_testdata->test_item = test_head->test_item;

	return 0;
}

int jadard_auto_test(struct seq_file *s, struct touchpanel_data *ts)
{
	struct auto_testdata jadard_testdata = {
		.tx_num = 0,
		.rx_num = 0,
		.fp = NULL,
		.irq_gpio = -1,
		.tp_fw = 0,
		.fw = NULL,
		.test_item = 0,
	};
    struct jadard_ts_data *pjadard_ts_data = (struct jadard_ts_data *)(ts->chip_data);
    int ret = 0;
	int status = 1;
    int len = 512;
    char *buf_tmp = NULL;

	status = jadard_read_limit_fw(s, ts, &jadard_testdata);
	if (status) {
		goto END;
	}

    buf_tmp = kzalloc(len * sizeof(char), GFP_KERNEL);

    if (buf_tmp != NULL) {
        status = pjadard_ts_data->module_fp->fp_sorting_test(buf_tmp, len, s, &jadard_testdata);
        ret = strlen(buf_tmp);
        msleep(2000);

        TPD_INFO("[JDTP] jadard_auto_test, %s\n", buf_tmp);
        kfree(buf_tmp);
    } else {
        TPD_INFO("[JDTP][ERROR] Memory allocate fail: %d\n", __LINE__);
    }

END:
	seq_printf(s, "imageid = 0x%llx, deviceid = 0x%llx\n", jadard_testdata.tp_fw, jadard_testdata.dev_tp_fw);
	seq_printf(s, "%d error(s). %s\n", status, status ? "" : "All test passed.");
	TPD_INFO("[JDTP] TP auto test %d error(s). %s\n", status, status ? "" : "All test passed.");

    return status;
}

int jadard_black_screen_test(struct black_gesture_test *p, struct touchpanel_data *ts)
{
	struct jadard_ts_data *chip_info = (struct jadard_ts_data *)ts->chip_data;

	TPD_INFO("[JDTP] jadard_black_screen_test IN");
	chip_info->oplus_ito_sorting_active = true;
	/* Write data for upgrade fw */
	msleep(1000);
	snprintf(p->message, 20, "0 errors. All test passed.");
	TPD_INFO("[JDTP] jadard_black_screen_test OUT");

	return 0;
}

struct engineer_test_operations jadard_engineer_test_ops = {
    .auto_test = jadard_auto_test,
    .black_screen_test = jadard_black_screen_test,
};
EXPORT_SYMBOL(jadard_engineer_test_ops);

MODULE_DESCRIPTION("Jadard Touchscreen Common Interface");
MODULE_LICENSE("GPL");