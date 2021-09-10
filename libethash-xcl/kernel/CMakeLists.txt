#add_custom_target(xilinx-kernel COMMAND make all
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
#)

set(SOURCES
        src/ethminer_kernel_dag.cpp
        src/ethminer_kernel_dag.hpp
        src/ethminer_kernel_search.hpp
        src/ethminer_kernel_search.cpp
        src/ethminer_definitions.h
        src/ethminer_hashing.cpp
        )

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(..)

add_library(ethash-xcl-kernel ${SOURCES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(..)

target_link_libraries(ethash-xcl-kernel PUBLIC ethcore ethash::ethash)
target_link_libraries(ethash-xcl-kernel PRIVATE Boost::filesystem Boost::thread)
target_link_libraries(ethash-xcl-kernel PRIVATE ${XILINX_XRT}/lib/libxilinxopencl.so)
target_link_libraries(ethash-xcl-kernel PRIVATE ${XILINX_XRT}/lib/libxrt_core.so)
target_link_libraries(ethash-xcl-kernel PRIVATE ${XILINX_XRT}/lib/libxrt_coreutil.so)
target_link_libraries(ethash-xcl-kernel PRIVATE ${XILINX_XRT}/lib/libxrt++.so)
