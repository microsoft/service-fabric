// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

bool FailoverUnitProxy::ConfigurationUtility::IsConfigurationMessageBodyStale(
    int64 localReplicaId,
    std::vector<Reliability::ReplicaDescription> const & replicaDescriptions,
    ConfigurationReplicaStore const & configurationReplicas) const
{
    for (auto replicaDesc = replicaDescriptions.begin(); replicaDesc != replicaDescriptions.end(); ++replicaDesc)
    {
        if (replicaDesc->ReplicaId == localReplicaId)
        {
            // Skip local replica
            continue;
        }

        // Check if there is an existing entry for this replica
        auto it = configurationReplicas.find(replicaDesc->FederationNodeId);

        if (it == configurationReplicas.end())
        {
            continue;
        }

        Reliability::ReplicaDescription const & existingReplicaDescription = it->second;

        // Found existing entry
        if (existingReplicaDescription.InstanceId > replicaDesc->InstanceId)
        {
            // We have a higher instance id replica, ignore stale entry
            return true;
        }

        if (existingReplicaDescription.InstanceId == replicaDesc->InstanceId &&
            existingReplicaDescription.State != replicaDesc->State)
        {
            // If the replica is already in Ready state, and the incoming is IB or SB
            // ignore the stale message
            if (existingReplicaDescription.State == Reliability::ReplicaStates::Ready &&
                (replicaDesc->State == Reliability::ReplicaStates::InBuild || replicaDesc->State == Reliability::ReplicaStates::StandBy))
            {
                return true;;
            }
        }

        if (existingReplicaDescription.InstanceId == replicaDesc->InstanceId &&
            existingReplicaDescription.IsUp != replicaDesc->IsUp)
        {
            // If the replica is marked as down, and the incoming is up for the same instance id
            // ignore the stale message
            if (replicaDesc->IsUp)
            {
                return true;
            }
        }
    }

    return false;
}

bool FailoverUnitProxy::ConfigurationUtility::CheckConfigurationMessageBodyForUpdates(
    int64 localReplicaId,
    vector<Reliability::ReplicaDescription> const & replicaDescriptions,
    bool shouldApply,
    ConfigurationReplicaStore & configurationReplicas) const
{
    bool modified = false;

    for (auto replicaDesc = replicaDescriptions.begin(); replicaDesc != replicaDescriptions.end(); ++replicaDesc)
    {
        if (replicaDesc->ReplicaId == localReplicaId)
        {
            // Skip current primary
            continue;
        }

        // Check if there is an existing entry for this replica
        auto it = configurationReplicas.find(replicaDesc->FederationNodeId);

        if (it == configurationReplicas.end())
        {
            modified = true;

            if (shouldApply)
            {
                configurationReplicas.insert(make_pair(replicaDesc->FederationNodeId, *replicaDesc));
            }
        }
        else
        {
            Reliability::ReplicaDescription & existingReplicaDescription = it->second;

            ASSERT_IF(existingReplicaDescription.InstanceId > replicaDesc->InstanceId, "Should have been verified by the IsConfigBodyStale");

            if (existingReplicaDescription.InstanceId < replicaDesc->InstanceId ||
                existingReplicaDescription.IsUp != replicaDesc->IsUp ||
                existingReplicaDescription.State != replicaDesc->State ||
                existingReplicaDescription.CurrentConfigurationRole != replicaDesc->CurrentConfigurationRole ||
                existingReplicaDescription.PreviousConfigurationRole != replicaDesc->PreviousConfigurationRole)
            {
                // Replace
                modified = true;

                if (shouldApply)
                {
                    it->second = *replicaDesc;
                }
            }
        }
    }

    // Remove any entries that are present in the configuration that are not present in the message
    for (auto iter = configurationReplicas.begin(); iter != configurationReplicas.end(); )
    {
        // Check if there is an existing entry for this replica in the message
        auto it =
            find_if(
                replicaDescriptions.begin(), replicaDescriptions.end(),
                [&iter](Reliability::ReplicaDescription const & entry)
        {
            return iter->first == entry.FederationNodeId;
        });

        if (it == replicaDescriptions.end())
        {
            modified = true;
            if (shouldApply)
            {
                iter = configurationReplicas.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        else
        {
            ++iter;
        }
    }

    return modified;
}

void FailoverUnitProxy::ConfigurationUtility::GetReplicaCountForPCAndCC(
    std::vector<Reliability::ReplicaDescription> const & replicaDescriptions,
    int & ccCount,
    int & ccNonDroppedCount,
    int & pcCount,
    int & pcNonDroppedCount) const
{
    ccCount = 0;
    pcCount = 0;

    ccNonDroppedCount = 0;
    pcNonDroppedCount = 0;

    for (auto replicaDesc = replicaDescriptions.begin(); replicaDesc != replicaDescriptions.end(); ++replicaDesc)
    {
        // Bookkeeping for quroum size calculation
        if (replicaDesc->CurrentConfigurationRole >= ReplicaRole::Secondary)
        {
            ccCount++;

            if (!replicaDesc->IsDropped)
            {
                ccNonDroppedCount++;
            }
        }

        if (replicaDesc->PreviousConfigurationRole >= ReplicaRole::Secondary)
        {
            pcCount++;

            if (!replicaDesc->IsDropped)
            {
                pcNonDroppedCount++;
            }
        }
    }
}

void FailoverUnitProxy::ConfigurationUtility::CreateReplicatorConfigurationList(
    ConfigurationReplicaStore const & configurationReplicas,
    bool isPrimaryChangeBetweenPCAndCC,
    bool isUpdateCatchupConfiguration,
    Common::ScopedHeap & heap,
    __out std::vector<::FABRIC_REPLICA_INFORMATION> & cc,
    __out std::vector<::FABRIC_REPLICA_INFORMATION> & pc,
    __out std::vector<Reliability::ReplicaDescription const *> & tombstoneReplicas) const
{
    for (auto const & iter : configurationReplicas)
    {
        Reliability::ReplicaDescription const & replicaDesc = iter.second;
        
        // Set the must catchup flag for primary replicas in CC if it is update catchup config
        if (TryAddCCReplicaToList(heap, replicaDesc, isPrimaryChangeBetweenPCAndCC, isUpdateCatchupConfiguration, cc))
        {
            tombstoneReplicas.push_back(&replicaDesc);
        }

        // Never set the must catchup flag for replicas in PC
        if (isUpdateCatchupConfiguration && TryAddPCReplicaToList(heap, replicaDesc, isPrimaryChangeBetweenPCAndCC, false, pc))
        {
            if (replicaDesc.CurrentConfigurationRole == ReplicaRole::None)
            {
                tombstoneReplicas.push_back(&replicaDesc);
            }
        }
    }
}

bool FailoverUnitProxy::ConfigurationUtility::TryAddCCReplicaToList(
    Common::ScopedHeap & heap,
    Reliability::ReplicaDescription const & description,
    bool isPrimaryChangeBetweenPCAndCC,
    bool setMustCatchupFlagIfRoleIsPrimary,
    __inout std::vector<::FABRIC_REPLICA_INFORMATION> & v) const
{
    UNREFERENCED_PARAMETER(isPrimaryChangeBetweenPCAndCC);
    return TryAddReplicaToList(heap, description, description.CurrentConfigurationRole, setMustCatchupFlagIfRoleIsPrimary, v);
}

bool FailoverUnitProxy::ConfigurationUtility::TryAddPCReplicaToList(
    Common::ScopedHeap & heap,
    Reliability::ReplicaDescription const & description,
    bool isPrimaryChangeBetweenPCAndCC,
    bool setMustCatchupFlagIfRoleIsPrimary,
    __inout std::vector<::FABRIC_REPLICA_INFORMATION> & v) const
{
    /*
        Consider the following case:

        1. A S/N reconfiguration is in progress (or S/I)
        2. Primary fails over
        3. During GetLSN phase S/N replica fails to respond and the Get LSN wait timer expires.
        4. Reconfiguration proceeds to phase2 and since this replica is not in CC it is not restarted
        5. When RA sends UC to RAP in this situation the LSN of S/N replica is -1 as it did not respond to GetLSN
           Do not pass this replica to the replicator (but it will be considered in calculation for write quorum)
           This should only be done during failovers
    */
    if (isPrimaryChangeBetweenPCAndCC && 
        description.IsInPreviousConfiguration && 
        !description.IsInCurrentConfiguration && 
        !description.IsLastAcknowledgedLSNValid)
    {
        return false;
    }

    return TryAddReplicaToList(heap, description, description.PreviousConfigurationRole, setMustCatchupFlagIfRoleIsPrimary, v);
}

bool FailoverUnitProxy::ConfigurationUtility::TryAddReplicaToList(
    Common::ScopedHeap & heap,
    Reliability::ReplicaDescription const & replicaDesc,
    ReplicaRole::Enum role,
    bool setMustCatchupFlagIfRoleIsPrimary,
    __inout std::vector<::FABRIC_REPLICA_INFORMATION> & v) const
{
    /*
        Only give Up, READY replicas in the configuration
        During swap, the failover view contains the new primary's CC role as 'P'
        However, the old primary is still the primary and is performing a catchup
        At this point the new primary is to be presented as a secondary to the replicator

        Similarly if this was a promote to primary and the old primary is still in the failover and REPL view 
        (becaue of MinReplicaSetSize) give the old primary in the PC as a secondary

    */
    if (replicaDesc.IsUp &&
        replicaDesc.State == Reliability::ReplicaStates::Ready &&
        role >= ReplicaRole::Secondary)
    {
        v.push_back(CreateReplicaInformation(
            heap, 
            replicaDesc,
            ReplicaRole::Secondary,
            true,
            setMustCatchupFlagIfRoleIsPrimary && role == ReplicaRole::Primary));

        return true;
    }

    return false;
}

FABRIC_REPLICA_INFORMATION FailoverUnitProxy::ConfigurationUtility::CreateReplicaInformation(
    Common::ScopedHeap & heap,
    Reliability::ReplicaDescription const & description,
    ReplicaRole::Enum role,
    bool setProgress,
    bool mustCatchup) const
{
    TESTASSERT_IF(!description.IsUp, "Passing a down replica to replicator");

    ::FABRIC_REPLICA_INFORMATION replicaInfo = {0};

    replicaInfo.Id = description.ReplicaId;
    replicaInfo.Role = ReplicaRole::ConvertToPublicReplicaRole(role);
    replicaInfo.ReplicatorAddress = description.ReplicationEndpoint.c_str();
    replicaInfo.CurrentProgress = setProgress ? description.LastAcknowledgedLSN : FABRIC_INVALID_SEQUENCE_NUMBER;
    replicaInfo.CatchUpCapability = setProgress ? description.FirstAcknowledgedLSN : FABRIC_INVALID_SEQUENCE_NUMBER;
    replicaInfo.Status = FABRIC_REPLICA_STATUS_UP;

    auto ex1 = heap.AddItem<FABRIC_REPLICA_INFORMATION_EX1>();
    ex1->MustCatchup = mustCatchup ? TRUE : FALSE;
    ex1->Reserved = NULL;
    replicaInfo.Reserved = ex1.GetRawPointer();

    return replicaInfo;
}
