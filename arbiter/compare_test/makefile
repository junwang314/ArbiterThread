AB_ROOT=..
ULIBC=../malloc-standard

#BUILD is defined in the included template
#BUILD=./build


CLIENTTEST_SRCS=\
	compare_test.c \
	$(ULIBC)/malloc.c \
	$(ULIBC)/free.c \
	$(ULIBC)/calloc.c \
	$(ULIBC)/realloc.c 

LIB_COMMON=$(AB_ROOT)/common/lib/

LIB_COMMON_OBJS=\
	$(BUILD)/timer.o

CLIENTTEST_OBJS=\
	$(patsubst %, $(BUILD)/%, $(patsubst %.c, %.o, $(CLIENTTEST_SRCS))) $(LIB_COMMON_OBJS)


INCS=\
	-I$(AB_ROOT)/common/include \
	-lpthread

CFLAGS=\
	-g  \
	$(INCS)

####varibles to fit in the make.in template

TARGETS=\
	compare_test

DEP_SRCS=\
	$(CLIENTTEST_SRCS)

MODULES=\
	$(LIB_COMMON)

MODULE_OBJS=\
	$(LIB_COMMON_OBJS)


################## include
include $(AB_ROOT)/common/make.in


######################

#defined in the included template
#all:$(BUILD_TARGETS) $(BUILD)


$(BUILD)/compare_test:$(CLIENTTEST_OBJS)	
	$(CC) $^ -o $@

