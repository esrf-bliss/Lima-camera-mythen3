############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2017
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
