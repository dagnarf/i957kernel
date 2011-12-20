#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#

obj-$(CONFIG_SENSORS_MT9T013)	+= mt9t013.o
obj-$(CONFIG_SENSORS_YDA165)	+= yda165.o
obj-$(CONFIG_SENSORS_YDA160)	+= yda160.o
obj-$(CONFIG_SENSORS_MAX9879)	+= max9879.o

ifeq ($(SEC_PROJECT_REGION_OPERATOR), P8LTE_KOR_SKT)
obj-$(CONFIG_OPTICAL_GP2A)  += gp2a_proximity_E150S.o gp2a_light_E150S.o gp2a_proc_E150S.o
else
obj-$(CONFIG_OPTICAL_GP2A)		+= gp2a_proximity.o gp2a_light.o gp2a_proc.o
endif

obj-$(CONFIG_USB_SWITCH_FSA9480)	+= fsa9480.o
obj-$(CONFIG_WM8994_AMP)	+= wm8994_amp.o

ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif
