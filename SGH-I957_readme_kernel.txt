HOW TO BUILD KERNEL 2.6.35 FOR SGH-I957

1. How to Build
	- get Toolchain
			From Codesourcery site( http://www.codesourcery.com )
			Ex) Download : http://www.codesourcery.com/sgpp/lite/arm/portal/package7813/public/arm-none-eabi/arm-2010.09-51-arm-none-eabi-i686-pc-linux-gnu.tar.bz2

			recommand :
							Feature : ARM
							Target OS : "EABI"
							package : "IA32 GNU/Linux TAR"

	- edit build_kernel.sh
			edit "PATH" to right toolchain path(You downloaded).
			Ex)  PATH=/opt/toolchains/arm-eabi-4.4.3/bin/:$PATH                 // You have to check.

	- make
			$ cd kernel
			$ ./build_kernel.sh
	
2. Output files
	- Kernel : Kernel/obj/arch/arm/boot/zImage
	- module : Kernel/obj/drivers/*/*.ko
	