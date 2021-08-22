//
// Created by Draiget on 6/4/2021.
//

#ifndef ETHMINER_XILINX_XCLMINER_H
#define ETHMINER_XILINX_XCLMINER_H

#include <libdevcore/Worker.h>
#include <libethcore/EthashAux.h>
#include <libethcore/Miner.h>

#include "xcl/xcl2.hpp"

namespace dev
{
    namespace eth
    {
        class XCLMiner : public Miner
        {
        public:
            XCLMiner(unsigned int _index, CLSettings _settings, DeviceDescriptor &_device);
            ~XCLMiner() override;

            static void enumDevices(std::map<string, DeviceDescriptor>& _DevicesCollection);

        protected:
            bool initDevice() override;

            bool initEpoch_internal() override;

            void kick_miner() override;

        private:
            void workLoop() override;

            vector<cl::Context> m_context;
            vector<cl::CommandQueue> m_queue;
            vector<cl::CommandQueue> m_abortqueue;
            cl::Kernel m_searchKernel;
            cl::Kernel m_dagKernel;
            cl::Device m_device;

            vector<cl::Buffer> m_dag;
            vector<void*> m_dagRawBuffers;
            vector<cl::Memory> m_generateDagBuffer;
            vector<cl::Buffer> m_light;
            vector<cl::Buffer> m_header;
            vector<cl::Buffer> m_searchBuffer;

            void clear_buffer() {
                m_dag.clear();
                m_generateDagBuffer.clear();
                m_light.clear();
                m_header.clear();
                m_searchBuffer.clear();
                m_queue.clear();
                m_context.clear();
                m_abortqueue.clear();
            }

            CLSettings m_settings;

            unsigned m_dagItems = 0;
            uint64_t m_lastNonce = 0;
        };
    }
}

#endif //ETHMINER_XILINX_XCLMINER_H
