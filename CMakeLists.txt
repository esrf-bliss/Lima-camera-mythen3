# CMakeLists.txt for Mythen3 camera

SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

#SOURCES
SET(mythen3_srcs src/Mythen3Interface.cpp  src/Mythen3Camera.cpp
	src/Mythen3Net.cpp)
	 
FILE(STRINGS "VERSION" mythen3_vers)
ADD_LIBRARY(limamythen3 SHARED ${mythen3_srcs})

#INCLUDEs
TARGET_INCLUDE_DIRECTORIES(limamythen3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
TARGET_INCLUDE_DIRECTORIES(limamythen3 PUBLIC "${CMAKE_SOURCE_DIR}/third-party/Processlib/sip/")

#LIBRARIES

TARGET_LINK_LIBRARIES(limamythen3 limacore)


SET_TARGET_PROPERTIES(limamythen3 PROPERTIES VERSION ${mythen3_vers} SOVERSION "1")

INSTALL(TARGETS limamythen3 LIBRARY DESTINATION lib)

IF(COMPILE_SIP)
    SET(NAME "mythen3")
    include(CameraRunSIP)
    INSTALL(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Mythen3")
ENDIF()
