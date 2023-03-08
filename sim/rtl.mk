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

TARGET ?= simtop
all: $(TARGET)

VOBJ := obj_dir
CXX   := g++
FBDIR := ../rtl/genrtl
CPUS ?= $(shell bash -c 'nproc --all')
VERBOSE ?= 0

.PHONY: rtl
rtl: ../rtl/genrtl/simtop.v

../rtl/genrtl/simtop.v:
	make --directory=../rtl

.PHONY: all
$(TARGET): $(VOBJ)/V$(TARGET)__ALL.a

SUBMAKE := $(MAKE) --no-print-directory --directory=$(VOBJ) -f
ifeq ($(VERILATOR_ROOT),)
VERILATOR := verilator
else
VERILATOR := $(VERILATOR_ROOT)/bin/verilator
endif
VFLAGS := -Wall -Wno-fatal -MMD --trace -cc \
		-I../rtl/genrtl
ifeq ($(VERBOSE), 1)
VFLAGS += +define+VERBOSE=1
endif

$(VOBJ)/V$(TARGET)__ALL.a: $(VOBJ)/V$(TARGET).cpp $(VOBJ)/V$(TARGET).h
$(VOBJ)/V$(TARGET)__ALL.a: $(VOBJ)/V$(TARGET).mk

$(VOBJ)/V%.cpp $(VOBJ)/V%.h $(VOBJ)/V%.mk: $(FBDIR)/%.v
	$(VERILATOR) $(VFLAGS) $*.v

$(VOBJ)/V%.cpp: $(VOBJ)/V%.h
$(VOBJ)/V%.mk:  $(VOBJ)/V%.h
$(VOBJ)/V%.h: $(FBDIR)/%.v

$(VOBJ)/V%__ALL.a: $(VOBJ)/V%.mk
	$(SUBMAKE) V$*.mk -j$(CPUS)

.PHONY: clean
clean:
	make --directory=../rtl clean
	rm -rf $(VOBJ)/*.mk
	rm -rf $(VOBJ)/*.cpp
	rm -rf $(VOBJ)/*.h
	rm -rf $(VOBJ)/
