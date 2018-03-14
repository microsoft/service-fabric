// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            // ReliabilityStub for RA Unit Tests
            class ReliabilitySubsystemStub : public IReliabilitySubsystemWrapper
            {
                DENY_COPY(ReliabilitySubsystemStub);
            public:
                struct UploadLfumInfo
                {
                    Reliability::GenerationNumber GenerationNumber;
                    std::vector<Reliability::FailoverUnitInfo> FailoverUnits;
                    bool AnyReplicaFound;
                };

                ReliabilitySubsystemStub(Common::FabricNodeConfigSPtr const & nodeConfig, std::wstring const & workingDirectory);

                std::wstring const & get_WorkingDirectory() const
                {
                    return workingDirectory_;
                }

                Common::FabricNodeConfigSPtr const & get_NodeConfig()
                {
                    return nodeConfig_;
                }

                __declspec(property(get = get_NodeUpToFMCount)) int NodeUpToFMCount;
                int get_NodeUpToFMCount() const { return nodeUpToFMCount_; }

                __declspec(property(get = get_NodeUpToFMMCount)) int NodeUpToFMMCount;
                int get_NodeUpToFMMCount() const { return nodeUpToFMMCount_; }

                __declspec(property(get = get_FMLfumUploads)) std::vector<UploadLfumInfo> const & FMLfumUploads;
                std::vector<UploadLfumInfo> const & get_FMLfumUploads() const { return fmLfumUploads_; }

                __declspec(property(get = get_FmmLfumUploads)) std::vector<UploadLfumInfo> const & FmmLfumUploads;
                std::vector<UploadLfumInfo> const & get_FmmLfumUploads() const { return fmmLfumUploads_; }

                void SendNodeUpToFM();

                void SendNodeUpToFMM();

                void UploadLFUM(
                    Reliability::GenerationNumber const & generation,
                    std::vector<Reliability::FailoverUnitInfo> && failoverUnitInfo,
                    bool anyReplicaFound);

                void UploadLFUMForFMReplicaSet(
                    Reliability::GenerationNumber const & generation,
                    std::vector<Reliability::FailoverUnitInfo> && failoverUnitInfo,
                    bool anyReplicaFound);

                void Reset();

                Reliability::FederationWrapper & get_FederationWrapper();
                Reliability::ServiceAdminClient & get_ServiceAdminClient();
                Reliability::ServiceResolver & get_ServiceResolver();
                Federation::FederationSubsystem & get_FederationSubsystem() const;
                void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient);
                Common::ErrorCode SetSecurity(Transport::SecuritySettings const& value);
                Common::HHandler RegisterFailoverManagerReadyEvent(Common::EventHandler const & handler);
                bool UnRegisterFailoverManagerReadyEvent(Common::HHandler hHandler);

                Common::HHandler RegisterFailoverManagerFailedEvent(Common::EventHandler const & handler);
                bool UnRegisterFailoverManagerFailedEvent(Common::HHandler hHandler);

                void ForwardHealthReportFromRAPToHealthManager(
                    Transport::MessageUPtr && messagePtr,
                    Transport::IpcReceiverContextUPtr && ipcTransportContext);

                Common::ErrorCode AddHealthReport(ServiceModel::HealthReport &&);

            protected:
                Common::ErrorCode OnOpen();
                Common::ErrorCode OnClose();
                void OnAbort();

            private:
                int nodeUpToFMCount_;
                int nodeUpToFMMCount_;

                std::vector<UploadLfumInfo> fmLfumUploads_;
                std::vector<UploadLfumInfo> fmmLfumUploads_;

                Common::FabricNodeConfigSPtr nodeConfig_;
                std::wstring const & workingDirectory_;
            };
        }
    }

}
