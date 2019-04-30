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
SYS_C_SRC = kmalloc.c

SYS_C_OBJ = kmalloc.o

SYS_S_SRC =

SYS_S_OBJ =

SYS_SRCS = $(SYS_C_SRC) $(SYS_S_SRC)

SYS_OBJS = $(SYS_C_OBJ) $(SYS_S_OBJ)

USR_C_SRC =

USR_C_OBJ =

USR_S_SRC = ulibs.S

USR_S_OBJ = ulibs.o

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
INCLUDES = -I. -I/home/fac/wrc/include

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

BOOT_OBJ = bootstrap.b
BOOT_SRC = bootstrap.S

# Assembly language object/source files

FMK_S_OBJ = long_mode.o startup.o isr_stubs.o $(U_S_OBJ)
FMK_S_SRC = long_mode.S	startup.S isr_stubs.S $(U_S_SRC)

# C object/source files

FMK_C_OBJ =	$(U_C_OBJ)
FMK_C_SRC =	$(U_C_SRC)

# Collections of files

FMK_OBJS = $(FMK_S_OBJ) $(FMK_C_OBJ)
FMK_SRCS = $(FMK_S_SRC) $(FMK_C_SRC)

OBJECTS = $(FMK_OBJS) $(SYS_OBJS) $(USR_OBJS)
SOURCES = $(FMK_SRCS) $(SYS_SRCS) $(USR_SRCS)


#
# For top level Makefile
#
all: $(OBJECTS)

#
# Clean out this directory
#

clean:
	rm -f *.X *.nl *.lst *.b *.o *.image *.dis *.s BuildImage Offsets

realclean:	clean

#
#       makedepend is a program which creates dependency lists by
#       looking at the #include lines in the source files
#

depend:
	makedepend $(INCLUDES) $(SOURCES)

