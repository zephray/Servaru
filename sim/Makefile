#
# Servaru
# Copyright 2023 Wenting Zhang
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

TARGET := simulator
OBJODIR := obj
RTLDIR := .

SIMTARGET ?= simtop
ifeq ($(VERILATOR_ROOT),)
VERILATOR_ROOT ?= $(shell bash -c 'verilator -V|grep VERILATOR_ROOT | head -1 | sed -e " s/^.*=\s*//"')
endif
VROOT := $(VERILATOR_ROOT)
VINCD := $(VROOT)/include
RTLOBJDIR := $(RTLDIR)/obj_dir
RTLOBJ := $(RTLOBJDIR)/V$(SIMTARGET)__ALL.a

CC = g++
CXX = g++
LD = g++

LIBS := -lm

COMMONFLAGS := \
	-Wall -O2 -g

CPPFLAGS := \
	-faligned-new \
	-std=c++17

LDFLAGS :=

INCLUDES += \
	-I. \
	-I$(RTLOBJDIR) \
	-I$(RTLDIR) \
	-I$(VINCD) \
	-I$(VINCD)/vltstd

CSRCS +=

CPPSRCS += \
	./main.cpp \
	verilated.cpp \
	verilated_vcd_c.cpp \
	verilated_threads.cpp

OBJS :=	$(CSRCS:%.c=$(OBJODIR)/%.o) \
		$(CPPSRCS:%.cpp=$(OBJODIR)/%.o)

OBJS += $(RTLOBJ)

$(RTLOBJ):
	@echo Building RTL
	TARGET=$(SIMTARGET) make -f rtl.mk

$(OBJODIR)/%.o: %.c $(RTLOBJ)
	@echo [CC] $<
	@mkdir -p $(dir $@)
	@$(CC) $(COMMONFLAGS) $(CCFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJODIR)/%.o: %.cpp $(RTLOBJ)
	@echo [CXX] $<
	@mkdir -p $(dir $@)
	@$(CXX) $(COMMONFLAGS) $(CPPFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJODIR)/%.o: $(VINCD)/%.cpp
	@echo [CXX] $<
	@mkdir -p $(dir $@)
	@$(CXX) $(COMMONFLAGS) $(CPPFLAGS) $(INCLUDES) -c -o $@ $<

PHONY += all
all: $(OBJS)
	@$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)
	@echo 'all finish'

PHONY += clean
clean:
	rm -rf $(OBJODIR)
	rm -f $(TARGET)
	TARGET=$(SIMTARGET) make -f rtl.mk clean
	@echo 'clean finish'

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)

# Set default target
.DEFAULT_GOAL:= all

