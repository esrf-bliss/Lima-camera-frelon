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

frelon-objs = FrelonBase.o FrelonSerialLine.o FrelonModel.o FrelonCamera.o \
	      FrelonTimingCtrl.o FrelonInterface.o FrelonCorrection.o

SRCS = $(frelon-objs:.o=.cpp) 

CPPFLAGS = -I../include -I$(LIMA_ESPIA_DIR)/include $(CT_CPPFLAGS) \
	   -I$(ESPIA_DRV_INCL)
CXXFLAGS = $(CT_CXXFLAGS)


all:	Frelon.o

Frelon.o:	$(frelon-objs)
	$(LD) -o $@ -r $+

FrelonBase.o:	Frelon.cpp
	$(CXX) $(CPPFLAGS) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o *.P

%.o : %.cpp
	$(COMPILE.cpp) $(CPPFLAGS) -MD $(CXXFLAGS) -o $@ $<
	@cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d

-include $(SRCS:.cpp=.P)

.PHONY: check-syntax
check-syntax:
	$(CXX) $(CPPFLAGS) -Wextra -fsyntax-only $(CXXFLAGS) $(CHK_SOURCES)
