// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class IReliabilitySubsystemWrapper 
        {
        public:
            __declspec(property(get=get_WorkingDirectory)) std::wstring const & WorkingDirectory;
            virtual std::wstring const & get_WorkingDirectory() const  = 0;
            
            __declspec(property(get=get_NodeConfig)) Common::FabricNodeConfigSPtr const & NodeConfig;
            virtual Common::FabricNodeConfigSPtr const & get_NodeConfig() = 0;

            virtual void SendNodeUpToFM() = 0;
             
            virtual void SendNodeUpToFMM() = 0;

            virtual void UploadLFUM(
                Reliability::GenerationNumber const & generation,
                std::vector<Reliability::FailoverUnitInfo> && failoverUnitInfo,
                bool anyReplicaFound) = 0;

            virtual void UploadLFUMForFMReplicaSet(
                Reliability::GenerationNumber const & generation,
                std::vector<Reliability::FailoverUnitInfo> && failoverUnitInfo,
                bool anyReplicaFound) = 0;

            virtual void ForwardHealthReportFromRAPToHealthManager(
                Transport::MessageUPtr && messagePtr,
                Transport::IpcReceiverContextUPtr && ipcTransportContext) = 0;

            virtual Common::ErrorCode AddHealthReport(ServiceModel::HealthReport && report) = 0;

            virtual ~IReliabilitySubsystemWrapper()
            {
            }
        };
    }
}

