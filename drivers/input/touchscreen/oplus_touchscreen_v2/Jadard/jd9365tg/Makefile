ccflags-y += -DCONFIG_TOUCHSCREEN_JADARD_DEBUG=y
ccflags-y += -DCONFIG_TOUCHSCREEN_JADARD_SORTING=y
ccflags-y += -D__JADARD_KMODULE__
ccflags-y += -D__JADARD_GKI__

ccflags-y += -Wall

oplus_bsp_tp_jadard9365tg-y                  += jadard_module.o \
                                                jadard_ic_JD9365TG.o \
                                                jadard_common.o \
                                                jadard_platform.o \
                                                jadard_debug.o \
                                                jadard_sorting.o
obj-$(CONFIG_TOUCHPANEL_JADARD_JD9365TG) += oplus_bsp_tp_jadard9365tg.o
