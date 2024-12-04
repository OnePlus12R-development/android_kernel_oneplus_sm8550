#ifndef JADARD_OPLUS_H
#define JADARD_OPLUS_H

#include "./jd9365tg/jadard_common.h"
#include "./jd9365tg/jadard_module.h"

void jadard_baseline_read(struct seq_file *s, void *chip_data);
void jadard_delta_read(struct seq_file *s, void *chip_data);
void jadard_main_register_read(struct seq_file *s, void *chip_data);
//void jadard_tp_limit_data_write(void *chip_data, int32_t count);
/* Must add jadard_ops function */
int jadard_auto_test(struct seq_file *s, struct touchpanel_data *ts);
int jadard_black_screen_test(struct black_gesture_test *p, struct touchpanel_data *ts);

#endif