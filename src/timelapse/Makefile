
ifeq ($(MEDIA_PARAM), )
    MEDIA_PARAM:=../Makefile.param
    include $(MEDIA_PARAM)
endif

export LC_ALL=C
SHELL:=/bin/bash

CURRENT_DIR := $(shell pwd)

CC := $(RK_MEDIA_CROSS)-gcc
CCPLUS := $(RK_MEDIA_CROSS)-g++
AR := $(RK_MEDIA_CROSS)-ar

PKG_NAME := luckfox
PKG_BIN ?= out
PKG_BUILD ?= build

RK_MEDIA_OPTS += -Wl,-rpath-link,${RK_MEDIA_OUTPUT}/lib:$(RK_MEDIA_OUTPUT)/root/usr/lib
PKG_CONF_OPTS += $(RK_MEDIA_OPTS)
PKG_CONF_OPTS += -DRKPLATFORM=ON -DARCH64=OFF

PKG_TARGET := timelapse-build

ifeq ($(PKG_BIN),)
$(error ### $(CURRENT_DIR): PKG_BIN is NULL, Please Check !!!)
endif

SRC += $(wildcard $(CURRENT_DIR)/src/*.c)
SRC += $(wildcard $(CURRENT_DIR)/src/*.cpp)
OBJ := $(SRC:%.c=%.o)
OBJ += $(SRC:%.cpp=%.o)

INC_FLAGS := -I$(CURRENT_DIR)/include
INC_FLAGS += -I$(RK_MEDIA_OUTPUT)/include
CFLAGS += -g -Wall $(INC_FLAGS) $(PKG_CONF_OPTS) -std=c++11 -Wall -Wextra -pthread
LD_FLAGS += -L$(RK_MEDIA_OUTPUT)/lib -L$(RK_MEDIA_OUTPUT)/root/usr/lib -lpthread -lrockit -lrockchip_mpp -lrkaiq -lrksysutils -lsmartIr -lrkaudio_detect -laec_bf_process -lrga -ldl -lrockiva

export LUCKFOX_OUT_DIR=$(CURRENT_DIR)/out
export PKG_CONF_OPTS
export OBJ
export CC
export CCPLUS
export AR
export CFLAGS
export LD_FLAGS

all: $(PKG_TARGET)
	@$(call MAROC_COPY_PKG_TO_MEDIA_OUTPUT, $(RK_MEDIA_OUTPUT), $(LUCKFOX_OUT_DIR))
	@echo "build $(PKG_NAME) done";

timelapse-build:
	@mkdir -p $(LUCKFOX_OUT_DIR)/bin
	@make -C $(CURRENT_DIR)/src;
	@make -C $(CURRENT_DIR)/src install;

clean:
	@make -C $(CURRENT_DIR)/src clean
	@rm -rf $(LUCKFOX_OUT_DIR)

distclean: clean
