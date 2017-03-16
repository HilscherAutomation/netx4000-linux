#
# Makefile for the linux kernel.
#
ccflags-$(CONFIG_ARCH_MULTIPLATFORM) := -I$(srctree)/$(src)/include \
	-I$(srctree)/arch/arm/plat-netx4000/include

# Specific board support
obj-$(CONFIG_MACH_NETX4000)		+= board-generic.o common.o
obj-$(CONFIG_SMP)			+= platsmp.o

# Platform specific device init code
