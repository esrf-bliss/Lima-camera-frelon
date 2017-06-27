###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
#  Copyright (C) : 2009-2017
#  European Synchrotron Radiation Facility
#  BP 220, Grenoble 38043
#  FRANCE
# 
#  Contact: lima@esrf.fr
# 
#  This is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
# 
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################

set(frelon_srcs src/Frelon.cpp  src/FrelonSerialLine.cpp
    src/FrelonModel.cpp src/FrelonCamera.cpp
    src/FrelonTimingCtrl.cpp src/FrelonInterface.cpp src/FrelonCorrection.cpp)

file(STRINGS "VERSION" frelon_vers)
add_library(limafrelon SHARED ${frelon_srcs})

target_include_directories(limafrelon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(limafrelon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tools/src")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../common/espia/cmake ${CMAKE_MODULE_PATH})
include(FindEspiaDrv)

#set paths for espia driver and limaespialib and include files
target_include_directories(limafrelon PUBLIC "${ESPIA_DRV_INCLUDE_DIR}")
target_include_directories(limafrelon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../common/espia/include")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../common/espia")

target_link_libraries(limafrelon limacore)
target_link_libraries(limafrelon ${ESPIA_DRV_LIB})
target_link_libraries(limafrelon limaespia)

set_target_properties(limafrelon PROPERTIES VERSION ${frelon_vers} SOVERSION "1")

install(TARGETS limafrelon LIBRARY DESTINATION lib)

if(COMPILE_SIP)
    set(NAME "frelon")
    set(IMPORTS
"%Import limaespia.sip")
    set(SIP_INCLUDES ${SIP_INCLUDES}
        "${CMAKE_CURRENT_SOURCE_DIR}/../common/espia/sip"
	"${CMAKE_BINARY_DIR}/camera/common/espia/sip")
    include(CameraRunSIP)    
    install(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Frelon")
    install(FILES python/FrelonAcq.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Frelon")
endif()

if(COMPILE_TEST)
	add_subdirectory(test/)
endif()
                               
