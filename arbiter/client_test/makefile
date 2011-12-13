
AB_ROOT=..

#BUILD is defined in the included template
#BUILD=./build


CLIENTTEST_SRCS=\
	client_test.c \

LIB_CLIENT=$(AB_ROOT)/client_module/

LIB_CLIENT_OBJS=\
	lib_client.o \

CLIENTTEST_OBJS=\
	$(patsubst %, $(BUILD)/%, $(patsubst %.c, %.o, $(CLIENTTEST_SRCS))) $(LIB_CLIENT_OBJS)


INCS=\
	-I$(AB_ROOT)/common/include \
	-I$(LIB_CLIENT) \

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

MODULE_OBJS=\
	$(LIB_CLIENT_OBJS) \


################## include
include $(AB_ROOT)/common/make.in


######################

#defined in the included template
#all:$(BUILD_TARGETS) $(BUILD)


$(BUILD)/client_test:$(CLIENTTEST_OBJS)	
	$(CC) $^ -o $@

run:$(BUILD)/client_test
	$^

