// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReliabilityUnitTest;

namespace
{
    void UploadLfumHandler(
        std::vector<ReliabilitySubsystemStub::UploadLfumInfo> & vect,
        Reliability::GenerationNumber const & genNum,
        std::vector<Reliability::FailoverUnitInfo> && fts,
        bool anyReplicaFound)
    {
        ReliabilitySubsystemStub::UploadLfumInfo var;
        var.AnyReplicaFound = anyReplicaFound;
        var.FailoverUnits = move(fts);
        var.GenerationNumber = genNum;

        vect.push_back(var);
    }
}

ReliabilitySubsystemStub::ReliabilitySubsystemStub(FabricNodeConfigSPtr const & nodeConfig, wstring const & workingDirectory)
: nodeConfig_(nodeConfig),
  workingDirectory_(workingDirectory)
{
    Reset();
}

Reliability::FederationWrapper & ReliabilitySubsystemStub::get_FederationWrapper() 
{
    Assert::CodingError("Not Impl");
}

ServiceAdminClient & ReliabilitySubsystemStub::get_ServiceAdminClient()
{
    Assert::CodingError("Not Impl");
}

ServiceResolver & ReliabilitySubsystemStub::get_ServiceResolver()
{
    Assert::CodingError("Not Impl");
}

Federation::FederationSubsystem & ReliabilitySubsystemStub::get_FederationSubsystem() const
{
    Assert::CodingError("Not Impl");
}

ErrorCode ReliabilitySubsystemStub::OnOpen()
{
    return ErrorCodeValue::Success;
}

ErrorCode ReliabilitySubsystemStub::OnClose()
{
    return ErrorCodeValue::Success;
}

void ReliabilitySubsystemStub::OnAbort()
{
}

void ReliabilitySubsystemStub::InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & )
{
    Assert::CodingError("Not Impl");
}

ErrorCode ReliabilitySubsystemStub::SetSecurity(Transport::SecuritySettings const& )
{
    Assert::CodingError("Not Impl");
}

HHandler ReliabilitySubsystemStub::RegisterFailoverManagerReadyEvent(Common::EventHandler const & )
{
    Assert::CodingError("Not Impl");
}

bool ReliabilitySubsystemStub::UnRegisterFailoverManagerReadyEvent(Common::HHandler )
{
    Assert::CodingError("Not Impl");
}

HHandler ReliabilitySubsystemStub::RegisterFailoverManagerFailedEvent(Common::EventHandler const & )
{
    Assert::CodingError("Not Impl");
}

bool ReliabilitySubsystemStub::UnRegisterFailoverManagerFailedEvent(Common::HHandler )
{
    Assert::CodingError("Not Impl");
}

void ReliabilitySubsystemStub::SendNodeUpToFM()
{
    ++nodeUpToFMCount_;
}

void ReliabilitySubsystemStub::SendNodeUpToFMM()
{
    ++nodeUpToFMMCount_;
}

void ReliabilitySubsystemStub::UploadLFUM(
    Reliability::GenerationNumber const & genNum,
    std::vector<Reliability::FailoverUnitInfo> && fts,
    bool anyReplicaFound)
{
    UploadLfumHandler(fmLfumUploads_, genNum, move(fts), anyReplicaFound);
}

void ReliabilitySubsystemStub::UploadLFUMForFMReplicaSet(
    Reliability::GenerationNumber const & genNum,
    std::vector<Reliability::FailoverUnitInfo> && fts,
    bool anyReplicaFound)
{
    UploadLfumHandler(fmmLfumUploads_, genNum, move(fts), anyReplicaFound);
}

void ReliabilitySubsystemStub::Reset()
{
    nodeUpToFMCount_ = 0;
    nodeUpToFMMCount_ = 0;
    fmLfumUploads_.clear();
    fmmLfumUploads_.clear();
}

void ReliabilitySubsystemStub::ForwardHealthReportFromRAPToHealthManager(
    Transport::MessageUPtr && ,
    Transport::IpcReceiverContextUPtr && )
{
    Assert::CodingError("This should not be called in RA unit test");
}

ErrorCode ReliabilitySubsystemStub::AddHealthReport(ServiceModel::HealthReport &&)
{
    Assert::CodingError("This should not be called in RA unit test");
}
