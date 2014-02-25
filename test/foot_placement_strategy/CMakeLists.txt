
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

include_directories(${UTILS_INCL})
include_directories(${ROBOTMODEL_INCL})

#message(STATUS "robotModel: ${ROBOTMODEL_INCL}")

include_directories(${EIGEN_INCLUDE_DIRS})
include_directories(${Kindr_INCLUDE_DIRS})
include_directories(../../include)


include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(../../thirdparty/tinyxml)


set(FOOTPLACMENTSTRATEGY_SRCS
	../test_main.cpp
	FootPlacementStrategyTest.cpp
	
	../../src/foot_placement_strategy/FootPlacementStrategyBase.cpp
	../../src/foot_placement_strategy/FootPlacementStrategyInvertedPendulum.cpp
	
	../../src/limb_coordinator/LimbCoordinatorBase.cpp
	../../src/limb_coordinator/LimbCoordinatorDynamicGait.cpp
	
	../../src/common/TorsoBase.cpp
	../../src/common/TorsoStarlETH.cpp
	../../src/common/LegGroup.cpp
	../../src/common/LegBase.cpp
	../../src/common/LegStarlETH.cpp
	
	../../src/gait_pattern/APS.cpp
	../../src/gait_pattern/GaitAPS.cpp
	../../src/gait_pattern/GaitPatternAPS.cpp
	
	../../thirdparty/tinyxml/tinystr.cpp
	../../thirdparty/tinyxml/tinyxml.cpp
	../../thirdparty/tinyxml/tinyxmlerror.cpp
	../../thirdparty/tinyxml/tinyxmlparser.cpp
)


add_executable( runUnitTestsFootPlacementStrategy EXCLUDE_FROM_ALL ${FOOTPLACMENTSTRATEGY_SRCS})
target_link_libraries(runUnitTestsFootPlacementStrategy  gtest_main gtest pthread starlethRobotModel starlethUtils  )
add_test( runUnitTestsFootPlacementStrategy ${EXECUTABLE_OUTPUT_PATH}/runUnitTestsFootPlacementStrategy )
add_dependencies(check runUnitTestsFootPlacementStrategy)

