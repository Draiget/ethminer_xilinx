
set(SOURCES
	XCLMiner.h XCLMiner.cpp
	xcl/xcl2.cpp xcl/xcl2.hpp
		kernel/src/ethminer_kernel.cpp
		kernel/src/ethminer_kernel.hpp
		kernel/src/dag_test.cpp
)

#if(USE_SYS_OPENCL)
#	# On macOS or using ROCm-OpenCL, use system OpenCL library.
#	find_package(OpenCL REQUIRED)
#else()
#	hunter_add_package(OpenCL)
#	find_package(OpenCL CONFIG REQUIRED)
#endif()

set(XILINX_XRT /opt/xilinx/xrt)

# -std=c++0x -DVITIS_PLATFORM=$(VITIS_PLATFORM) -D__USE_XOPEN2K8 -I/opt/xilinx/xrt/include/ -I/tools/Xilinx/Vivado/2020.2/include/ -O0 -g -Wall -c -fmessage-length=0

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(..)
include_directories(${XILINX_XRT}/include)
include_directories(/tools/Xilinx/Vivado/2020.2/include)

add_library(ethash-xcl ${SOURCES})
target_link_directories(ethash-xcl PRIVATE ${XILINX_XRT}/lib)

#LINK_DIRECTORIES(${XILINX_XRT}/lib)

target_link_libraries(ethash-xcl PUBLIC ethcore ethash::ethash)
# target_link_libraries(ethash-xcl PRIVATE OpenCL::OpenCL)
target_link_libraries(ethash-xcl PRIVATE Boost::filesystem Boost::thread)
#target_link_libraries(ethash-xcl PRIVATE xilinxopencl)
target_link_libraries(ethash-xcl PRIVATE ${XILINX_XRT}/lib/libxilinxopencl.so)
target_link_libraries(ethash-xcl PRIVATE ${XILINX_XRT}/lib/libxrt_core.so)
target_link_libraries(ethash-xcl PRIVATE ${XILINX_XRT}/lib/libxrt_coreutil.so)
target_link_libraries(ethash-xcl PRIVATE ${XILINX_XRT}/lib/libxrt++.so)

add_subdirectory(kernel)
# FPGA --------------
# Kernels compile:
# make -j24 all
# /tools/Xilinx/Vitis/2020.2/bin/v++ --target hw --compile -I"../src" --config generate_dag_test-compile.cfg -o"build/generate_dag_test.xo" "../src/ethminer_kernel.cpp"
#
# HW link:
# make all
#/ tools/Xilinx/Vitis/2020.2/bin/v++ --target hw --link --config binary_container_1-link.cfg -o"binary_container_1.xclbin" ../../ethminer_kernels/Hardware/build/generate_dag_test.xo