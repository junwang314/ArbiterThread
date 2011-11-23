
AB_ROOT=..

#BUILD is defined in the included template
#BUILD=./build

INCS=\
	-I$(AB_ROOT)/common/include

CFLAGS=\
	-g  \
	$(INCS)

ARBITER_SRCS=\
	ipc.c \
	arbiter.c


ARBITER_OBJS=\
	$(patsubst %, $(BUILD)/%, $(patsubst %.c, %.o, $(ARBITER_SRCS)))


####varibles to fit in the make.in template

TARGETS=\
	arbiter

DEP_SRCS=\
	$(ARBITER_SRCS)

MODULE_OBJS=


################## include
include $(AB_ROOT)/common/make.in


######################

#defined in the included template
#all:$(BUILD_TARGETS) $(BUILD)


$(BUILD)/arbiter:$(ARBITER_OBJS)
	$(CC) $^ -o $@

run:$(BUILD)/arbiter
	$^

