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

// WARNING: Do not change the value of the following constant
// unless you are prepared to make the neccessary adjustments
// to the assembly code for the binary kernels.
const size_t c_maxSearchResults = 4;

// NOTE: The following struct must match the one defined in
// ethash.cl
struct SearchResults
{
    struct
    {
        uint32_t gid;
        // Can't use h256 data type here since h256 contains
        // more than raw data. Kernel returns raw mix hash.
        uint32_t mix[8];
        uint32_t pad[7];  // pad to 16 words for easy indexing
    } rslt[c_maxSearchResults];
    uint32_t count;
    uint32_t hashCount;
    uint32_t abort;
};

dev::eth::XCLMiner::XCLMiner(
        unsigned int _index,
        CLSettings _settings,
        DeviceDescriptor &_device) : Miner("xcl-", _index), m_settings(std::move(_settings))
{
    m_deviceDescriptor = _device;
    m_settings.localWorkSize = ((m_settings.localWorkSize + 7) / 8) * 8;
    m_settings.globalWorkSize = m_settings.localWorkSize * m_settings.globalWorkSizeMultiplier;
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

        xclDeviceUsage  usage_info{};
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

    try {
        auto devices = vector<cl::Device>(&m_device, &m_device + 1);

        // create context
        m_context.clear();
        m_context.emplace_back(devices);
        m_queue.clear();
        m_queue.emplace_back(cl::CommandQueue(
                m_context[0],
                m_device,
                CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE));

        m_dagItems = m_epochContext.dagNumItems;

        // create buffer for dag
        try
        {
            xcllog << "Creating DAG buffer, size: "
                  << dev::getFormattedMemory((double)m_epochContext.dagSize)
                  << ", free: "
                  << dev::getFormattedMemory(
                          (double)(m_deviceDescriptor.totalMemory - RequiredMemory));
            m_dag.clear();

            if (m_epochContext.dagNumItems & 1)
            {
                m_dag.emplace_back(m_context[0], CL_MEM_READ_ONLY, m_epochContext.dagSize / 2 + 64);
                m_dag.emplace_back(m_context[0], CL_MEM_READ_ONLY, m_epochContext.dagSize / 2 - 64);
            }
            else
            {
                m_dag.emplace_back(
                        cl::Buffer(m_context[0], CL_MEM_READ_ONLY, (m_epochContext.dagSize) / 2));
                m_dag.emplace_back(
                        cl::Buffer(m_context[0], CL_MEM_READ_ONLY, (m_epochContext.dagSize) / 2));
            }

            xcllog << "Creating light cache buffer, size: "
                  << dev::getFormattedMemory((double)m_epochContext.lightSize);

            m_light.clear();
            bool light_on_host = false;
            try
            {
                m_light.emplace_back(m_context[0], CL_MEM_READ_ONLY, m_epochContext.lightSize);
            }
            catch (cl::Error const& err)
            {
                if ((err.err() == CL_OUT_OF_RESOURCES) || (err.err() == CL_OUT_OF_HOST_MEMORY))
                {
                    // Ok, no room for light cache on GPU. Try allocating on host
                    clog(WarnChannel) << "No room on GPU, allocating light cache on host";
                    clog(WarnChannel) << "Generating DAG will take minutes instead of seconds";
                    light_on_host = true;
                }
                else
                    throw;
            }

            if (light_on_host)
            {
                m_light.emplace_back(m_context[0], CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                     m_epochContext.lightSize);
                xcllog << "WARNING: Generating DAG will take minutes, not seconds";
            }
            xcllog << "Loading kernels";

            // Let's load our Xilinx kernel container
            cl::Context context(m_device);
            cl::CommandQueue q(context, m_device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);

            std::string xclbin_path = "/home/draiget/workspace-test/ethminer_system/Emulation-SW/binary_container_1.xclbin";
            cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
            devices.resize(1);
            cl::Program program(context, devices, xclBins);

            m_dagKernel = cl::Kernel(program, "generate_dag");

            //m_queue[0].enqueueWriteBuffer(
            //        m_light[0], CL_TRUE, 0, m_epochContext.lightSize, m_epochContext.lightCache);
        }
        catch (cl::Error const& err)
        {
            cwarn << ethCLErrorHelper("Creating DAG buffer failed", err);
            pause(MinerPauseEnum::PauseDueToInitEpochError);
            return true;
        }
    }
    catch (cl::Error const& err)
    {
        xcllog << ethCLErrorHelper("OpenCL init failed", err);
        pause(MinerPauseEnum::PauseDueToInitEpochError);
        return false;
    }
    return true;
}

void XCLMiner::kick_miner() {
    // Memory for abort Cannot be static because crashes on macOS.
    const uint32_t one = 1;
    if (!m_settings.noExit && !m_abortqueue.empty()) {
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
            // Read results.
            volatile SearchResults results;

            if (m_queue.size())
            {

            } else {
                results.count = 0;
            }

            // Wait for work or 3 seconds (whichever the first)
            const WorkPackage w = work();
            if (!w)
            {
                boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(3);
                boost::mutex::scoped_lock l(x_work);
                m_new_work_signal.timed_wait(l, timeout);
                continue;
            }

            if (current.header != w.header)
            {
                if (current.epoch != w.epoch)
                {
                    m_abortqueue.clear();

                    if (!initEpoch()) {
                        break;  // This will simply exit the thread
                    }

                    m_abortqueue.push_back(cl::CommandQueue(m_context[0], m_device));
                }

                // Upper 64 bits of the boundary.
                const uint64_t target = (uint64_t)(u64)((u256)w.boundary >> 192);
                assert(target > 0);

                startNonce = w.startNonce;
            }

            if (results.count)
            {
                // Report results while the kernel is running.
                for (uint32_t i = 0; i < results.count; i++)
                {
                    uint64_t nonce = current.startNonce + results.rslt[i].gid;
                    if (nonce != m_lastNonce)
                    {
                        m_lastNonce = nonce;
                        h256 mix;
                        memcpy(mix.data(), (char*)results.rslt[i].mix, sizeof(results.rslt[i].mix));

                        Farm::f().submitProof(Solution{
                                nonce, mix, current, std::chrono::steady_clock::now(), m_index});
                        xcllog << EthWhite << "Job: " << current.header.abridged() << " Sol: 0x"
                              << toHex(nonce) << EthReset;
                    }
                }
            }

            current = w;  // kernel now processing newest work
            current.startNonce = startNonce;
            // Increase start nonce for following kernel execution.
            startNonce += m_settings.globalWorkSize;
        }

        if (m_queue.size()) {
            m_queue[0].finish();
        }

        clear_buffer();
    }
    catch (cl::Error const& _e)
    {
        string _what = ethCLErrorHelper("OpenCL Error", _e);
        clear_buffer();
        throw std::runtime_error(_what);
    }
}


