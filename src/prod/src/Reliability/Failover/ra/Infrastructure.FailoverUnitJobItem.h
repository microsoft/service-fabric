// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> StateCleanupJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> ReplicaUpMessageRetryJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> ReconfigurationMessageRetryJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> ReplicaCloseMessageRetryJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> ReplicaOpenMessageRetryJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> NodeActivateDeactivateJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, NodeUpdateServiceJobItemContext> NodeUpdateServiceJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, ReplicaUpReplyJobItemContext> ReplicaUpReplyJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> UpdateServiceDescriptionMessageRetryJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, QueryJobItemContext> QueryJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> AppHostClosedJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> RuntimeClosedJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> ServiceTypeRegisteredJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, ApplicationUpgradeEnumerateFTsJobItemContext> ApplicationUpgradeEnumerateFTsJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, ApplicationUpgradeUpdateVersionJobItemContext> ApplicationUpgradeUpdateVersionJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, UpgradeCloseCompletionCheckContext> UpgradeCloseCompletionCheckJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, ApplicationUpgradeReplicaDownCompletionCheckJobItemContext> ApplicationUpgradeReplicaDownCompletionCheckJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> ApplicationUpgradeFinalizeFTJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> FabricUpgradeSendCloseMessageJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> FabricUpgradeRollbackJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> UpdateEntitiesOnCodeUpgradeJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, GetLfumJobItemContext> GetLfumJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, GenerationUpdateJobItemContext> GenerationUpdateJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> NodeUpAckJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, HostQueryJobItemContext> HostQueryJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, ClientReportFaultJobItemContext> ClientReportFaultJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> UpdateStateOnLFUMLoadJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, CheckReplicaCloseProgressJobItemContext> CheckReplicaCloseProgressJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> NodeCloseJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> CloseFailoverUnitJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> UploadForReplicaDiscoveryJobItem;
        typedef Infrastructure::EntityJobItem<FailoverUnit, JobItemContextBase> UpdateEntityAfterFMMessageSendJobItem;
    }
}
