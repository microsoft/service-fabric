// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest; 
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;

namespace
{
    FailoverUnit* GetData(LocalFailoverUnitMapEntrySPtr const & entry)
    {
        return entry->State.Test_Data.Data.get();
    }
}

FailoverUnitContainer::FailoverUnitContainer(FailoverUnitSPtr && ft)
{
    ftSPtr_ = move(ft);
}

FailoverUnitContainer::~FailoverUnitContainer()
{
}

void FailoverUnitContainer::InsertIntoLFUM(UnitTestContext & utContext)
{
    auto id = ftSPtr_->FailoverUnitId;
    entry_ = make_shared<LocalFailoverUnitMapEntry>(id, utContext.RA.LfumStore, move(ftSPtr_));

    // Reconcile all the sets over here
    // The FT may have different membership in sets
    // These need to be added to the set themselves
    ReconcileAllSets(utContext.RA);

    auto ft = GetData(entry_);

    auto error = Infrastructure::ReliabilityUnitTest::InfrastructureTestUtility::WriteRecordIntoStore<FailoverUnit>(
        *utContext.RA.LfumStore,
        Storage::Api::OperationType::Insert,
        entry_->PersistenceId,
        ft);

    ASSERT_IF(!error.IsSuccess(), "Must succeed");

    if (ft->ServiceTypeRegistration != nullptr)
    {
        utContext.RA.HostingAdapterObj.Test_GetServiceTypeMapObj().Test_Add(ft->ServiceTypeRegistration);
    }

    ft->AssertInvariants();
    utContext.RA.LocalFailoverUnitMapObj.Test_AddFailoverUnit(entry_->Id, entry_);    
}

void FailoverUnitContainer::ReconcileAllSets(ReconfigurationAgent & ra)
{
    auto const & stateSnapshot = entry_->State.Test_Data;
    if (stateSnapshot.IsDeleted || stateSnapshot.Data == nullptr)
    {
        return;
    }

    auto failoverUnit = GetData(entry_);

    StateMachineActionQueue queue;
    ReconcileSet(failoverUnit->MessageRetryActiveFlag, queue);
    ReconcileSet(failoverUnit->CleanupPendingFlag, queue);
    ReconcileSet(failoverUnit->LocalReplicaClosePending, queue);
    ReconcileSet(failoverUnit->LocalReplicaOpenPending, queue);
    ReconcileSet(failoverUnit->Test_GetLocalReplicaServiceDescriptionUpdatePending(), queue);
    
    // fix up fm sets
    auto v = failoverUnit->Test_FMMessageStateObj().Test_GetAllSets();
    for (auto it : v)
    {
        ReconcileSet(*it, queue);
    }

    queue.ExecuteAllActions(DefaultActivityId, entry_, ra);
}

void FailoverUnitContainer::ReconcileSet(
    SetMembershipFlag const & flag,
    StateMachineActionQueue & queue)
{
    if (flag.IsSet)
    {
        queue.Enqueue(make_unique<Infrastructure::EntitySet::ChangeSetMembershipAction>(flag.Id, flag.IsSet));
    }
}

bool FailoverUnitContainer::Create(
    std::wstring const & ftString, 
    ReadContext const & readContext, 
    FailoverUnitContainerUPtr & rv)
{
    FailoverUnitSPtr ft;
    
    ErrorLogger errLogger(L"FailoverUnitContainer", ftString);
    Reader r(ftString, readContext);

    if (!r.Read<FailoverUnitSPtr>(L'\0', ft))
    {
        errLogger.Log(L"FailoverUnit");
        return false;
    }

    FailoverUnitContainer* ptr = new FailoverUnitContainer(move(ft));
    rv = FailoverUnitContainerUPtr(ptr);
    return true;
}

FailoverUnitContainerUPtr FailoverUnitContainer::CreateClosedFT(std::wstring const & shortName, UnitTestContext & utContext)
{
    SingleFTReadContext newContext = Default::GetInstance().LookupFTContext(shortName);
    newContext.RA = &utContext.RA;

    return CreateClosedFT(newContext);
}

FailoverUnitContainerUPtr FailoverUnitContainer::CreateClosedFT(SingleFTReadContext const & readContext)
{
    auto ft = CreateClosedFTPtr(readContext);

    FailoverUnitContainer* ptr = new FailoverUnitContainer(move(ft));
    return FailoverUnitContainerUPtr(ptr);
}

FailoverUnitSPtr FailoverUnitContainer::CreateClosedFTPtr(std::wstring const & shortName)
{
    return CreateClosedFTPtr(Default::GetInstance().LookupFTContext(shortName));
}

FailoverUnitSPtr FailoverUnitContainer::CreateClosedFTPtr(StateManagement::SingleFTReadContext const & readContext)
{
    FailoverUnitSPtr ft;

    Reader r(L"C None 000/000/411 1:1 -", readContext);
    bool read = r.Read<FailoverUnitSPtr>(L'\0', ft);
    ASSERT_IF(!read, "the default closed FT should always be parseable");

    return ft;
}

void FailoverUnitContainer::Compare(FailoverUnit & expected, FailoverUnit & actual)
{
    TestLog::WriteInfo(wformatString("Validating FT"));

    TestLog::WriteInfo(wformatString("Expected"));
	  TestLog::WriteInfo(wformatString("{0}", expected));

    TestLog::WriteInfo(wformatString("Actual"));
	  TestLog::WriteInfo(wformatString("{0}", actual));

    Verify::AreEqualLogOnError(wformatString(expected.State), wformatString(actual.State), L"FT State");
    Verify::AreEqualLogOnError(wformatString(expected.LocalReplicaRole), wformatString(actual.LocalReplicaRole), L"LocalReplicaRole");
    Verify::AreEqualLogOnError(wformatString(expected.ReconfigurationStage), wformatString(actual.ReconfigurationStage), L"ReconfigStage");
        
    Verify::AreEqualLogOnError(wformatString("{0}/{1}/{2}", expected.PreviousConfigurationEpoch, expected.IntermediateConfigurationEpoch, expected.CurrentConfigurationEpoch), 
                        wformatString("{0}/{1}/{2}", actual.PreviousConfigurationEpoch, actual.IntermediateConfigurationEpoch, actual.CurrentConfigurationEpoch), 
                        L"Epoch");

    Verify::AreEqualLogOnError(expected.DeactivationInfo, actual.DeactivationInfo, L"DeactivationInfo");

    Verify::AreEqualLogOnError(wformatString("{0}:{1}", expected.LocalReplicaId, expected.LocalReplicaInstanceId),
                        wformatString("{0}:{1}", actual.LocalReplicaId, actual.LocalReplicaInstanceId), 
                        L"Replica Id");

    Verify::AreEqualLogOnError(wformatString("{0} {1}", expected.TargetReplicaSetSize, expected.MinReplicaSetSize),
                        wformatString("{0} {1}", expected.TargetReplicaSetSize, expected.MinReplicaSetSize),
                        L"Replica Set Size");

    Verify::AreEqualLogOnError(expected.ServiceDescription.UpdateVersion, actual.ServiceDescription.UpdateVersion, L"Update version");
    Verify::AreEqualLogOnError(expected.IsRuntimeActive, actual.IsRuntimeActive, L"IsRuntimeActive");
    Verify::AreEqualLogOnError(expected.IsUpdateReplicatorConfiguration, actual.IsUpdateReplicatorConfiguration, L"IsUpdateReplicatorConfiguration");
    Verify::AreEqualLogOnError(expected.LocalReplicaOpen, actual.LocalReplicaOpen, L"LocalReplicaOpen");   
    Verify::AreEqualLogOnError(expected.LocalReplicaClosePending.IsSet, actual.LocalReplicaClosePending.IsSet, L"LocalReplicaClosePending");
    Verify::AreEqualLogOnError(expected.LocalReplicaOpenPending.IsSet, actual.LocalReplicaOpenPending.IsSet, L"LocalReplicaOpenPending");    
    Verify::AreEqualLogOnError(expected.LocalReplicaDeleted, actual.LocalReplicaDeleted, L"LocalReplicaDeleted");
    Verify::AreEqualLogOnError(expected.CleanupPendingFlag.IsSet, actual.CleanupPendingFlag.IsSet, L"CleanupPending");
    Verify::AreEqualLogOnError(expected.MessageRetryActiveFlag.IsSet, actual.MessageRetryActiveFlag.IsSet, L"MessageRetryFlag");

    if (expected.SenderNode != ReconfigurationAgent::InvalidNode)
    {
        Verify::AreEqualLogOnError(expected.SenderNode, actual.SenderNode, L"SenderNode");
    }

    Verify::AreEqualLogOnError(expected.CloseMode, actual.CloseMode, L"Replica Close Mode");

    Verify::AreEqualLogOnError(expected.IsEndpointPublishPending, actual.IsEndpointPublishPending, L"UpdatedEndpointAvailable");
    CompareFMMessageState(expected.Test_FMMessageStateObj(), actual.Test_FMMessageStateObj());
    
    Verify::AreEqualLogOnError(expected.Replicas.size(), actual.Replicas.size(), L"Replica Size");
    if (expected.Replicas.size() != actual.Replicas.size())
    {
        return;
    }

    for(size_t i = 0; i < expected.Replicas.size(); i++)
    {
        TestLog::WriteInfo(wformatString("{0}", Default::Delimiter));
        TestLog::WriteInfo(wformatString("Validating Replica {0} of {1}", i, expected.Replicas.size()));
        CompareReplicas(expected.Replicas[i], actual.Replicas[i], i);
    }
}

void FailoverUnitContainer::CompareFMMessageState(FMMessageState const & expected, FMMessageState const & actual)
{
    Verify::AreEqualLogOnError(expected.FMMessageRetryPending.IsSet, actual.FMMessageRetryPending.IsSet, L"FMMessageRetryPending");
    Verify::AreEqualLogOnError(expected.Test_GetDownReplicaInstanceId(), actual.Test_GetDownReplicaInstanceId(), wformatString("DownREplicaInstanceId. Expected {0}. Actual {1}", expected.Test_GetDownReplicaInstanceId(), actual.Test_GetDownReplicaInstanceId()));
    Verify::AreEqualLogOnError(expected.MessageStage, actual.MessageStage, L"FM Message Stage");
}

namespace
{
    wstring GetLsnString(Replica const & replica)
    {
        return wformatString(
            "{0} {1} {2}",
            replica.GetFirstAcknowledgedLSN(),
            replica.GetLastAcknowledgedLSN(),
            L" "/*replica.LSNEpoch*/);
    }
}

void FailoverUnitContainer::CompareReplicas(Replica const & expected, Replica const & actual, size_t replicaIndex)
{
    Verify::AreEqualLogOnError(wformatString("{0}/{1}/{2}", expected.PreviousConfigurationRole, expected.IntermediateConfigurationRole, expected.CurrentConfigurationRole),
                        wformatString("{0}/{1}/{2}", actual.PreviousConfigurationRole, actual.IntermediateConfigurationRole, actual.CurrentConfigurationRole),
                        wformatString("Replica {0}: Replica Role", replicaIndex).c_str());

    Verify::AreEqualLogOnError(wformatString(expected.State), wformatString(actual.State), wformatString("Replica {0}: Replica State", replicaIndex).c_str());
    Verify::AreEqualLogOnError(expected.IsUp ? L"U" : L"D", actual.IsUp ? L"U" : L"D", wformatString("Replica {0}: IsUp", replicaIndex).c_str());
    Verify::AreEqualLogOnError(wformatString(expected.MessageStage), wformatString(actual.MessageStage), wformatString("Replica {0}: MessageStage", replicaIndex).c_str());
    Verify::AreEqualLogOnError(expected.ToBeDeactivated, actual.ToBeDeactivated, wformatString("Replica {0}: ToBeDeactivated", replicaIndex).c_str());
    Verify::AreEqualLogOnError(expected.ToBeActivated, actual.ToBeActivated, wformatString("Replica {0}: ToBeActivated", replicaIndex).c_str());
    Verify::AreEqualLogOnError(expected.ReplicatorRemovePending, actual.ReplicatorRemovePending, wformatString("Replica {0}: ReplicatorRemovePending", replicaIndex).c_str());
        
    Verify::AreEqualLogOnError(wformatString("{0} {1}:{2}", expected.FederationNodeInstance, expected.ReplicaId, expected.InstanceId),
                        wformatString("{0} {1}:{2}", actual.FederationNodeInstance, actual.ReplicaId, actual.InstanceId),
                        wformatString("Replica {0}: NodeInstance ReplicaId:InstanceId", replicaIndex).c_str());

    Verify::AreEqualLogOnError(wformatString(expected.PackageVersionInstance), wformatString(actual.PackageVersionInstance), wformatString("Replica {0}: PackageVersionInstance", replicaIndex).c_str());

    Verify::AreEqualLogOnError(
        GetLsnString(expected),
        GetLsnString(actual),
        wformatString("Replica {0}: LSN", replicaIndex).c_str());

    Verify::AreEqualLogOnError(expected.ServiceLocation, actual.ServiceLocation, wformatString("Replica {0}: ServiceLocation", replicaIndex).c_str());
    Verify::AreEqualLogOnError(expected.ReplicationEndpoint, actual.ReplicationEndpoint, wformatString("Replica {0}: ReplicationEndpoint", replicaIndex).c_str());    
}
