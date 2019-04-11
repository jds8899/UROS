#
# SCCS ID: @(#)Makefile	1.18        3/29/18
#
# Makefile to control the compiling, assembling and linking of
# standalone programs in the DSL.  Used for both individual
# interrupt handling assignments and the SP baseline OS (with
# appropriate tweaking).
#

#
# User supplied files
#
SYS_C_SRC = src/C64/clock.c src/C64/klibc.c src/C64/kmalloc.c src/C64/pcbs.c \
	src/C64/queues.c src/C64/scheduler.c \
	src/C64/sio.c src/C64/stacks.c src/C64/syscalls.c src/C64/system.c

SYS_C_OBJ = src/C64/clock.o src/C64/klibc.o src/C64/kmalloc.o src/C64/pcbs.o \
	src/C64/queues.o src/C64/scheduler.o \
	src/C64/sio.o src/C64/stacks.o src/C64/syscalls.o src/C64/system.o

SYS_C_R_INTER_SRC = src/C64/kmalloc.c
SYS_C_R_INTER_OBJ = src/C64/kmalloc.o

SYS_S_SRC = src/C64/klibs.S

SYS_S_OBJ = src/C64/klibs.o

SYS_SRCS = $(SYS_C_SRC) $(SYS_S_SRC)

SYS_OBJS = $(SYS_C_OBJ) $(SYS_S_OBJ)

USR_C_SRC = src/C64/ulibc.c src/C64/users.c

USR_C_OBJ = src/C64/ulibc.o src/C64/users.o

USR_S_SRC = src/C64/ulibs.S

USR_S_OBJ = src/C64/ulibs.o

USR_SRCS = $(USR_C_SRC) $(USR_S_SRC)
USR_OBJS = $(USR_C_OBJ) $(USR_S_OBJ)

#
# User compilation/assembly definable options
#
# General options:
#	CHILD_FIRST		allow the child to run first after fork()
#	CLEAR_BSS_SEGMENT	include code to clear all BSS space
#	DUMP_QUEUES		clock ISR dumps queues etc. every 10 seconds
#	DUMP_QUEUE_CONTENTS	dump full contents (vs. sizes)
#	SP_OS_CONFIG		enable SP OS-specific startup variations
#
# Debugging options:
#	DEBUG_KMALLOC		debug the kernel allocator code
#	DEBUG_KMALLOC_FREELIST	debug the freelist creation
#	DEBUG_UNEXP_INTS	dump process info in the 'unexpected' ISR
#	INCLUDE_SHELL		include a debugging shell
#	ISR_DEBUGGING_CODE	include context restore debugging code
#	REPORT_MYSTERY_INTS	print a message on interrupt 0x27
#	SANITY_CHECK		include "sanity check" tests
#	TRACE_STACK_SETUP	enable tracing of _stk_setup()
#	TRACE_EXEC		trace exec() pre/post _stk_setup()
#	TRACE_SPAWN=n		trace the activity of spawn()
#
# Values usable for TRACE_SPAWN cause output at these times:
#	1:	entry to spawn(), incoming argv, strtbl pre/post fork(),
#		outgoing argv, _sys_exec pre & post _stk_setup call
#	2 adds:	addr of strtbl, arg string list as length is calculated,
#		_sys_fork after allocs, strtbl contents
#	3 adds:	fork() return value in parent, child priority set
#
MAIN_OPTIONS = -DSP_OS_CONFIG -DCLEAR_BSS_SEGMENT
#MAIN_OPTIONS = -DSP_OS_CONFIG -DCLEAR_BSS_SEGMENT -DDUMP_QUEUES
DBG_OPTIONS = -DSANITY_CHECK -DISR_DEBUGGING_CODE -DDEBUG_UNEXP_INTS #-DTRACE_SPAWN -DDEBUG_KMALLOC
USER_OPTIONS = $(MAIN_OPTIONS) $(DBG_OPTIONS)

#
# YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS POINT!!!
#
# Compilation/assembly control
#

#
# We only want to include from the current directory and ~wrc/include
#
INCLUDES = -I. -I/home/comp_sci/capstone/include

#
# Compilation/assembly/linking commands and options
#
CPP = cpp
# CPPFLAGS = $(USER_OPTIONS) -nostdinc -I- $(INCLUDES)
CPPFLAGS = $(USER_OPTIONS) -nostdinc $(INCLUDES)

CC = gcc
CFLAGS = -std=c99 -fno-stack-protector -fno-builtin -Wall -Wstrict-prototypes $(CPPFLAGS)

AS = as
ASFLAGS =

LD = ld
LDFLAGS = -melf_x86_64

#		
# Transformation rules - these ensure that all compilation
# flags that are necessary are specified
#
# Note use of 'cpp' to convert .S files to temporary .s files: this allows
# use of #include/#define/#ifdef statements. However, the line numbers of
# error messages reflect the .s file rather than the original .S file. 
# (If the .s file already exists before a .S file is assembled, then
# the temporary .s file is not deleted.  This is useful for figuring
# out the line numbers of error messages, but take care not to accidentally
# start fixing things by editing the .s file.)
#
# The .c.X rule produces a .X file which contains the original C source
# code from the file being compiled mixed in with the generated
# assembly language code.  Very helpful when you need to figure out
# exactly what C statement generated which assembly statements!
#

.SUFFIXES:	.S .b .X

.c.X:
	$(CC) $(CFLAGS) -g -c -Wa,-adhln $*.c > $*.X

.c.s:
	$(CC) $(CFLAGS) -S $*.c

.S.s:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S

.S.o:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S
	$(AS) $(ASFLAGS) -g -o $*.o $*.s -a=$*.lst
	$(RM) -f $*.s

.s.b:
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(LD) $(LDFLAGS) -Ttext 0x0 -s --oformat binary -e begtext -o $*.b $*.o

.c.o:
	$(CC) $(CFLAGS) -g -c $*.c

# Binary/source file for system bootstrap code

BOOT_OBJ = src/C64/bootstrap.b
BOOT_SRC = src/C64/bootstrap.S

# Assembly language object/source files

FMK_S_OBJ = src/C64/long_mode.o src/C64/startup.o src/C64/isr_stubs.o $(U_S_OBJ)
FMK_S_SRC = src/C64/long_mode.S	src/C64/startup.S src/C64/isr_stubs.S $(U_S_SRC)

# C object/source files

FMK_C_OBJ =	src/C64/c_io.o src/C64/support.o $(U_C_OBJ)
FMK_C_SRC =	src/C64/c_io.c src/C64/support.c $(U_C_SRC)

# Collections of files

FMK_OBJS = $(FMK_S_OBJ) $(FMK_C_OBJ)
FMK_SRCS = $(FMK_S_SRC) $(FMK_C_SRC)

OBJECTS = $(FMK_OBJS) $(SYS_OBJS) $(USR_OBJS)
SOURCES = $(FMK_SRCS) $(SYS_SRCS) $(USR_SRCS)

TARGET ?= x86_64-uros
RUST_FILES = target/$(TARGET)/debug/liburos.a

#
# Targets for remaking bootable image of the program
#
# Default target:  usb.image
#

parts:
	+$(MAKE) -C src/C64

rust:
	@RUST_TARGET_PATH=$(shell pwd) xargo build --target $(TARGET)

build/usb.image: src/C64/bootstrap.b build/prog.b build/prog.nl build/BuildImage build/prog.dis
	build/BuildImage -d usb -o build/usb.image -b src/C64/bootstrap.b build/prog.b 0x10000

build/floppy.image: src/C64/bootstrap.b build/prog.b prog.nl build/BuildImage prog.dis
	build/BuildImage -d floppy -o build/floppy.image -b src/C64/bootstrap.b build/prog.b 0x10000

build/prog.out: rust parts
	$(LD) $(LDFLAGS) -o build/prog.out $(FMK_S_OBJ) $(RUST_FILES) $(SYS_C_R_INTER_OBJ)

build/prog.o:	rust parts
	$(LD) $(LDFLAGS) -o build/prog.o -T linker.ld $(FMK_S_OBJ) $(RUST_FILES) $(SYS_C_R_INTER_OBJ)

build/prog.b:	build/prog.o
	$(LD) $(LDFLAGS) -o build/prog.b -s --oformat binary -T linker.ld build/prog.o

#
# Targets for copying bootable image onto boot devices
#

floppy:	build/floppy.image
	dd if=floppy.image of=/dev/fd0

usb:	build/usb.image
	/home/jds/Desktop/dcopy build/usb.image

qemu:   build/usb.image
	qemu-system-x86_64 -hdb build/usb.image -serial stdio -device nec-usb-xhci -device usb-mouse -m 512

qemu-output:   build/usb.image
	qemu-system-x86_64 -hdb build/usb.image -serial stdio -device nec-usb-xhci -device usb-mouse -m 512 -d int,cpu_reset > out.txt 

debug:   build/usb.image
	qemu-system-x86_64 -s -S -hdb build/usb.image -serial stdio -device nec-usb-xhci -device usb-mouse -m 512

#
# Special rule for creating the modification and offset programs
#
# These are required because we don't want to use the same options
# as for the standalone binaries.
#

build/BuildImage:	src/build_utils/BuildImage.c
	$(CC) -o build/BuildImage src/build_utils/BuildImage.c

# Doesn't work, I don't know :/
Offsets:	src/build_utils/Offsets.c
	$(CC) -mx32 -std=c99 $(INCLUDES) -o Offsets src/build_utils/Offsets.c

#
# Clean out this directory
#

clean:
	+$(MAKE) -C src/C64 clean
	rm -f build/*.nl build/*.b build/*.o build/*.image build/*.dis build/BuildImage build/Offsets

realclean:	clean

#
# Create a printable namelist from the prog.o file
#

build/prog.nl: build/prog.o
	nm -Bng build/prog.o | pr -w80 -3 > build/prog.nl

#
# Generate a disassembly
#

build/prog.dis: build/prog.o
	objdump -d build/prog.o > build/prog.dis

#
#       makedepend is a program which creates dependency lists by
#       looking at the #include lines in the source files
#

depend:
	makedepend $(INCLUDES) $(SOURCES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

startup.o: long_mode.h bootstrap.h
long_mode.o: bootstrap.h
isr_stubs.o: bootstrap.h
c_io.o: c_io.h startup.h support.h x86arch.h
support.o: startup.h support.h c_io.h x86arch.h
support.o: bootstrap.h
clock.o: x86arch.h startup.h common.h c_io.h kmalloc.h
clock.o: support.h system.h bootstrap.h pcbs.h stacks.h queues.h klib.h
clock.o: clock.h scheduler.h sio.h syscalls.h
klibc.o: common.h c_io.h kmalloc.h support.h system.h
klibc.o: x86arch.h bootstrap.h pcbs.h stacks.h queues.h
klibc.o: klib.h scheduler.h sio.h
kmalloc.o: common.h c_io.h kmalloc.h support.h system.h
kmalloc.o: x86arch.h bootstrap.h pcbs.h stacks.h
kmalloc.o: queues.h klib.h
pcbs.o: common.h c_io.h kmalloc.h support.h system.h
pcbs.o: x86arch.h bootstrap.h pcbs.h stacks.h queues.h
pcbs.o: klib.h
queues.o: common.h c_io.h kmalloc.h support.h system.h
queues.o: x86arch.h bootstrap.h pcbs.h stacks.h
queues.o: queues.h klib.h scheduler.h sio.h
scheduler.o: common.h c_io.h kmalloc.h support.h system.h
scheduler.o: x86arch.h bootstrap.h pcbs.h stacks.h
scheduler.o: queues.h klib.h scheduler.h
sio.o: common.h c_io.h kmalloc.h support.h system.h
sio.o: x86arch.h bootstrap.h pcbs.h stacks.h queues.h
sio.o: klib.h sio.h scheduler.h startup.h uart.h
stacks.o: common.h c_io.h kmalloc.h support.h system.h
stacks.o: x86arch.h bootstrap.h pcbs.h stacks.h
stacks.o: queues.h klib.h
syscalls.o: common.h c_io.h kmalloc.h support.h system.h
syscalls.o: x86arch.h bootstrap.h pcbs.h stacks.h
syscalls.o: queues.h klib.h uart.h startup.h syscalls.h
syscalls.o: scheduler.h clock.h sio.h
system.o: common.h c_io.h kmalloc.h support.h system.h
system.o: x86arch.h bootstrap.h pcbs.h stacks.h
system.o: queues.h klib.h clock.h syscalls.h sio.h scheduler.h users.h
ulibc.o: common.h c_io.h kmalloc.h support.h system.h
ulibc.o: x86arch.h bootstrap.h pcbs.h stacks.h queues.h
ulibc.o: klib.h
users.o: common.h c_io.h kmalloc.h support.h system.h
users.o: x86arch.h bootstrap.h pcbs.h stacks.h queues.h
users.o: klib.h users.h
ulibs.o: syscalls.h common.h c_io.h kmalloc.h support.h system.h
ulibs.o: x86arch.h bootstrap.h pcbs.h stacks.h queues.h
ulibs.o: klib.h
