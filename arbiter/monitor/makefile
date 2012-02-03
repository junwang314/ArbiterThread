
AB_ROOT=..

#BUILD is defined in the included template
#BUILD=./build

INCS=\
	-I$(AB_ROOT)/common/include

CFLAGS=\
	-g  \
	-lpthread \
	$(INCS)

ARBITER_SRCS=\
	ipc.c \
	arbiter.c \
	ablib_malloc.c \
	ablib_free.c \
	client.c \
	label.c \
	ablib_brk.c \
	ablib_sbrk.c

ABTEST_SRCS=\
	ablib_malloc.c \
	ablib_free.c \
	label.c \
	ablib_brk.c \
	ablib_sbrk.c \
	test_ablib_malloc.c \


LIB_COMMON=$(AB_ROOT)/common/lib/

LIB_COMMON_OBJS=\
	linked_list.o

ARBITER_OBJS=\
	$(patsubst %, $(BUILD)/%, $(patsubst %.c, %.o, $(ARBITER_SRCS))) $(LIB_COMMON_OBJS)

ABTEST_OBJS=\
	$(patsubst %, $(BUILD)/%, $(patsubst %.c, %.o, $(ABTEST_SRCS))) $(LIB_COMMON_OBJS)


####varibles to fit in the make.in template

TARGETS=\
	arbiter test

DEP_SRCS=\
	$(ARBITER_SRCS)

MODULES=\
	$(LIB_COMMON) \

MODULE_OBJS=\
	$(LIB_COMMON_OBJS) \


################## include
include $(AB_ROOT)/common/make.in



######################

#defined in the included template
#all:$(BUILD_TARGETS) $(BUILD)


$(BUILD)/arbiter:$(ARBITER_OBJS)
	$(CC) $^ -o $@

$(BUILD)/test:$(ABTEST_OBJS)
	$(CC) $^ -o $@

run:$(BUILD)/arbiter
	$^

run-test:$(BUILD)/test
	$^