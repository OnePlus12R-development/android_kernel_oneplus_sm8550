#ifndef _CTS_TCS_CMDS_H_
#define _CTS_TCS_CMDS_H_

struct tcs_command TcsCmd[] = {
/* cmdID, classID, isWrite, isRead, baseFlage */
	{  3,  0, 0, 1, 0 },	/* TP_STD_CMD_INFO_CHIP_FW_ID_RO */
	{  5,  0, 0, 1, 0 },	/* TP_STD_CMD_INFO_FW_VER_RO */
	{  7,  0, 0, 1, 0 },	/* TP_STD_CMD_INFO_TOUCH_XY_INFO_RO */
	{ 17,  0, 0, 1, 0 },	/* TP_STD_CMD_INFO_MODULE_ID_RO */
	
	{  1,  1, 1, 1, 0 },	/* TP_STD_CMD_TP_DATA_OFFSET_AND_TYPE_CFG_RW */
	{  2,  1, 0, 1, 0 },	/* TP_STD_CMD_TP_DATA_READ_START_RO */
	{  3,  1, 0, 1, 0 },	/* TP_STD_CMD_TP_DATA_COORDINATES_RO */
	{  4,  1, 0, 1, 0 },	/* TP_STD_CMD_TP_DATA_RAW_RO */
	{  5,  1, 0, 1, 0 },	/* TP_STD_CMD_TP_DATA_DIFF_RO */
	{  6,  1, 0, 1, 0 },	/* TP_STD_CMD_TP_DATA_BASE_RO */
	{ 10,  1, 0, 1, 0 },	/* TP_STD_CMD_TP_DATA_CNEG_RO */
	{ 20,  1, 1, 0, 0 },	/* TP_STD_CMD_TP_DATA_WR_REG_RAM_SEQUENCE_WO */
	{ 21,  1, 1, 0, 0 },	/* TP_STD_CMD_TP_DATA_WR_REG_RAM_BATCH_WO */
	{ 22,  1, 1, 0, 0 },	/* TP_STD_CMD_TP_DATA_WR_DDI_REG_SEQUENCE_WO */
	{ 35,  1, 0, 1, 0 },	/* TP_STD_CMD_GET_DATA_BY_POLLING_RO */
	
	{  0,  2, 0, 1, 0 },	/* TP_STD_CMD_SYS_STS_READ_RO */
	{  1,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_WORK_MODE_RW */
	{  3,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_DAT_RDY_FLAG_RW */
	{  4,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_PWR_STATE_RW */
	{  5,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_CHARGER_PLUGIN_RW */
	{  6,  2, 0, 1, 0 },	/* TP_STD_CMD_SYS_STS_DDI_CODE_VER_RO */
	{  7,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_DAT_TRANS_IN_NORMAL_RW */
	{  8,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_VSTIM_LVL_RW */
	{ 17,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_CNEG_RDY_FLAG_RW */
	{ 19,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_EP_PLUGIN_RW */
	{ 22,  2, 1, 0, 0 },	/* TP_STD_CMD_SYS_STS_RESET_WO */
	{ 23,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_INT_TEST_EN_RW */
	{ 24,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_SET_INT_PIN_RW */
	{ 25,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_CNEG_RD_EN_RW */
	{ 35,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_INT_MODE_RW */
	{ 36,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_INT_KEEP_TIME_RW */
	{ 51,  2, 0, 1, 0 },	/* TP_STD_CMD_SYS_STS_CURRENT_WORKMODE_RO */
	{ 63,  2, 0, 1, 0 },	/* TP_STD_CMD_SYS_STS_DATA_CAPTURE_SUPPORT_RO */
	{ 64,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_DATA_CAPTURE_EN_RW */
	{ 65,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_DATA_CAPTURE_FUNC_MAP_RW */
	{ 66,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_PANEL_DIRECTION_RW */
	{ 78,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_GAME_MODE_RW */
	{ 82,  2, 1, 1, 0 },	/* TP_STD_CMD_SYS_STS_PRODUCTION_TEST_EN_RW */
	{ 84,  2, 1, 1, 0 },   /*TP_STD_CMD_SYS_STS_diaphragm*/
	{ 85,  2, 1, 1, 0 },    /*TP_STD_CMD_SYS_STS_smooth*/
	{ 86,  2, 1, 1, 0 },    /*TP_STD_CMD_SYS_STS_sensitive*/
	{ 87,  2, 1, 1, 0 },    /*TP_STD_CMD_SYS_STS_ctrl_rate*/
	{ 88,  2, 1, 1, 0 },    /*TP_STD_CMD_SYS_STS_get_water_flag*/
	
	{  1,  3, 1, 1, 0 },	/* TP_STD_CMD_GSTR_WAKEUP_EN_RW */
	{ 30,  3, 1, 1, 0 },	/* TP_STD_CMD_GSTR_DAT_RDY_FLAG_GSTR_RW */
	{ 32,  3, 1, 1, 0 },	/* TP_STD_CMD_GSTR_RAW_DBG_MODE_RW */
	{ 40,  3, 1, 1, 0 },	/* TP_STD_CMD_GSTR_ENTER_MAP_RW */
	{ 43,  3, 1, 1, 0 },	/* TP_STD_CMD_GSTR_RECOVER_PWR_MODE_RW */
	
	{  1,  4, 1, 1, 0 },	/* TP_STD_CMD_MNT_EN_RW */
	{  3,  4, 1, 0, 0 },	/* TP_STD_CMD_MNT_FORCE_EXIT_MNT_WO */
	
	{  1,  5, 1, 1, 0 },	/* TP_STD_CMD_DDI_ESD_EN_RW */
	{  2,  5, 1, 1, 0 },	/* TP_STD_CMD_DDI_ESD_OPTIONS_RW */
	
	{  1,  6, 1, 1, 0 },	/* TP_STD_CMD_CNEG_EN_RW */
	{  2,  6, 1, 1, 0 },	/* TP_STD_CMD_CNEG_OPTIONS_RW */
	
	{  2,  7, 1, 1, 0 },	/* TP_STD_CMD_COORD_FLIP_X_EN_RW */
	{  3,  7, 1, 1, 0 },	/* TP_STD_CMD_COORD_FLIP_Y_EN_RW */
	{  4,  7, 1, 1, 0 },	/* TP_STD_CMD_COORD_SWAP_AXES_EN_RW */
	
	{ 42,  9, 1, 1, 0 },	/* TP_STD_CMD_PARA_PROXI_EN_RW */
	
	{  1, 11, 1, 1, 0 },	/* TP_STD_CMD_OPENSHORT_EN_RW */
	{  2, 11, 1, 1, 0 },	/* TP_STD_CMD_OPENSHORT_MODE_SEL_RW */
	{  3, 11, 1, 1, 0 },	/* TP_STD_CMD_OPENSHORT_SHORT_SEL_RW */
	{  4, 11, 1, 1, 0 },	/* TP_STD_CMD_OPENSHORT_SHORT_DISP_ON_EN_RW */
};

#endif /* _CTS_TCS_CMDS_H_ */
