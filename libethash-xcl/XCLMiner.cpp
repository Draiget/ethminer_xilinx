//
// Created by Draiget on 6/4/2021.
//

#include "XCLMiner.h"

#include <libethcore/Farm.h>
#include <ethash/ethash.hpp>
#include <utility>
#include <xrt.h>

using namespace dev;
using namespace eth;

namespace dev {
    namespace eth {
        struct XCLChannel : public LogChannel
        {
            static const char* name() { return EthOrange "xl"; }
            static const int verbosity = 2;
            static const bool debug = false;
        };
    }
}

#define xcllog clog(XCLChannel)

#define XILINX_MEMORY_PER_HBM_BANK_MB 256
#define XILINX_MEMORY_BANKS_ALLOWED 24
#define ETHASH_NODE_WORDS (64 / 4)

enum XilinxGenerateDagKernelOffsets {
    XILINX_OFFSET_GEN_DAG_BANK_INDEX = XILINX_MEMORY_BANKS_ALLOWED,
    XILINX_OFFSET_GEN_DAG_COUNT,
    XILINX_OFFSET_GEN_DAG_LIGHT,
    XILINX_OFFSET_GEN_DAG_LIGHT_SIZE,
};

enum XilinxSearchKernelOffsets {
    XILINX_OFFSET_SEARCH_BANK_INDEX = XILINX_MEMORY_BANKS_ALLOWED - 1,
    XILINX_OFFSET_SEARCH_OUTPUT,
    XILINX_OFFSET_SEARCH_START_NONCE,
    XILINX_OFFSET_SEARCH_TARGET,
    XILINX_OFFSET_SEARCH_HEADER,
};

bool xcl_check(cl_int result, const char* msg){
    if (result != CL_SUCCESS){
        xcllog << msg;
        return false;
    }

    return true;
}

/**
 * Returns the name of a numerical cl_int error
 * Takes constants from CL/cl.h and returns them in a readable format
 */
static const char* strClError(cl_int err)
{
    switch (err)
    {
        case CL_SUCCESS:
            return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND:
            return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE:
            return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE:
            return "CL_COMPILER_NOT_AVAILABLE";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case CL_OUT_OF_RESOURCES:
            return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY:
            return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP:
            return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH:
            return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_BUILD_PROGRAM_FAILURE:
            return "CL_BUILD_PROGRAM_FAILURE";
        case CL_MAP_FAILURE:
            return "CL_MAP_FAILURE";
        case CL_MISALIGNED_SUB_BUFFER_OFFSET:
            return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
            return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";

#ifdef CL_VERSION_1_2
        case CL_COMPILE_PROGRAM_FAILURE:
            return "CL_COMPILE_PROGRAM_FAILURE";
        case CL_LINKER_NOT_AVAILABLE:
            return "CL_LINKER_NOT_AVAILABLE";
        case CL_LINK_PROGRAM_FAILURE:
            return "CL_LINK_PROGRAM_FAILURE";
        case CL_DEVICE_PARTITION_FAILED:
            return "CL_DEVICE_PARTITION_FAILED";
        case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
            return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
#endif  // CL_VERSION_1_2

        case CL_INVALID_VALUE:
            return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE:
            return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_PLATFORM:
            return "CL_INVALID_PLATFORM";
        case CL_INVALID_DEVICE:
            return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT:
            return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES:
            return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE:
            return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_HOST_PTR:
            return "CL_INVALID_HOST_PTR";
        case CL_INVALID_MEM_OBJECT:
            return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE:
            return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER:
            return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY:
            return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS:
            return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PROGRAM:
            return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE:
            return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME:
            return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION:
            return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL:
            return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX:
            return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE:
            return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE:
            return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS:
            return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION:
            return "CL_INVALID_WORK_DIMENSION";
        case CL_INVALID_WORK_GROUP_SIZE:
            return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE:
            return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET:
            return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST:
            return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT:
            return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION:
            return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT:
            return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE:
            return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL:
            return "CL_INVALID_MIP_LEVEL";
        case CL_INVALID_GLOBAL_WORK_SIZE:
            return "CL_INVALID_GLOBAL_WORK_SIZE";
        case CL_INVALID_PROPERTY:
            return "CL_INVALID_PROPERTY";

#ifdef CL_VERSION_1_2
        case CL_INVALID_IMAGE_DESCRIPTOR:
            return "CL_INVALID_IMAGE_DESCRIPTOR";
        case CL_INVALID_COMPILER_OPTIONS:
            return "CL_INVALID_COMPILER_OPTIONS";
        case CL_INVALID_LINKER_OPTIONS:
            return "CL_INVALID_LINKER_OPTIONS";
        case CL_INVALID_DEVICE_PARTITION_COUNT:
            return "CL_INVALID_DEVICE_PARTITION_COUNT";
#endif  // CL_VERSION_1_2

#ifdef CL_VERSION_2_0
        case CL_INVALID_PIPE_SIZE:
            return "CL_INVALID_PIPE_SIZE";
        case CL_INVALID_DEVICE_QUEUE:
            return "CL_INVALID_DEVICE_QUEUE";
#endif  // CL_VERSION_2_0

#ifdef CL_VERSION_2_2
            case CL_INVALID_SPEC_ID:
        return "CL_INVALID_SPEC_ID";
    case CL_MAX_SIZE_RESTRICTION_EXCEEDED:
        return "CL_MAX_SIZE_RESTRICTION_EXCEEDED";
#endif  // CL_VERSION_2_2
    }

    std::ostringstream osstream;
    osstream << "Unknown CL error (id = " << err << ") encountered";
    return osstream.str().c_str();
}

/**
 * Prints cl::Errors in a uniform way
 * @param msg text prepending the error message
 * @param clerr cl:Error object
 *
 * Prints errors in the format:
 *      msg: what(), string err() (numeric err())
 */
static std::string ethCLErrorHelper(const char* msg, cl::Error const& clerr)
{
    std::ostringstream osstream;
    osstream << msg << ": " << clerr.what() << ": " << strClError(clerr.err()) << " (" << clerr.err() << ")";
    return osstream.str();
}


dev::eth::XCLMiner::XCLMiner(
        unsigned int _index,
        XCLSettings _settings,
        DeviceDescriptor &_device)
        : Miner("xcl-", _index),
        m_settings(std::move(_settings)),
        m_batch_size(_settings.search_lookup_size),
        m_streams_batch_size(_settings.search_lookup_size * _settings.search_streams)
{
    m_deviceDescriptor = _device;
}

dev::eth::XCLMiner::~XCLMiner() {
    DEV_BUILD_LOG_PROGRAMFLOW(xcllog, "xcl-" << m_index << " XCLMiner::~XCLMiner() begin");
    stopWorking();
    kick_miner();
    DEV_BUILD_LOG_PROGRAMFLOW(xcllog, "xcl-" << m_index << " XCLMiner::~XCLMiner() end");
}

void dev::eth::XCLMiner::enumDevices(map<string, DeviceDescriptor> &_DevicesCollection) {
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    if (devices.empty()){
        return;
    }

    unsigned int xrt_device_count = xclProbe();
    for (unsigned int device_id = 0; device_id < xrt_device_count; device_id++){
        auto device_handle = xclOpen(device_id, nullptr, xclVerbosityLevel::XCL_ERROR);
        if (device_handle == XRT_NULL_HANDLE){
            continue;
        }

        xclDeviceUsage usage_info{};
        if (xclGetUsageInfo(device_handle, &usage_info) == 0){
            ostringstream s;
            s << "XRT_Device, memSize=" << usage_info.memSize << ", "
              << "ddrMemUsed=" << usage_info.ddrMemUsed;
            xcllog << s.str();
        }

        xclDeviceInfo2 device_info{};
        if (xclGetDeviceInfo2(device_handle, &device_info) == 0){
            ostringstream s;
            s << "XRT_Device, ddr_banks=" << device_info.mDDRBankCount << ", "
              << "ddr_size=" << device_info.mDDRSize << ", "
              << "chipTemp=" << device_info.mOnChipTemp;
            xcllog << s.str();
        }

        xclClose(device_handle);
    }

    unsigned int device_ordinal = 0;
    for (auto const& device : devices){
        string uniqueId;
        DeviceDescriptor deviceDescriptor;

        std::ostringstream s;
        s << device_ordinal;
        uniqueId = s.str();

        deviceDescriptor.name = device.getInfo<CL_DEVICE_NAME>();
        deviceDescriptor.type = DeviceTypeEnum::FPGA;
        deviceDescriptor.uniqueId = uniqueId;
        deviceDescriptor.clDetected = true;
        deviceDescriptor.clPlatformId = 0;
        deviceDescriptor.clPlatformName = device.getInfo<CL_DEVICE_VENDOR>();
        deviceDescriptor.clPlatformType = ClPlatformTypeEnum::Xilinx;
        deviceDescriptor.clDeviceOrdinal = device_ordinal;

        // TODO: Obtain from FPGA
        deviceDescriptor.clPlatformVersion = "unknown";
        deviceDescriptor.clPlatformVersionMajor = 0;
        deviceDescriptor.clPlatformVersionMinor = 0;

        deviceDescriptor.clName = deviceDescriptor.name;
        deviceDescriptor.clDeviceVersion = device.getInfo<CL_DEVICE_VERSION>();
        deviceDescriptor.clDeviceVersionMajor = std::stoi(deviceDescriptor.clDeviceVersion.substr(7, 1));
        deviceDescriptor.clDeviceVersionMinor = std::stoi(deviceDescriptor.clDeviceVersion.substr(9, 1));
        deviceDescriptor.totalMemory = (size_t)(256 * 32) * 1024 * 1024;
        deviceDescriptor.clMaxMemAlloc = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
        deviceDescriptor.clMaxWorkGroup = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
        deviceDescriptor.clMaxComputeUnits = 0;

        _DevicesCollection[uniqueId] = deviceDescriptor;
        device_ordinal++;
    }
}

bool XCLMiner::initDevice() {
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    if (devices.empty()){
        return false;
    }

    m_device = devices.at(m_deviceDescriptor.clDeviceOrdinal);

    ostringstream s;
    s << "Using Device : " << m_deviceDescriptor.uniqueId << " " << m_deviceDescriptor.clName;

    s << " Memory : " << dev::getFormattedMemory((double)m_deviceDescriptor.totalMemory);
    s << " (" << m_deviceDescriptor.totalMemory << " B)";
    xcllog << s.str();

    return true;
}

bool XCLMiner::initEpoch_internal() {
    auto startInit = std::chrono::steady_clock::now();
    size_t RequiredMemory = (m_epochContext.dagSize);

    // Release the pause flag if any
    resume(MinerPauseEnum::PauseDueToInsufficientMemory);
    resume(MinerPauseEnum::PauseDueToInitEpochError);

    // Check whether the current device has sufficient memory every time we recreate the dag
    if (m_deviceDescriptor.totalMemory < RequiredMemory)
    {
        xcllog << "Epoch " << m_epochContext.epochNumber << " requires "
              << dev::getFormattedMemory((double)RequiredMemory) << " memory. Only "
              << dev::getFormattedMemory((double)m_deviceDescriptor.totalMemory)
              << " available on device.";
        pause(MinerPauseEnum::PauseDueToInsufficientMemory);
        return true;  // This will prevent to exit the thread and
        // Eventually resume mining when changing coin or epoch (NiceHash)
    }

    xcllog << "Generating split DAG + Light (total): "
          << dev::getFormattedMemory((double)RequiredMemory);

//#define ZONTWELG_DUMP_DAG

#ifdef ZONTWELG_DUMP_DAG
    std::ofstream dag_stream ("/root/light_data.bin", std::ios::out | std::fstream::binary);
    dag_stream.write((char*)&m_epochContext.lightNumItems, sizeof(m_epochContext.lightNumItems));

    for (auto i = 0; i < m_epochContext.lightNumItems; i++){
        dag_stream.write((char *) &m_epochContext.lightCache[i].bytes[0], sizeof(m_epochContext.lightCache[i].bytes));
    }

    dag_stream.close();

    xcllog << "m_epochContext.dagNumItems: " << m_epochContext.dagNumItems;
    xcllog << "m_epochContext.lightNumItems: " << m_epochContext.lightNumItems;
    xcllog << "m_epochContext.epochNumber: " << m_epochContext.epochNumber;
    xcllog << "m_epochContext.lightSize: " << m_epochContext.lightSize;
    xcllog << "m_epochContext.dagSize: " << m_epochContext.dagSize;
    xcllog << "m_settings.search_lookup_size: " << m_settings.search_lookup_size;
    xcllog << "m_settings.search_streams: " << m_settings.search_streams;
#endif

#ifndef XILINX_ENABLE_DAG_GENERATION
    try {
        auto devices = vector<cl::Device>(&m_device, &m_device + 1);

        // create context
        m_context.clear();
        m_context.emplace_back(devices);
        m_queue.clear();
        m_queue.emplace_back(cl::CommandQueue(
                m_context[0],
                m_device,
                CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)); // CL_QUEUE_PROFILING_ENABLE

        m_dagItems = m_epochContext.dagNumItems;

        const auto hbm_size_bits = 1024 * 1024 * 256;

        auto dag_bits_per_hbm_bank = (uint64_t)XILINX_MEMORY_PER_HBM_BANK_MB * 1024 * 1024;
        auto consumed_hbm_banks = (uint32_t)(m_epochContext.dagSize / ((uint64_t)XILINX_MEMORY_PER_HBM_BANK_MB * 1024 * 1024));
        auto extra_hbm_bank = m_epochContext.dagSize % ((uint64_t)XILINX_MEMORY_PER_HBM_BANK_MB * 1024 * 1024);
        auto total_banks = consumed_hbm_banks + (extra_hbm_bank > 0 ? 1 : 0);

        xcllog << "Using "
                << consumed_hbm_banks << " HBM banks ("
                << XILINX_MEMORY_PER_HBM_BANK_MB << " MB per bank), extra: " << extra_hbm_bank;

        // create buffer for dag
        try
        {
            xcllog << "Creating DAG buffer, size: "
                  << dev::getFormattedMemory((double)m_epochContext.dagSize)
                  << ", free: "
                  << dev::getFormattedMemory(
                          (double)(m_deviceDescriptor.totalMemory - RequiredMemory));

            m_dag.clear();

            for (auto dag_arg_index = 0; dag_arg_index < XILINX_MEMORY_BANKS_ALLOWED; ++dag_arg_index){
                // Split DAG to different buffers where each one will fit to specific HBM block
                if (dag_arg_index < consumed_hbm_banks){
                    m_dag.emplace_back(
                            m_context[0],
                            CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            (size_t)dag_bits_per_hbm_bank,
                            nullptr);

                    continue;
                }

                // Extra bank if first ones does not fit whole DAG size
                if (dag_arg_index == consumed_hbm_banks && extra_hbm_bank > 0){
                    m_dag.emplace_back(
                            m_context[0],
                            CL_MEM_READ_WRITE,
                            (size_t)extra_hbm_bank,
                            nullptr);

                    continue;
                }

                // For other banks that are not in use, create minimal buffer only
                m_dag.emplace_back(m_context[0], CL_MEM_READ_WRITE, (size_t)1, nullptr);
            }

            xcllog << "Creating light cache buffer, size: "
                  << dev::getFormattedMemory((double)m_epochContext.lightSize);

            m_light.clear();
            try
            {
                m_light.emplace_back(m_context[0], CL_MEM_READ_ONLY, (size_t)m_epochContext.lightSize, nullptr);
            }
            catch (cl::Error const& err)
            {
                if ((err.err() == CL_OUT_OF_RESOURCES) || (err.err() == CL_OUT_OF_HOST_MEMORY))
                {
                    clog(WarnChannel) << "Error allocating Light cache, out of CL/HBM memory";
                }
                else
                {
                    throw;
                }
            }
        }
        catch (cl::Error const& err)
        {
            cwarn << ethCLErrorHelper("Creating DAG buffer failed", err);
            pause(MinerPauseEnum::PauseDueToInitEpochError);
            return true;
        }

        auto const max_n = (uint32_t)(m_epochContext.dagSize / ETHASH_NODE_WORDS * 4);
        xcllog << "Loading kernels";

        // Loading primary Xilinx kernel container with bitstream
        std::string xcl_bin_path =
                "/home/draiget/workspace-test/ethminer_system_hw_link/Hardware/binary_container_1.xclbin";

        cl::Program::Binaries xclBins = xcl::import_binary_file(xcl_bin_path);
        devices.resize(1);
        cl::Program program(m_context[0], devices, xclBins);

        xcllog << "Loading FPGA kernels";

        m_dagKernel = cl::Kernel(program, "generate_dag");
        m_searchKernel = cl::Kernel(program, "search");

        // Set DAG kernel arguments
        for (auto dag_arg_index = 0; dag_arg_index < XILINX_MEMORY_BANKS_ALLOWED; ++dag_arg_index){
            auto r = m_dagKernel.setArg(dag_arg_index, m_dag[dag_arg_index]);
            if (r != CL_SUCCESS){
                xcllog << "Setting generate dag kernel argument #" << dag_arg_index << " failed - " << r;
                return false;
            }

            r = m_searchKernel.setArg(dag_arg_index, m_dag[dag_arg_index]);
            if (r != CL_SUCCESS){
                xcllog << "Setting search kernel argument #" << dag_arg_index << " failed - " << r;
                return false;
            }
        }

        // Set light kernel arguments
        if (!xcl_check(
                m_dagKernel.setArg(
                        XILINX_OFFSET_GEN_DAG_LIGHT,
                        m_light[0]),
                "Failed to set light buffer"))
        {
            return false;
        }

        if (!xcl_check(
                m_dagKernel.setArg(
                        XILINX_OFFSET_GEN_DAG_LIGHT_SIZE,
                        (uint32_t)m_epochContext.lightSize),
                "Failed to set lightSize"))
        {
            return false;
        }

        // Take all memory pointers to single vector
        //std::vector<cl::Memory> memory_batch(total_banks + 1);
        //for (auto i = 0; i < total_banks; i++) {
        //    memory_batch[i] = m_dag[i];
        //}
        //memory_batch[total_banks] = m_light[0];

        // Migrate memory
        //xcllog << "Migrating memory buffers to FPGA";
        //cl::Event memory_event;
        //m_queue[0].enqueueMigrateMemObjects(memory_batch, 0, NULL, &memory_event);
        //clWaitForEvents(1, (const cl_event *)&memory_event);

        m_queue[0].enqueueWriteBuffer(
                m_light[0],
                CL_TRUE,
                0,
                m_epochContext.lightSize,
                m_epochContext.lightCache);

        cl::Event event_sp;

        // Run kernels for each bank
        auto elements_per_hbm_bank = dag_bits_per_hbm_bank / sizeof(hash64_t);
        for (auto i = 0; i < total_banks; i++){

            uint32_t bank_elements_count;
            if (extra_hbm_bank > 0 && i == total_banks - 1){
                bank_elements_count = extra_hbm_bank / sizeof(hash64_t);
            } else {
                bank_elements_count = elements_per_hbm_bank;
            }

            m_dagKernel.setArg(XILINX_OFFSET_GEN_DAG_BANK_INDEX, (uint32_t)i);
            m_dagKernel.setArg(XILINX_OFFSET_GEN_DAG_COUNT, (uint32_t)bank_elements_count);

            // Run kernel
            m_queue[0].enqueueNDRangeKernel(m_dagKernel, cl::NullRange, 1, 1, nullptr);
        }

        xcllog << "Generate dag compute items queued, waiting to finish ...";
        m_queue[0].finish();

        auto dagTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startInit);
        xcllog << dev::getFormattedMemory((double)m_epochContext.dagSize)
               << " of DAG data generated in "
               << dagTime.count() << " ms.";

        // Copy dag back?
        m_dag_outputs.resize(XILINX_MEMORY_BANKS_ALLOWED);
        for (auto i = 0; i < XILINX_MEMORY_BANKS_ALLOWED; ++i){
            // Split DAG to different buffers where each one will fit to specific HBM block
            if (i < consumed_hbm_banks){
                m_dag_outputs[i] = (hash128_t *)m_queue[0].enqueueMapBuffer(
                        m_dag[0],
                        CL_TRUE,
                        CL_MAP_READ,
                        0,
                        (size_t)dag_bits_per_hbm_bank);

                continue;
            }

            // Extra bank if first ones does not fit whole DAG size
            if (i == consumed_hbm_banks && extra_hbm_bank > 0){
                m_dag_outputs[i] = (hash128_t *)m_queue[0].enqueueMapBuffer(
                        m_dag[0],
                        CL_TRUE,
                        CL_MAP_READ,
                        0,
                        (size_t)extra_hbm_bank);

                continue;
            }

            // For other banks that are not in use, create minimal buffer only
            m_dag_outputs[i] = (hash128_t *)m_queue[0].enqueueMapBuffer(
                    m_dag[0],
                    CL_TRUE,
                    CL_MAP_READ,
                    0,
                    (size_t)1);
        }

        xcllog << "Dag memory pointer stored.";
    }
    catch (cl::Error const& err)
    {
        xcllog << ethCLErrorHelper("OpenCL init failed", err);
        pause(MinerPauseEnum::PauseDueToInitEpochError);
        return false;
    }
#endif

    return true;
}

void XCLMiner::kick_miner() {
    // Memory for abort Cannot be static because crashes on macOS.
    const uint32_t one = 1;
    if (!m_abortqueue.empty()) {
        //m_abortqueue[0].enqueueWriteBuffer(
        //        m_searchBuffer[0], CL_TRUE, offsetof(SearchResults, abort), sizeof(one), &one);
    }

    m_new_work_signal.notify_one();
}

void XCLMiner::workLoop() {
    // Memory for zero-ing buffers. Cannot be static or const because crashes on macOS.
    uint32_t zerox3[3] = {0, 0, 0};

    uint64_t startNonce = 0;

    // The work package currently processed by GPU.
    WorkPackage current;
    current.header = h256();

    if (!initDevice()) {
        return;
    }

    try
    {
        while (!shouldStop())
        {
            // Wait for work or 3 seconds (whichever the first)
            const WorkPackage w = work();
            if (!w) {
                boost::system_time const timeout =
                        boost::get_system_time() + boost::posix_time::seconds(3);
                boost::mutex::scoped_lock l(x_work);
                m_new_work_signal.timed_wait(l, timeout);
                continue;
            }

            // Epoch change ?
            if (current.epoch != w.epoch) {
                if (!initEpoch()) {
                    break;  // This will simply exit the thread
                }

                // As DAG generation takes a while we need to
                // ensure we're on latest job, not on the one
                // which triggered the epoch change
                current = w;
                continue;
            }

            // Persist most recent job.
            // Job's differences should be handled at higher level
            current = w;
            uint64_t upper64OfBoundary = (uint64_t)(u64)((u256)current.boundary >> 192);

            // Eventually start searching
            search(current.header.data(), upper64OfBoundary, current.startNonce, w);
        }

        m_queue[0].finish();
        clear_buffer();
    }
    catch (cl::Error const& _e)
    {
        string _what = ethCLErrorHelper("OpenCL Error", _e);
        clear_buffer();
        throw std::runtime_error(_what);
    }
}

union hash256_t
{
    uint64_t word64s[4];
    uint32_t word32s[8];
    uint8_t bytes[32];
    char str[32];
};

#define MAX_NONCE_SEARCH 8192 * 128

void XCLMiner::search(uint8_t* header, uint64_t target, uint64_t start_nonce, const WorkPackage &w) {
    // auto const search_header = *reinterpret_cast<hash32_t*>(header);
    auto target_header = *(hash256_t*)&header;
    auto const batch_size = MAX_NONCE_SEARCH;

    if (m_current_target != target){
        m_current_target = target;
    }

    m_searchKernel.setArg(XILINX_OFFSET_SEARCH_HEADER, target_header);
    m_searchKernel.setArg(XILINX_OFFSET_SEARCH_TARGET, (uint64_t)target);

    std::vector<cl::Buffer> search_buffers(m_settings.search_streams);
    for (auto i = 0; i < m_settings.search_streams; i++){
        search_buffers[i] = cl::Buffer(
                m_context[0],
                CL_MEM_READ_WRITE,
                (size_t)sizeof(search_results),
                nullptr);
    }

    std::vector<cl::Event> search_events(m_settings.search_streams);

    // Prime each stream, clear search result buffers and start the search
    for (auto current_index = 0; current_index < m_settings.search_streams;
         current_index++, start_nonce += m_batch_size)
    {
        // Set search output kernel argument
        m_searchKernel.setArg(XILINX_OFFSET_SEARCH_OUTPUT, search_buffers[current_index]);
        m_searchKernel.setArg(XILINX_OFFSET_SEARCH_START_NONCE, (uint64_t)start_nonce);

        // Migrate dags memory and output buffer
        std::vector<cl::Memory> memory_batch(XILINX_MEMORY_BANKS_ALLOWED + 1);
        for (auto i = 0; i < XILINX_MEMORY_BANKS_ALLOWED; i++) {
            memory_batch[i] = m_dag[i];
        }
        memory_batch[XILINX_OFFSET_SEARCH_OUTPUT] = search_buffers[current_index];

        cl::Event memory_migrate_event;
        m_queue[0].enqueueMigrateMemObjects(memory_batch, 0, nullptr, &memory_migrate_event);
        clWaitForEvents(1, (const cl_event *)&memory_migrate_event);

        // Queue and run search function
        //auto err = m_queue[0].enqueueNDRangeKernel(
        //        m_searchKernel,
        //        cl::NullRange,
        //        1,
        //        1,
        //        nullptr,
        //        &search_events[current_index]);
        auto err = m_queue[0].enqueueTask(m_searchKernel, nullptr, &search_events[current_index]);
        if (err != CL_SUCCESS){
            xcllog << "Failed to enqueue search kernel! Error: " << err;
        }
    }

    // process stream batches until we get new work.
    bool done = false;

    while (!done)
    {
        // Exit next time around if there's new work awaiting
        bool t = true;
        done = m_new_work.compare_exchange_strong(t, false);

        // Check on every batch if we need to suspend mining
        if (!done) {
            done = paused();
        }

        // This inner loop will process each FPGA kernels individually
        for (auto current_index = 0; current_index < m_settings.search_streams;
             current_index++, start_nonce += m_batch_size)
        {
            // Each pass of this loop will wait for a kernel to exit,
            // save any found solutions, then restart the kernel
            // on the next group of nonces.
            clWaitForEvents(1, (const cl_event *) &search_events[current_index]);
            m_queue[0].finish();

            if (shouldStop()) {
                m_new_work.store(false, std::memory_order_relaxed);
                done = true;
            }

            h256 mix;

            auto *output_results = (search_results *)m_queue[0].enqueueMapBuffer(
                    search_buffers[current_index],
                    CL_TRUE,
                    CL_MAP_READ,
                    0,
                    (size_t)sizeof(search_results) * 1);

            if (output_results->count > 0){
                memcpy(mix.data(), (void*)&output_results[0].result.mix, sizeof(output_results[0].result.mix));
            }

            // restart the stream on the next batch of nonces
            // unless we are done for this round.
            if (!done) {
                //m_queue[0].enqueueNDRangeKernel(
                //        m_searchKernel,
                //        cl::NullRange,
                //        1,
                //        1,
                //        nullptr,
                //        &search_events[current_index]);
                m_queue[0].enqueueTask(m_searchKernel, nullptr, &search_events[current_index]);
            }

            if (output_results->count > 0) {
                uint64_t nonce_base = start_nonce - m_streams_batch_size;
                uint64_t nonce = nonce_base + output_results->result.gid;

                Farm::f().submitProof(
                        Solution{nonce, mix, w, std::chrono::steady_clock::now(), m_index});

                xcllog << EthWhite << "Job: " << w.header.abridged() << " Sol: 0x"
                        << toHex(nonce) << EthReset;
            }
        }


        // Update the hash rate
        updateHashRate(m_batch_size, m_settings.search_streams);

        // Bail out if it's shutdown time
        if (shouldStop()) {
            m_new_work.store(false, std::memory_order_relaxed);
            break;
        }
    }
}


