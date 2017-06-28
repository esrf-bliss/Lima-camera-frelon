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
set(NAME "frelon")

set(${NAME}_srcs src/Frelon.cpp  src/FrelonSerialLine.cpp
    src/FrelonModel.cpp src/FrelonCamera.cpp
    src/FrelonTimingCtrl.cpp src/FrelonInterface.cpp src/FrelonCorrection.cpp)

add_library(lima${NAME} SHARED ${${NAME}_srcs})

target_include_directories(lima${NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(lima${NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tools/src")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../common/espia/cmake ${CMAKE_MODULE_PATH})
include(FindEspiaDrv)

#set paths for espia driver and limaespialib and include files
target_include_directories(lima${NAME} PUBLIC "${ESPIA_DRV_INCLUDE_DIR}")
target_include_directories(lima${NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../common/espia/include")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../common/espia")

target_link_libraries(lima${NAME} limacore)
target_link_libraries(lima${NAME} ${ESPIA_DRV_LIB})
target_link_libraries(lima${NAME} limaespia)

include(CameraVERSION)
install(TARGETS lima${NAME} LIBRARY DESTINATION lib)

if(LIMA_ENABLE_PYTHON)
    set(IMPORTS
"%Import limaespia.sip")
    set(SIP_INCLUDES ${SIP_INCLUDES}
        "${CMAKE_CURRENT_SOURCE_DIR}/../common/espia/sip"
	"${CMAKE_BINARY_DIR}/camera/common/espia/sip")
    include(CameraRunSIP)    
    install(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Frelon")
    install(FILES python/FrelonAcq.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Frelon")
endif()

if(LIMA_ENABLE_TESTS)
	add_subdirectory(test/)
endif()
                               
