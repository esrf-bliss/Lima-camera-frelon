############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2011
# European Synchrotron Radiation Facility
# BP 220, Grenoble 38043
# FRANCE
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################
LIMA_DIR := $(shell pwd)/../../..
LIMA_ESPIA_DIR := $(LIMA_DIR)/camera/common/espia

include $(LIMA_DIR)/config.inc
include $(LIMA_DIR)/control/control.inc
include $(LIMA_ESPIA_DIR)/include/espia.inc

SRCS = test_frelon.cpp test_frelon_interface.cpp test_frelon_spectroscopy.cpp \
       test_frelon_control.cpp

LDFLAGS = -pthread -L$(LIMA_DIR)/build -L$(ESPIA_DRV_LIB)
LDLIBS = -llimafrelon -llimaespia -llimacore -lespia

ifneq ($(COMPILE_CBF_SAVING), 0)
LDFLAGS += -L$(LIMA_DIR)/third-party/CBFLib/lib
LDLIBS += -lcbf
endif

ifneq ($(COMPILE_CONFIG), 0)
LDFLAGS += -L$(LIMA_DIR)/third-party/libconfig/lib/.libs
LDLIBS += -lconfig++
endif

CXXFLAGS = -I../include -I$(LIMA_ESPIA_DIR)/include $(CT_CXXFLAGS) \
	   -I$(ESPIA_DRV_INCL)

test-progs = test_frelon test_frelon_interface test_frelon_spectroscopy \
	     test_frelon_control
old-progs  = testfrelon testfreloninterface testfrelonspectroscopy \
	     testfreloncontrol

all: 	$(test-progs)

test_frelon:		test_frelon.o
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

test_frelon_interface:	test_frelon_interface.o
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

test_frelon_spectroscopy:	test_frelon_spectroscopy.o
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

test_frelon_control:	test_frelon_control.o
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

clean:
	rm -f *.o *.P $(test-progs) $(old-progs)

%.o : %.cpp
	$(COMPILE.cpp) -MD $(CXXFLAGS) -o $@ $<
	@cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d

-include $(SRCS:.cpp=.P)
