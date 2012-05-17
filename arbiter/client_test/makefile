
AB_ROOT=..

#BUILD is defined in the included template
#BUILD=./build


CLIENTTEST_SRCS=\
	client_test.c \
	micro_benchmark.c \

LIB_CLIENT=$(AB_ROOT)/client_module/

LIB_COMMON=$(AB_ROOT)/common/lib/

LIB_CLIENT_OBJS=\
	$(BUILD)/lib_client.o \

LIB_COMMON_OBJS=\
	$(BUILD)/timer.o

CLIENTTEST_OBJS=\
	$(patsubst %, $(BUILD)/%, $(patsubst %.c, %.o, $(CLIENTTEST_SRCS))) $(LIB_CLIENT_OBJS) $(LIB_COMMON_OBJS)


INCS=\
	-I$(AB_ROOT)/common/include \
	-I$(LIB_CLIENT) \
	-lpthread

CFLAGS=\
	-g  \
	$(INCS)

####varibles to fit in the make.in template

TARGETS=\
	client_test

DEP_SRCS=\
	$(wildcard *.c)

MODULES=\
	$(LIB_CLIENT) \
	$(LIB_COMMON)

MODULE_OBJS=\
	$(LIB_CLIENT_OBJS) \
	$(LIB_COMMON_OBJS)


################## include
include $(AB_ROOT)/common/make.in


######################

#defined in the included template
#all:$(BUILD_TARGETS) $(BUILD)


$(BUILD)/client_test:$(CLIENTTEST_OBJS)	
	$(CC) $^ -o $@

