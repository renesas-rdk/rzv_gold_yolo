cmake_minimum_required(VERSION 3.8)
project(rzv_gold_yolo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rzv_model REQUIRED)

# create target
add_library(${PROJECT_NAME} SHARED
  src/gold_yolo_model.cpp
)

# include directories
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  $<BUILD_INTERFACE:/usr/include/renesas/opencv4>
  $<BUILD_INTERFACE:$ENV{TVM_ROOT}/3rdparty/spdlog/include>
)

# ROS2 dependencies
ament_target_dependencies(${PROJECT_NAME}
  rzv_model
)

# Non-ROS2 dependencies
if(NOT DEFINED RENESAS_LIB_PATH) # Allow overriding via cross compilation
  set(RENESAS_LIB_PATH "/usr/lib/aarch64-linux-gnu/renesas")
endif()

target_link_libraries(${PROJECT_NAME}
  ${RENESAS_LIB_PATH}/libopencv_core.so
  ${RENESAS_LIB_PATH}/libopencv_imgproc.so
)

install(
  TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include/
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
