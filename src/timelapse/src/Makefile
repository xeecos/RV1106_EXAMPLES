# Rockchip sysutils Makefile

SRCS := $(wildcard *.c)
SRCS += $(wildcard *.cpp)
OBJS  := $(SRCS:%.c=%.o)
OBJS  += $(SRCS:%.cpp=%.o)

.PHONY : clean all

TARGET1 = timelapse_server


all: $(TARGET1)

$(TARGET1):  $(TARGET1).o $(OBJ)
	$(CCPLUS) $(TARGET1).o -o $@  $(LD_FLAGS) $(CFLAGS)

$(OBJS): $(SRCS)
	$(CC) -c $(filter $(basename $@)%, $^) $(LD_FLAGS) $(CFLAGS)

install:
	@mv -f $(TARGET1) $(LUCKFOX_OUT_DIR)/bin


clean:
	@rm -f $(TARGET)

