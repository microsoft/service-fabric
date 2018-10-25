// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Reliability/Failover/common/GenerationProposalReplyMessageBody.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class IReconfigurationAgent
        {
        public:

            virtual Common::ErrorCode Open(NodeUpOperationFactory & nodeUpFactory) = 0;

            virtual void Close() = 0;

            virtual Store::ILocalStore & Test_GetLocalStore() = 0;

            virtual void ProcessTransportRequest(
                Transport::MessageUPtr & message, 
                Federation::OneWayReceiverContextUPtr && context) = 0;

            virtual void ProcessTransportRequestResponseRequest(
                Transport::MessageUPtr & message, 
                Federation::RequestReceiverContextUPtr && context) = 0;

            virtual GenerationProposalReplyMessageBody ProcessGenerationProposal(
                Transport::Message & request, 
                bool & isGfumUploadOut) = 0;

            virtual void ProcessServiceTypeDisabled(
                ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                uint64 sequenceNumber,
                ServiceModel::ServicePackageActivationContext const& activationContext) = 0;

            virtual void ProcessServiceTypeEnabled(
                ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                uint64 sequenceNumber,
                ServiceModel::ServicePackageActivationContext const& activationContext) = 0;

            virtual void ProcessServiceTypeRegistered(
                Hosting2::ServiceTypeRegistrationSPtr const & registration) = 0;

            virtual void ProcessRuntimeClosed(
                std::wstring const & hostId, 
                std::wstring const & runtimeId) = 0;

            virtual void ProcessAppHostClosed(
                std::wstring const & hostId,
                Common::ActivityDescription const & activityDescription) = 0;

            virtual void ProcessNodeUpAck(
                Transport::MessageUPtr && nodeUpReply, 
                bool isFromFMM) = 0;

            virtual void ProcessLFUMUploadReply(
                Reliability::GenerationHeader const & generationHeader) = 0;

            virtual ~IReconfigurationAgent()
            {
            }
        };

        struct ReconfigurationAgentConstructorParameters
        {
            Common::ComponentRoot const * Root;
            IReliabilitySubsystemWrapperSPtr ReliabilityWrapper;
            IFederationWrapper * FederationWrapper;
            Transport::IpcServer * IpcServer;
            Hosting2::IHostingSubsystem * Hosting;
            Store::IStoreFactorySPtr StoreFactory;
            Infrastructure::IClockSPtr Clock;
            Common::ProcessTerminationServiceSPtr ProcessTerminationService;
            KtlLogger::KtlLoggerNodeSPtr KtlLoggerNode;
        };

        typedef IReconfigurationAgentUPtr ReconfigurationAgentFactoryFunctionPtr(ReconfigurationAgentConstructorParameters &);

        ReconfigurationAgentFactoryFunctionPtr ReconfigurationAgentFactory;
    }
}

