
set(SOURCES
	XCLMiner.h XCLMiner.cpp
	xcl/xcl2.cpp xcl/xcl2.hpp
)

#if(USE_SYS_OPENCL)
#	# On macOS or using ROCm-OpenCL, use system OpenCL library.
#	find_package(OpenCL REQUIRED)
#else()
#	hunter_add_package(OpenCL)
#	find_package(OpenCL CONFIG REQUIRED)
#endif()

set(XILINX_XRT /opt/xilinx/xrt)

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

