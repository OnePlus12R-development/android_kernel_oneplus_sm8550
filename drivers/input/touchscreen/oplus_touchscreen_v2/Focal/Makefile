#
# Makefile for the touchscreen drivers.
#

# Each configuration option enables a list of files.
#obj-$(CONFIG_TOUCHPANEL_FOCAL) += focal_common.o
#obj-$(CONFIG_TOUCHPANEL_FOCAL_FT8006) += FT8006/
#obj-$(CONFIG_TOUCHPANEL_FOCAL_FT3518) += ft3518/


obj-$(CONFIG_TOUCHPANEL_FOCAL_FT3518) += ft3518/
obj-$(CONFIG_TOUCHPANEL_FOCAL_FT3658U) += ft3658u/
obj-$(CONFIG_TOUCHPANEL_FOCAL_FT3658U_SPI) += ft3658u_spi/
obj-$(CONFIG_TOUCHPANEL_FOCAL_FT3681) += ft3681/
obj-$(CONFIG_TOUCHPANEL_FOCAL_FT8057P) += ft8057p/
obj-$(CONFIG_TOUCHPANEL_FOCAL_FT3683G) += ft3683g/

oplus_bsp_tp_focal_common-y += focal_common.o
obj-$(CONFIG_TOUCHPANEL_FOCAL) += oplus_bsp_tp_focal_common.o
#endif
