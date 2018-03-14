// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Transport;

ReliabilitySubsystemWrapper::ReliabilitySubsystemWrapper(ReliabilitySubsystem * reliability)
: reliability_(reliability)
{
}

std::wstring const & ReliabilitySubsystemWrapper::get_WorkingDirectory() const
{
    return reliability_->WorkingDirectory;        
}

Common::FabricNodeConfigSPtr const & ReliabilitySubsystemWrapper::get_NodeConfig()
{
    return reliability_->NodeConfig;
}

void ReliabilitySubsystemWrapper::SendNodeUpToFM()
{
    reliability_->SendNodeUpToFM();
}

void ReliabilitySubsystemWrapper::SendNodeUpToFMM()
{
    reliability_->SendNodeUpToFMM();
}

void ReliabilitySubsystemWrapper::UploadLFUM(
    Reliability::GenerationNumber const & generation,
    std::vector<Reliability::FailoverUnitInfo> && failoverUnitInfo,
    bool anyReplicaFound)
{
    reliability_->UploadLFUM(generation, move(failoverUnitInfo), anyReplicaFound);
}

void ReliabilitySubsystemWrapper::UploadLFUMForFMReplicaSet(
    Reliability::GenerationNumber const & generation,
    std::vector<Reliability::FailoverUnitInfo> && failoverUnitInfo,
    bool anyReplicaFound)
{
    reliability_->UploadLFUMForFMReplicaSet(generation, move(failoverUnitInfo), anyReplicaFound);
}

ErrorCode ReliabilitySubsystemWrapper::AddHealthReport(ServiceModel::HealthReport && healthReport)
{
    return reliability_->HealthClient->AddHealthReport(move(healthReport));
}

void ReliabilitySubsystemWrapper::ForwardHealthReportFromRAPToHealthManager(
    MessageUPtr && messagePtr,
    IpcReceiverContextUPtr && ipcTransportContext)
{
    MessageUPtr msgPtr = move(messagePtr);

    // Ensure HM header is set
    msgPtr->Headers.Replace(ActorHeader(Actor::HM));

    MoveUPtr<IpcReceiverContext> contextMover(move(ipcTransportContext));

    AsyncOperationSPtr sendToHealthManager = 
        reliability_->HealthClient->HealthReportingTransportObj.BeginReport(
            move(msgPtr),
            Client::ClientConfig::GetConfig().HealthOperationTimeout,
            [this, contextMover](AsyncOperationSPtr const & operation) mutable
            { 
                IpcReceiverContextUPtr context = contextMover.TakeUPtr();

                this->SendToHealthManagerCompletedCallback(operation, move(context)); 
            },
            reliability_->Root.CreateAsyncOperationRoot());
}

void ReliabilitySubsystemWrapper::SendToHealthManagerCompletedCallback(AsyncOperationSPtr const & asyncOperation, IpcReceiverContextUPtr && context)
{
    Transport::MessageUPtr reply;

    ErrorCode errorCode = reliability_->HealthClient->HealthReportingTransportObj.EndReport(asyncOperation, reply);
	errorCode.ReadValue();

	if (reply != nullptr)
	{
		// No error check needed as errors will be sent back to RAP as part of reply.
		context->Reply(move(reply));
	}
}

