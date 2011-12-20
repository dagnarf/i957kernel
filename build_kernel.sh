#!/bin/bash

# modify below path to your CROSS TOOL CHAIN PATH
export PATH=/opt/toolchains/arm-eabi-4.4.3/bin:$PATH

mkdir obj
make O=obj ARCH=arm CROSS_COMPILE=arm-eabi- p5_lte_usa_att_rev00_defconfig
make O=obj ARCH=arm CROSS_COMPILE=arm-eabi- headers_install
make -j16 O=obj ARCH=arm CROSS_COMPILE=arm-eabi-
