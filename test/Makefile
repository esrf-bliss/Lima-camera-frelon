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
include ../../../config.inc

SRCS = test_frelon.cpp test_frelon_interface.cpp test_frelon_spectroscopy.cpp \
       test_frelon_control.cpp

include ../../common/espia/include/espia.inc

LDFLAGS = -pthread -L../../../build -L$(ESPIA_DRV_LIB) \
	  -L../../../third-party/Processlib/build 
LDLIBS = -llimafrelon -llimaespia -llimacore -lespia -lprocesslib 

ifneq ($(COMPILE_CBF_SAVING), 0)
LDFLAGS += -L../../../third-party/CBFLib/lib
LDLIBS += -lcbf
endif

CXXFLAGS = -I../include -I../../common/espia/include \
	   -I../../../hardware/include -I../../../common/include \
           -I../../../control/include/ \
           -I../../../control/software_operation/include/ \
	   -I../../../third-party/Processlib/core/include \
	   -I../../../third-party/Processlib/tasks/include \
	   -I../../../third-party/Sps/Include \
	   -I$(ESPIA_DRV_INCL) -Wall -pthread -g

test-progs = test_frelon test_frelon_interface test_frelon_spectroscopy \
	     test_frelon_control

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
	rm -f *.o *.P test_frelon test_frelon_interface test_frelon_spectroscopy

%.o : %.cpp
	$(COMPILE.cpp) -MD $(CXXFLAGS) -o $@ $<
	@cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d

-include $(SRCS:.cpp=.P)
