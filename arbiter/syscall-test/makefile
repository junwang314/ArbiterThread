AB_ROOT=..

#BUILD is defined in the included template
#BUILD=./build

INCS=\
	-I$(AB_ROOT)/common/include

CFLAGS=\
	-g  \
	$(INCS)

SYSCALLTEST_SRCS=\
	syscall_test.c


SYSCALLTEST_OBJS=\
	$(patsubst %, $(BUILD)/%, $(patsubst %.c, %.o, $(SYSCALLTEST_SRCS)))


####varibles to fit in the make.in template

TARGETS=\
	syscall_test

DEP_SRCS=\
	$(SYSCALLTEST_SRCS)

MODULE_OBJS=


################## include
include $(AB_ROOT)/common/make.in


######################

#defined in the included template
#all:$(BUILD_TARGETS) $(BUILD)


$(BUILD)/syscall_test:$(SYSCALLTEST_OBJS)
	$(CC) $? -o $@

run:$(BUILD)/syscall_test
	$^

