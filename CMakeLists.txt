###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
#  Copyright (C) : 2009-2017
#  European Synchrotron Radiation Facility
#  CS40220 38043 Grenoble Cedex 9 
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

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

#SOURCES
set(mythen3_srcs src/Mythen3Interface.cpp  src/Mythen3Camera.cpp
	src/Mythen3Net.cpp)
	 
file(STRINGS "VERSION" mythen3_vers)
add_library(limamythen3 SHARED ${mythen3_srcs})

#INCLUDEs
target_include_directories(limamythen3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(limamythen3 PUBLIC "${CMAKE_SOURCE_DIR}/third-party/Processlib/sip/")

#LIBRARIES

target_link_libraries(limamythen3 limacore)


set_target_properties(limamythen3 PROPERTIES VERSION ${mythen3_vers} SOVERSION "1")

install(TARGETS limamythen3 LIBRARY DESTINATION lib)

if(COMPILE_SIP)
    set(NAME "mythen3")
    include(CameraRunSIP)
    install(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Mythen3")
endif()

if(COMPILE_TEST)
	add_subdirectory(test/)
endif()
