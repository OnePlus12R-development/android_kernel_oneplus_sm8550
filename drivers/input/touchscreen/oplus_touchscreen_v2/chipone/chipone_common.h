#ifndef CTS_COMMON_H
#define CTS_COMMON_H

#include <linux/firmware.h>
#include <linux/proc_fs.h>

#include "../touchpanel_common.h"
#include "../touch_comon_api/touch_comon_api.h"
#include "../touchpanel_autotest/touchpanel_autotest.h"
#include "../touchpanel_healthinfo/touchpanel_healthinfo.h"


struct cts_testdata {
	int tx_num;
	int rx_num;
	int fd;
	int irq_gpio;
	int key_tx;
	int key_rx;
	uint64_t  tp_fw;
	const struct firmware *fw;
};

struct cts_autotest_para {
	uint16_t    limit_type_rawdata;
	signed int   test_rawdata_min;
	signed int   test_rawdata_max;
	unsigned int test_rawdata_frames;
	uint16_t    limit_type_noise;
	signed int   test_noise_min;
	signed int   test_noise_max;
	unsigned int test_noise_frames;
	uint16_t    limit_type_open;
	signed int   test_open_min;
	signed int   test_open_max;
	uint16_t    limit_type_short;
	signed int   test_short_min;
	signed int   test_short_max;
	uint16_t    limit_type_comp_cap;
	signed int   test_comp_cap_min;
	signed int   test_comp_cap_max;
	/*gesture data test*/
	uint16_t    limit_type_gstr_rawdata;
	signed int   test_gstr_rawdata_min;
	signed int   test_gstr_rawdata_max;
	unsigned int test_gstr_rawdata_frames;
	uint16_t    limit_type_gstr_lp_rawdata;
	signed int   test_gstr_lp_rawdata_min;
	signed int   test_gstr_lp_rawdata_max;
	unsigned int test_gstr_lp_rawdata_frames;
	uint16_t    limit_type_gstr_noise;
	signed int   test_gstr_noise_min;
	signed int   test_gstr_noise_max;
	unsigned int test_gstr_noise_frames;
	uint16_t    limit_type_gstr_lp_noise;
	signed int   test_gstr_lp_noise_min;
	signed int   test_gstr_lp_noise_max;
	unsigned int test_gstr_lp_noise_frames;
	uint8_t		 test_work_mode;
};

struct cts_autotest_offset {
	int32_t *cts_rawdata_min;
	int32_t *cts_rawdata_max;
	int32_t *cts_noise_min;
	int32_t *cts_noise_max;
	int32_t *cts_open_min;
	int32_t *cts_open_max;
	int32_t *cts_short_min;
	int32_t *cts_short_max;
	int32_t *cts_comp_cap_min;
	int32_t *cts_comp_cap_max;
	int32_t *cts_gstr_rawdata_min;
	int32_t *cts_gstr_rawdata_max;
	int32_t *cts_gstr_lp_rawdata_min;
	int32_t *cts_gstr_lp_rawdata_max;
	int32_t *cts_gstr_noise_min;
	int32_t *cts_gstr_noise_max;
	int32_t *cts_gstr_lp_noise_min;
	int32_t *cts_gstr_lp_noise_max;
};

int cts_auto_test(struct seq_file *s,  struct touchpanel_data *ts);
int cts_black_screen_autotest(struct black_gesture_test *p,
	struct touchpanel_data *ts);

struct cts_auto_test_operations {
	int (*test1)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*test2)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*test3)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*test4)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*test5)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*test6)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*test7)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	
	int (*black_screen_test1)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*black_screen_test2)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*black_screen_test3)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*black_screen_test4)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*auto_test_preoperation)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*auto_test_endoperation)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*black_screen_test_preoperation)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
	int (*black_screen_test_endoperation)(struct seq_file *s, void *chip_data,
		struct auto_testdata *cts_testdata, struct test_item_info *p_test_item_info);
};

enum {
	TYPE_ERROR                              = 0x00,
	TYPE_INT_PIN              				= 0x01,
	TYPE_RESET_PIN                    		= 0x02,
	TYPE_RAWDATA                         	= 0x03,
	TYPE_NOISE                       		= 0x04,
	TYPE_OPEN                       		= 0x05,
	TYPE_SHORT                      		= 0x06,
	TYPE_COMP_CAP                           = 0x07,
	TYPE_GSTR_RAWDATA                       = 0x08,
	TYPE_GSTR_LP_RAWDATA                  	= 0x09,
	TYPE_GSTR_NOISE                        	= 0x0A,
	TYPE_GSTR_LP_NOISE                  	= 0x0B,
	TYPE_MAX                                = 0xFF,
};

#endif /* CTS_COMMON_H */

