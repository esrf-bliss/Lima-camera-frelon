# CMakeLists.txt for Frelon camera

SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")


SET(frelon_srcs src/FrelonCamera.cpp  src/FrelonInterface.cpp
	 src/FrelonCorrection.cpp src/Frelon.cpp
	 src/FrelonModel.cpp src/FrelonSerialLine.cpp
	 src/FrelonTimingCtrl.cpp)

FILE(STRINGS "VERSION" frelon_vers)
ADD_LIBRARY(limafrelon SHARED ${frelon_srcs})

TARGET_INCLUDE_DIRECTORIES(limafrelon PUBLIC "${CMAKE_SOURCE_DIR}/camera/common/espia/include")
TARGET_INCLUDE_DIRECTORIES(limafrelon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

#FIND_LIBRARY(FRELON_LIB nom_frelon_lib ${})


TARGET_LINK_LIBRARIES(limafrelon limacore)


SET_TARGET_PROPERTIES(limafrelon PROPERTIES VERSION ${frelon_vers} SOVERSION "1")

INSTALL(TARGETS limafrelon LIBRARY DESTINATION lib)

IF(COMPILE_SIP)
    SET(NAME "frelon")
    SET(INCLUDES)
    FILE(GLOB sipfiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/sip" "${CMAKE_CURRENT_SOURCE_DIR}/sip/*.sip")
    FOREACH(sipfile ${sipfiles})
        SET(INCLUDES 
        "${INCLUDES} 
        %Include ${sipfile}")
    ENDFOREACH()

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/sip/limamodules.sip.in sip/limafrelon.sip)
    SET(SIP_CONCAT_PARTS 1)
    SET(SIP_INCLUDES ${SIP_INCLUDES}
        "${CMAKE_SOURCE_DIR}/third-party/Processlib/sip"
        "${CMAKE_BINARY_DIR}/sip/core"
        "${CMAKE_SOURCE_DIR}/third-party/Processlib/tasks/sip"
        "${CMAKE_SOURCE_DIR}/common/sip"
        "${CMAKE_SOURCE_DIR}/hardware/sip"
        "${CMAKE_SOURCE_DIR}/control/sip"
        "${CMAKE_SOURCE_DIR}/control/software_operation/sip"
        "${CMAKE_CURRENT_SOURCE_DIR}/sip")
    set(SIP_DISABLE_FEATURES WITH_CONFIG)
    set(SIP_CHECK_EXC ON)
    ADD_SIP_PYTHON_MODULE(limafrelon ${CMAKE_CURRENT_BINARY_DIR}/sip/limafrelon.sip)
    TARGET_INCLUDE_DIRECTORIES(python_module_limafrelon PRIVATE
                               ${PYTHON_INCLUDE_DIRS}
                               "${CMAKE_SOURCE_DIR}/sip"
                               "${CMAKE_SOURCE_DIR}/sip/core")
    TARGET_LINK_LIBRARIES(python_module_limafrelon limafrelon)

    INSTALL(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Frelon")
ENDIF()
