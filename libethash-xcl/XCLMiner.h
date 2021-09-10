//
// Created by Draiget on 6/4/2021.
//

#ifndef ETHMINER_XILINX_XCLMINER_H
#define ETHMINER_XILINX_XCLMINER_H

#include <libdevcore/Worker.h>
#include <libethcore/Miner.h>
#include <map>
#include <libethash-cuda/ethash_cuda_miner_kernel.h>

#include "xcl/xcl2.hpp"

struct search_result
{
    // One word for gid and 8 for mix hash
    uint32_t gid;
    uint32_t mix[8];
};

struct search_results
{
    search_result result;
    uint32_t count = 0;
};

namespace dev
{
    namespace eth
    {
        class XCLMiner : public Miner
        {
        public:
            XCLMiner(unsigned int _index, XCLSettings _settings, DeviceDescriptor &_device);
            ~XCLMiner() override;

            static void enumDevices(std::map<string, DeviceDescriptor>& _DevicesCollection);

            void search(uint8_t* header, uint64_t target, uint64_t _startN, const dev::eth::WorkPackage& w);
        protected:
            bool initDevice() override;

            bool initEpoch_internal() override;

            void kick_miner() override;

        private:
            void workLoop() override;

            std::vector<volatile search_results*> m_search_buf;
            uint64_t m_current_target = 0;
            atomic<bool> m_new_work = {false};

            const uint32_t m_batch_size;
            const uint32_t m_streams_batch_size;

            vector<cl::Context> m_context;
            vector<cl::CommandQueue> m_queue;
            vector<cl::CommandQueue> m_abortqueue;
            cl::Kernel m_searchKernel;
            cl::Kernel m_dagKernel;
            cl::Device m_device;

            vector<hash128_t*> m_dag_outputs;
            vector<hash128_t*> m_dag_buffer;
            vector<cl::Buffer> m_dag;
            vector<cl::Buffer> m_light;
            vector<cl::Buffer> m_header;
            vector<cl::Buffer> m_searchBuffer;

            void clear_buffer() {
                m_dag.clear();
                m_light.clear();
                m_header.clear();
                m_searchBuffer.clear();
                m_queue.clear();
                m_context.clear();
                m_abortqueue.clear();
            }

            XCLSettings m_settings;

            unsigned m_dagItems = 0;
            uint64_t m_lastNonce = 0;
        };
    }
}

#endif //ETHMINER_XILINX_XCLMINER_H
