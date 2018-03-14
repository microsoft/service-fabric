// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define AtomicGroupStateProviderBeginCommit                         L"atomic.begincommit"
#define AtomicGroupStateProviderEndCommit                           L"atomic.endcommit"
#define AtomicGroupStateProviderBeginRollback                       L"atomic.beginrollback"
#define AtomicGroupStateProviderEndRollback                         L"atomic.endrollback"
#define AtomicGroupStateProviderBeginUndoProgress                   L"atomic.beginundoprogress"
#define AtomicGroupStateProviderEndUndoProgress                     L"atomic.endundoprogress"
#define ProviderStateChangedOnDataLoss                              L"provider.statechangedondataloss"
#define ProviderStateChangedOnDataLossDecrementProgress             L"provider.statechangedondataloss.decrementprogress"
#define ProviderBeginGetNextCopyStateBlock                          L"provider.begingetnextcopystate.block"
#define ServiceGroupDelayCommit                                     L"servicegroup.delaycommit"
#define ServiceGroupAfterGetCopyStreamReportFaultPermanent          L"servicegroup.after.getcopystream.reportfault.permanent"
#define ServiceGroupAfterGetReplicationStreamReportFaultPermanent   L"servicegroup.after.getreplicationstream.reportfault.permanent"
#define ServiceGroupAfterBeginReplicateAtomicGroupCommitReportFaultPermanent L"servicegroup.after.begincommit.reportfault.permanent"
#define ServiceGroupSkipValidation                                  L"servicegroup.skipvalidation"
#define ServiceBeginCloseEnableSecondaryPump                        L"service.beginclose.enablesecondarypump"
#define ServiceAbortEnableSecondaryPump                             L"service.abort.enablesecondarypump"
#define ServiceBeginChangeRoleEnableSecondaryPump                   L"service.beginchangerole.enablesecondarypump"
#define ServiceBeginOpenExpectNewOpenMode                           L"service.beginopen.expectnewopenmode"
#define ServiceBeginOpenExpectExistingOpenMode                      L"service.beginopen.expectexistingopenmode"
#define ServiceBeginOpenBlock                                       L"service.beginopen.block"
#define ServiceEndOpenBlock                                         L"service.endopen.block"
#define ServiceBeginCloseBlock                                      L"service.beginclose.block"
#define ServiceEndCloseBlock                                        L"service.endclose.block"
#define ServiceBeginChangeRoleBlock                                 L"service.beginchangerole.block"
#define ServiceEndChangeRoleBlock                                   L"service.endchangerole.block"
#define ServiceAbortBlock                                           L"service.abort.block"
#define StartPumpReplicationBlock                                   L"startpump.replication.block"
#define StartPumpCopyBlock                                          L"startpump.copy.block"
#define PumpReplicationBlock                                        L"pump.replication.block"
#define PumpCopyBlock                                               L"pump.copy.block"
#define ReplicatorBeginOpenBlock                                    L"replicator.beginopen.block"
#define ReplicatorEndOpenBlock                                      L"replicator.endopen.block"
#define ReplicatorBeginCloseBlock                                   L"replicator.beginclose.block"
#define ReplicatorEndCloseBlock                                     L"replicator.endclose.block"
#define ReplicatorBeginChangeRoleBlock                              L"replicator.beginchangerole.block"
#define ReplicatorEndChangeRoleBlock                                L"replicator.endchangerole.block"
#define ReplicatorBeginUpdateEpochBlock                             L"replicator.beginupdateepoch.block"
#define ReplicatorEndUpdateEpochBlock                               L"replicator.endupdateepoch.block"
#define ReplicatorAbortBlock                                        L"replicator.abort.block"
#define ReplicatorUpdateCatchUpReplicaSetConfigurationBlock         L"replicator.updatecatchupreplicasetconfiguration.block"
#define ReplicatorUpdateCurrentReplicaSetConfigurationBlock         L"replicator.updatecurrentreplicasetconfiguration.block"
#define ReplicatorBeginWaitForCatchupQuorumBlock                    L"replicator.beginwaitforcatchupquorum.block"
#define ReplicatorBeginOnDataLossBlock                              L"replicator.beginondataloss.block"
#define ReplicatorGetQueryBlock                                     L"replicator.getquery.block"
#define ReplicatorReturnZeroProgress                                L"replicator.returnzeroprogress"

#define StateProviderSecondaryPumpBlock                             L"provider.secondarypump.block"
#define StateProviderBeginUpdateEpochBlock                          L"provider.beginupdateepoch.block"
#define StateProviderEndUpdateEpochBlock                            L"provider.endupdateepoch.block"
#define StateProviderPrepareCheckpointBlock                         L"provider2.preparecheckpoint.block"
#define StateProviderPerformCheckpointBlock                         L"provider2.performcheckpoint.block"
#define StateProviderCompleteCheckpointBlock                        L"provider2.completecheckpoint.block"
#define StateProviderRecoverCheckpointAsyncBlock                    L"provider2.recovercheckpointasync.block"
#define StateProviderBackupCheckpointAsyncBlock                     L"provider2.backupcheckpointasync.block"
#define StateProviderRestoreCheckpointAsyncBlock                    L"provider2.restorecheckpointasync.block"
#define StateProviderOpenAsyncBlock                                 L"provider2.openasync.block"
#define StateProviderInitializeBlock                                L"provider2.initialize.block"
#define StateProviderGetChildrenBlock                               L"provider2.getchildren.block"
#define StateProviderGetNameBlock                                   L"provider2.getname.block"
#define StateProviderCloseAsyncBlock                                L"provider2.closeasync.block"
#define StateProviderChangeRoleAsyncBlock                           L"provider2.changeroleasync.block"
#define StateProviderPrepareForRemoveAsyncBlock                     L"provider2.prepareforremoveasync.block"
#define StateProviderRemoveStateAsyncBlock                          L"provider2.removestateasync.block"
#define StateProviderGetCurrentStateBlock                           L"provider2.getcurrentstate.block"
#define StateProviderBeginSettingCurrentStateBlock                  L"provider2.beginsettingcurrentstate.block"
#define StateProviderSetCurrentCurrentStateAsyncBlock               L"provider2.setcurrentstateasync.block"
#define StateProviderEndSettingCurrentStateAsyncBlock               L"provider2.endsettingcurrentstate.block"

namespace TestHooks
{
    class ApiSignalHelper
    {
        DENY_COPY(ApiSignalHelper);

    public:
        static void AddSignal(
            std::wstring const& nodeId,
            std::wstring const& serviceName,
            std::wstring const& strSignal);

        static void ResetSignal(
            std::wstring const& nodeId,
            std::wstring const& serviceName,
            std::wstring const& strSignal);

        static void ResetAllSignals();

        static bool IsSignalSet(
            std::wstring const& nodeId,
            std::wstring const& serviceName,
            std::wstring const& strSignal);

        static bool IsSignalSet(
            Federation::NodeId const& nodeId,
            std::wstring const& serviceName,
            std::wstring const& strSignal);

        static void ApiSignalHelper::WaitForSignalReset(
            std::wstring const& nodeId,
            std::wstring const& serviceName,
            std::wstring const& strSignal);

        static void ApiSignalHelper::WaitForSignalReset(
            Federation::NodeId const& nodeId,
            std::wstring const& serviceName,
            std::wstring const& strSignal);

        static void ApiSignalHelper::WaitForSignalIfSet(
            TestHookContext const&,
            std::wstring const& strSignal);

        static void ApiSignalHelper::WaitForSignalHit(
            std::wstring const& nodeId,
            std::wstring const& serviceName,
            std::wstring const& strSignal);

        static void ApiSignalHelper::FailIfSignalHit(
            std::wstring const& nodeId,
            std::wstring const& serviceName,
            std::wstring const& strSignal,
            Common::TimeSpan timeout);

    private:
        static Common::Global<NodeServiceEntityStore<Signal>> signalStore_;
        static Common::StringLiteral const TraceSource;
    };
};
