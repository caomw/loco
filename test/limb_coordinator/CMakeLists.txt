############################################################################################
# Software License Agreement (BSD License)
#
# Copyright (c) 2014, Christian Gehring, Péter Fankhauser, C. Dario Bellicoso, Stelian Coros
# All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials provided
#     with the distribution.
#   * Neither the name of Autonomous Systems Lab nor ETH Zurich
#     nor the names of its contributors may be used to endorse or
#     promote products derived from this software without specific
#     prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
#  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
# Project configuration
cmake_minimum_required (VERSION 2.8)

# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
#set(ROS_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-std=c++0x)

find_package(Eigen REQUIRED)
find_package(Kindr REQUIRED)

include_directories(${EIGEN_INCLUDE_DIRS})
include_directories(${Kindr_INCLUDE_DIRS})


include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})


#include_directories(../../thirdparty/tinyxml)
#include_directories(../../include)
#include_directories(../../temp_helpers/include)

set(LIMBCOORDINATOR_SRCS
	../test_main.cpp
	LimbCoordinatorTest.cpp
	)
	set(TEAteateas
	../../src/limb_coordinator/LimbCoordinatorBase.cpp
	../../src/limb_coordinator/LimbCoordinatorDynamicGait.cpp
	

	
	../../src/common/TorsoBase.cpp
	../../src/common/TorsoStarlETH.cpp
	../../src/common/TorsoStateBase.cpp
	../../src/common/TorsoStateMeasured.cpp
	../../src/common/TorsoStateDesired.cpp
	
    ../../src/common/TorsoPropertiesBase.cpp
	../../src/common/TorsoPropertiesStarlETH.cpp
	../../src/common/LegGroup.cpp
	../../src/common/LegBase.cpp
	../../src/common/LegStarlETH.cpp
	../../src/common/LegStateBase.cpp
	../../src/common/LegStateLiftOff.cpp
	../../src/common/LegStateTouchDown.cpp
	../../src/common/LegPropertiesBase.cpp
	../../src/common/LegPropertiesStarlETH.cpp
	
	../../src/gait_pattern/APS.cpp
	../../src/gait_pattern/GaitAPS.cpp
	../../src/gait_pattern/GaitPatternAPS.cpp

)

# Add test cpp file
add_executable( runUnitTestsLimbCoordinator EXCLUDE_FROM_ALL ${LIMBCOORDINATOR_SRCS})
# Link test executable against gtest & gtest_main
target_link_libraries(runUnitTestsLimbCoordinator  gtest_main gtest pthread loco ${LOCO_LIBS} )
add_test( runUnitTestsLimbCoordinator ${EXECUTABLE_OUTPUT_PATH}/runUnitTestsLimbCoordinator )
add_dependencies(check runUnitTestsLimbCoordinator)
