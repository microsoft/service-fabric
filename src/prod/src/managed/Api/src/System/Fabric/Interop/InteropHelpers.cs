// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System.Fabric;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using FABRIC_STRING_PAIR = NativeTypes.FABRIC_APPLICATION_PARAMETER;

    /// <summary>
    /// All WF interop helpers used within System.Fabric
    /// </summary>
    internal static class InteropHelpers
    {
        private const string TraceComponent = "InteropHelpers";

        internal class ServiceGroupDescriptionHelper
        {
            internal static IntPtr ToNative(PinCollection pin, ServiceGroupDescription description)
            {
                if (description == null)
                {
                    return IntPtr.Zero;
                }

                uint length = (uint)description.MemberDescriptions.Count;
                var nativeDescription = new NativeTypes.FABRIC_SERVICE_GROUP_DESCRIPTION[1];

                nativeDescription[0].Description = description.ServiceDescription.ToNative(pin);
                nativeDescription[0].MemberCount = length;

                var members = new NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION[length];
                for (int i = 0; i < length; i++)
                {
                    members[i].ServiceTypeName = pin.AddBlittable(description.MemberDescriptions[i].ServiceTypeName);
                    members[i].ServiceName = pin.AddObject(description.MemberDescriptions[i].ServiceName);

                    var initializationData = NativeTypes.ToNativeBytes(pin, description.MemberDescriptions[i].InitializationData);
                    members[i].InitializationDataSize = initializationData.Item1;
                    members[i].InitializationData = initializationData.Item2;

                    var metrics = NativeTypes.ToNativeLoadMetrics(pin, description.MemberDescriptions[i].Metrics);
                    members[i].MetricCount = metrics.Item1;
                    members[i].Metrics = metrics.Item2;

                    pin.AddBlittable(members[i]);
                }

                nativeDescription[0].MemberDescriptions = pin.AddBlittable(members);
                return pin.AddBlittable(nativeDescription[0]);
            }
        }

        internal class RestartNodeHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static RestartNodeResult CreateFromNative(NativeClient.IFabricRestartNodeResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var selectedReplica = new SelectedReplica(0, SelectedPartition.None);
                var nodeResult = NodeResult.CreateFromNative(nativeResult.get_Result());

                var restartNodeResult = new RestartNodeResult(selectedReplica, nodeResult);

                GC.KeepAlive(nativeResult);
                return restartNodeResult;
            }
        }

        internal class StartNodeHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static System.Fabric.Result.StartNodeResult CreateFromNative(NativeClient.IFabricStartNodeResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var nodeResult = NodeResult.CreateFromNative(nativeResult.get_Result());

                var startNodeResult = new StartNodeResult(nodeResult.NodeName, nodeResult.NodeInstance);

                GC.KeepAlive(nativeResult);
                return startNodeResult;
            }
        }

        internal class StopNodeHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static StopNodeResult CreateFromNative(NativeClient.IFabricStopNodeResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var nodeResult = NodeResult.CreateFromNative(nativeResult.get_Result());

                var stopNodeResult = new StopNodeResult(nodeResult.NodeName, nodeResult.NodeInstance);

                GC.KeepAlive(nativeResult);
                return stopNodeResult;
            }
        }

        internal class RestartDeployedCodePackageHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static System.Fabric.Result.RestartDeployedCodePackageResult CreateFromNative(NativeClient.IFabricRestartDeployedCodePackageResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var restartDeployedCodePackageResult = RestartDeployedCodePackageResult.CreateFromNative(nativeResult.get_Result());

                GC.KeepAlive(nativeResult);
                return restartDeployedCodePackageResult;
            }
        }

        internal class MovePrimaryHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static System.Fabric.Result.MovePrimaryResult CreateFromNative(NativeClient.IFabricMovePrimaryResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var movePrimaryResult = MovePrimaryResult.CreateFromNative(nativeResult.get_Result());

                GC.KeepAlive(nativeResult);
                return movePrimaryResult;
            }
        }

        internal class MoveSecondaryHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static System.Fabric.Result.MoveSecondaryResult CreateFromNative(NativeClient.IFabricMoveSecondaryResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var movePrimaryResult = MoveSecondaryResult.CreateFromNative(nativeResult.get_Result());

                GC.KeepAlive(nativeResult);
                return movePrimaryResult;
            }
        }

        internal class ServiceGroupUpdateDescriptionHelper
        {
            internal static IntPtr ToNative(PinCollection pin, ServiceGroupUpdateDescription updateDescription)
            {
                if (updateDescription == null)
                {
                    return IntPtr.Zero;
                }

                var nativeDescription = new NativeTypes.FABRIC_SERVICE_GROUP_UPDATE_DESCRIPTION[1];

                nativeDescription[0].UpdateDescription = updateDescription.ServiceUpdateDescription.ToNative(pin);

                return pin.AddBlittable(nativeDescription[0]);
            }
        }

        internal class ServiceDescriptionHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = "Implementing with SafeHandle is a work item - Bug 221350")]
            internal static unsafe ServiceDescription CreateFromNative(NativeClient.IFabricServiceDescriptionResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var description = ServiceDescription.CreateFromNative(nativeResult.get_Description());
                GC.KeepAlive(nativeResult);

                return description;
            }
        }

        internal class ComFabricServicePartitionResolutionChangeHandler : NativeClient.IFabricServicePartitionResolutionChangeHandler
        {
            private readonly ServicePartitionResolutionChangeHandler callback;
            private readonly FabricClient client;

            private ComFabricServicePartitionResolutionChangeHandler(FabricClient client, ServicePartitionResolutionChangeHandler callback)
            {
                this.client = client;
                this.callback = callback;
            }

            public static NativeClient.IFabricServicePartitionResolutionChangeHandler ToNative(FabricClient client, ServicePartitionResolutionChangeHandler callback)
            {
                ReleaseAssert.AssertIfNot(client != null, "client != null");

                if (callback == null)
                {
                    return null;
                }
                else
                {
                    var native = new ComFabricServicePartitionResolutionChangeHandler(client, callback);
                    return native;
                }
            }

            [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "Match native API.")]
            public void OnChange(NativeClient.IFabricServiceManagementClient source, Int64 handlerId, NativeClient.IFabricResolvedServicePartitionResult partition, int error)
            {
                ReleaseAssert.AssertIfNot(
                    object.ReferenceEquals(source, this.client.NativePropertyClient),
                    "object.ReferenceEquals(source, this.client.NativePropertyClient)");

                ServicePartitionResolutionChange args = ServicePartitionResolutionChange.FromNative(partition, error);
                this.callback(this.client, handlerId, args);
            }
        }

        internal class UpgradeDomainHelper
        {
            internal static ReadOnlyCollection<UpgradeDomainStatus> CreateUpgradeDomainsFromNative(uint count, IntPtr pointer)
            {
                var collection = new Collection<UpgradeDomainStatus>();

                unsafe
                {
                    var array = (NativeTypes.FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION*)pointer;
                    for (int ix = 0; ix < count; ++ix)
                    {
                        var nativeDomainStatus = array[ix];

                        var name = NativeTypes.FromNativeString(nativeDomainStatus.Name);

                        var domainStatus = new UpgradeDomainStatus() { Name = name };

                        switch (nativeDomainStatus.State)
                        {
                            case NativeTypes.FABRIC_UPGRADE_DOMAIN_STATE.FABRIC_UPGRADE_DOMAIN_STATE_PENDING:
                            case NativeTypes.FABRIC_UPGRADE_DOMAIN_STATE.FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS:
                            case NativeTypes.FABRIC_UPGRADE_DOMAIN_STATE.FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED:
                                domainStatus.State = (UpgradeDomainState)nativeDomainStatus.State;
                                break;

                            default:
                                domainStatus.State = UpgradeDomainState.Invalid;
                                break;
                        }

                        collection.Add(domainStatus);
                    }
                }

                return new ReadOnlyCollection<UpgradeDomainStatus>(collection);
            }
        }

        internal class ApplicationUpgradeProgressHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static ApplicationUpgradeProgress CreateFromNative(NativeClient.IFabricApplicationUpgradeProgressResult3 nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var progress = new ApplicationUpgradeProgress(nativeResult);

                progress.ApplicationName = NativeTypes.FromNativeUri(nativeResult.get_ApplicationName());
                progress.ApplicationTypeName = NativeTypes.FromNativeString(nativeResult.get_ApplicationTypeName());
                progress.TargetApplicationTypeVersion = NativeTypes.FromNativeString(nativeResult.get_TargetApplicationTypeVersion());

                switch (nativeResult.get_UpgradeState())
                {
                    case NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLBACK_IN_PROGRESS:
                    case NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLBACK_COMPLETED:
                    case NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLBACK_PENDING:
                    case NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLFORWARD_PENDING:
                    case NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLFORWARD_IN_PROGRESS:
                    case NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLFORWARD_COMPLETED:
                    case NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_FAILED:
                        progress.UpgradeState = (ApplicationUpgradeState)nativeResult.get_UpgradeState();
                        break;

                    default:
                        progress.UpgradeState = ApplicationUpgradeState.Invalid;
                        break;
                }

                uint count = 0;
                IntPtr pointer = nativeResult.GetUpgradeDomains(out count);

                progress.UpgradeDomains = UpgradeDomainHelper.CreateUpgradeDomainsFromNative(count, pointer);

                switch (nativeResult.get_RollingUpgradeMode())
                {
                    case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
                    case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
                    case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
                        progress.RollingUpgradeMode = (RollingUpgradeMode)nativeResult.get_RollingUpgradeMode();
                        break;

                    default:
                        progress.RollingUpgradeMode = RollingUpgradeMode.Invalid;
                        break;
                }

                progress.NextUpgradeDomain = NativeTypes.FromNativeString(nativeResult.get_NextUpgradeDomain());

                IntPtr upgradeProgressPtr = nativeResult.get_UpgradeProgress();
                if (upgradeProgressPtr != IntPtr.Zero)
                {
                    unsafe
                    {
                        var castedProgressPtr = (NativeTypes.FABRIC_APPLICATION_UPGRADE_PROGRESS*)upgradeProgressPtr;
                        progress.UpgradeDescription = ApplicationUpgradeDescription.FromNative(castedProgressPtr->UpgradeDescription);
                        progress.UpgradeDuration = TimeSpan.FromSeconds(castedProgressPtr->UpgradeDurationInSeconds);
                        progress.CurrentUpgradeDomainDuration = TimeSpan.FromSeconds(castedProgressPtr->CurrentUpgradeDomainDurationInSeconds);
                        progress.UnhealthyEvaluations = HealthEvaluation.FromNativeList(castedProgressPtr->UnhealthyEvaluations);
                        progress.CurrentUpgradeDomainProgress = UpgradeDomainProgress.FromNative(castedProgressPtr->CurrentUpgradeDomainProgress);

                        if (castedProgressPtr->Reserved != IntPtr.Zero)
                        {
                            var castedProgressEx1Ptr = (NativeTypes.FABRIC_APPLICATION_UPGRADE_PROGRESS_EX1*)castedProgressPtr->Reserved;
                            progress.StartTimestampUtc = NativeTypes.FromNullableNativeFILETIME(castedProgressEx1Ptr->StartTimestampUtc);
                            progress.FailureTimestampUtc = NativeTypes.FromNullableNativeFILETIME(castedProgressEx1Ptr->FailureTimestampUtc);

                            if (progress.FailureTimestampUtc.HasValue)
                            {
                                switch (castedProgressEx1Ptr->FailureReason)
                                {
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_INTERRUPTED:
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_HEALTH_CHECK:
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_UPGRADE_DOMAIN_TIMEOUT:
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_OVERALL_UPGRADE_TIMEOUT:
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_PROCESSING_FAILURE:
                                        progress.FailureReason = (UpgradeFailureReason)castedProgressEx1Ptr->FailureReason;
                                        break;

                                    default:
                                        progress.FailureReason = UpgradeFailureReason.None;
                                        break;
                                }

                                progress.UpgradeDomainProgressAtFailure = UpgradeDomainProgress.FromNative(
                                    castedProgressEx1Ptr->UpgradeDomainProgressAtFailure);
                            }

                            if (castedProgressEx1Ptr->Reserved != IntPtr.Zero)
                            {
                                var castedProgressEx2Ptr = (NativeTypes.FABRIC_APPLICATION_UPGRADE_PROGRESS_EX2*)castedProgressEx1Ptr->Reserved;
                                progress.UpgradeStatusDetails = NativeTypes.FromNativeString(castedProgressEx2Ptr->UpgradeStatusDetails);
                            }
                        }
                    }
                }


                GC.KeepAlive(nativeResult);
                return progress;
            }

            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static ReadOnlyCollection<UpgradeDomainStatus> GetChangedUpgradeDomainsFromNative(NativeClient.IFabricApplicationUpgradeProgressResult previousProgress, NativeClient.IFabricApplicationUpgradeProgressResult upgradeProgress)
            {
                uint count = 0;
                IntPtr pointer = upgradeProgress.GetChangedUpgradeDomains(previousProgress, out count);

                ReadOnlyCollection<UpgradeDomainStatus> returnValue = UpgradeDomainHelper.CreateUpgradeDomainsFromNative(count, pointer);

                if (previousProgress != null)
                {
                    GC.KeepAlive(previousProgress);
                }

                GC.KeepAlive(upgradeProgress);

                return returnValue;
            }
        }

        internal class InvokeDataLossProgressHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static PartitionDataLossProgress CreateFromNative(NativeClient.IFabricPartitionDataLossProgressResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var progress = PartitionDataLossProgress.FromNative(nativeResult.get_Progress());

                GC.KeepAlive(nativeResult);
                return progress;
            }
        }

        internal class InvokeQuorumLossProgressHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static PartitionQuorumLossProgress CreateFromNative(NativeClient.IFabricPartitionQuorumLossProgressResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var progress = PartitionQuorumLossProgress.FromNative(nativeResult.get_Progress());

                GC.KeepAlive(nativeResult);
                return progress;
            }
        }

        internal class RestartPartitionProgressHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static PartitionRestartProgress CreateFromNative(NativeClient.IFabricPartitionRestartProgressResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var progress = PartitionRestartProgress.FromNative(nativeResult.get_Progress());

                GC.KeepAlive(nativeResult);
                return progress;
            }
        }

        internal class NodeTransitionProgressHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static NodeTransitionProgress CreateFromNative(NativeClient.IFabricNodeTransitionProgressResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var progress = NodeTransitionProgress.FromNative(nativeResult.get_Progress());

                GC.KeepAlive(nativeResult);
                return progress;
            }
        }

        internal class GetTestCommandListHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static unsafe TestCommandStatusList CreateFromNative(NativeClient.IFabricTestCommandStatusResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                NativeTypes.FABRIC_TEST_COMMAND_STATUS_LIST* native = (NativeTypes.FABRIC_TEST_COMMAND_STATUS_LIST*)nativeResult.get_Result();

                var resultList = new List<TestCommandStatus>();

                var nativeItemArray = (NativeTypes.FABRIC_TEST_COMMAND_STATUS*)native->Items;
                for (int i = 0; i < native->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    TestCommandStatus item = new TestCommandStatus(nativeItem.OperationId, TestCommandStateHelper.FromNative(nativeItem.TestCommandState), TestCommandTypeHelper.FromNative(nativeItem.TestCommandType));
                    resultList.Add(item);
                }

                TestCommandStatusList queryList = new TestCommandStatusList(resultList);
                GC.KeepAlive(nativeResult);

                return queryList;
            }
        }

        internal class GetChaosHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static unsafe ChaosDescription CreateFromNative(NativeClient.IFabricChaosDescriptionResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var chaosDescription = ChaosDescription.FromNative(nativeResult.get_ChaosDescriptionResult());

                GC.KeepAlive(nativeResult);
                return chaosDescription;
            }
        }

        internal class GetChaosScheduleHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static unsafe ChaosScheduleDescription CreateFromNative(NativeClient.IFabricChaosScheduleDescriptionResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var chaosScheduleDescription = ChaosScheduleDescription.FromNative(nativeResult.get_ChaosScheduleDescriptionResult());

                GC.KeepAlive(nativeResult);
                return chaosScheduleDescription;
            }
        }

        internal class GetChaosReportHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static unsafe ChaosReport CreateFromNative(NativeClient.IFabricChaosReportResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var chaosReport = ChaosReport.FromNative(nativeResult.get_ChaosReportResult());

                GC.KeepAlive(nativeResult);
                return chaosReport;
            }
        }

        internal class GetChaosEventsHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static unsafe ChaosEventsSegment CreateFromNative(NativeClient.IFabricChaosEventsSegmentResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var chaosEventsSegment = ChaosEventsSegment.FromNative(nativeResult.get_ChaosEventsSegmentResult());

                GC.KeepAlive(nativeResult);
                return chaosEventsSegment;
            }
        }

        internal class FabricUpgradeProgressHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static FabricUpgradeProgress CreateFromNative(NativeClient.IFabricUpgradeProgressResult3 nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var progress = new FabricUpgradeProgress(nativeResult);

                progress.TargetCodeVersion = NativeTypes.FromNativeString(nativeResult.get_TargetCodeVersion());
                progress.TargetConfigVersion = NativeTypes.FromNativeString(nativeResult.get_TargetConfigVersion());

                switch (nativeResult.get_UpgradeState())
                {
                    case NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLBACK_IN_PROGRESS:
                    case NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLBACK_COMPLETED:
                    case NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLBACK_PENDING:
                    case NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLFORWARD_PENDING:
                    case NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLFORWARD_IN_PROGRESS:
                    case NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLFORWARD_COMPLETED:
                    case NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_FAILED:
                        progress.UpgradeState = (FabricUpgradeState)nativeResult.get_UpgradeState();
                        break;

                    default:
                        progress.UpgradeState = FabricUpgradeState.Invalid;
                        break;
                }

                uint count = 0;
                IntPtr pointer = nativeResult.GetUpgradeDomains(out count);

                progress.UpgradeDomains = UpgradeDomainHelper.CreateUpgradeDomainsFromNative(count, pointer);

                switch (nativeResult.get_RollingUpgradeMode())
                {
                    case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
                    case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
                    case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
                        progress.RollingUpgradeMode = (RollingUpgradeMode)nativeResult.get_RollingUpgradeMode();
                        break;

                    default:
                        progress.RollingUpgradeMode = RollingUpgradeMode.Invalid;
                        break;
                }

                progress.NextUpgradeDomain = NativeTypes.FromNativeString(nativeResult.get_NextUpgradeDomain());

                IntPtr upgradeProgressPtr = nativeResult.get_UpgradeProgress();
                if (upgradeProgressPtr != IntPtr.Zero)
                {
                    unsafe
                    {
                        var castedProgressPtr = (NativeTypes.FABRIC_UPGRADE_PROGRESS*)upgradeProgressPtr;
                        progress.UpgradeDescription = FabricUpgradeDescription.FromNative(castedProgressPtr->UpgradeDescription);
                        progress.UpgradeDuration = TimeSpan.FromSeconds(castedProgressPtr->UpgradeDurationInSeconds);
                        progress.CurrentUpgradeDomainDuration = TimeSpan.FromSeconds(castedProgressPtr->CurrentUpgradeDomainDurationInSeconds);
                        progress.UnhealthyEvaluations = HealthEvaluation.FromNativeList(castedProgressPtr->UnhealthyEvaluations);
                        progress.CurrentUpgradeDomainProgress = UpgradeDomainProgress.FromNative(castedProgressPtr->CurrentUpgradeDomainProgress);

                        if (castedProgressPtr->Reserved != IntPtr.Zero)
                        {
                            var castedProgressEx1Ptr = (NativeTypes.FABRIC_UPGRADE_PROGRESS_EX1*)castedProgressPtr->Reserved;
                            progress.StartTimestampUtc = NativeTypes.FromNullableNativeFILETIME(castedProgressEx1Ptr->StartTimestampUtc);
                            progress.FailureTimestampUtc = NativeTypes.FromNullableNativeFILETIME(castedProgressEx1Ptr->FailureTimestampUtc);

                            if (progress.FailureTimestampUtc.HasValue)
                            {
                                switch (castedProgressEx1Ptr->FailureReason)
                                {
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_INTERRUPTED:
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_HEALTH_CHECK:
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_UPGRADE_DOMAIN_TIMEOUT:
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_OVERALL_UPGRADE_TIMEOUT:
                                    case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_PROCESSING_FAILURE:
                                        progress.FailureReason = (UpgradeFailureReason)castedProgressEx1Ptr->FailureReason;
                                        break;

                                    default:
                                        progress.FailureReason = UpgradeFailureReason.None;
                                        break;
                                }

                                progress.UpgradeDomainProgressAtFailure = UpgradeDomainProgress.FromNative(
                                    castedProgressEx1Ptr->UpgradeDomainProgressAtFailure);
                            }
                        }
                    }
                }

                GC.KeepAlive(nativeResult);
                return progress;
            }

            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static ReadOnlyCollection<UpgradeDomainStatus> GetChangedUpgradeDomainsFromNative(NativeClient.IFabricUpgradeProgressResult previousProgress, NativeClient.IFabricUpgradeProgressResult upgradeProgress)
            {
                uint count = 0;
                IntPtr pointer = upgradeProgress.GetChangedUpgradeDomains(previousProgress, out count);

                ReadOnlyCollection<UpgradeDomainStatus> returnValue = UpgradeDomainHelper.CreateUpgradeDomainsFromNative(count, pointer);

                if (previousProgress != null)
                {
                    GC.KeepAlive(previousProgress);
                }

                GC.KeepAlive(upgradeProgress);

                return returnValue;
            }
        }

        internal class GetApplicationManifestHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static string CreateFromNative(NativeCommon.IFabricStringResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var result = NativeTypes.FromNativeString(nativeResult.get_String());

                GC.KeepAlive(nativeResult);

                return result;

            }
        }

        internal class ResolvedServiceEndpointCollectionHelper
        {
            internal static unsafe List<ResolvedServiceEndpoint> CreateFromNative(int count, IntPtr endpoints)
            {
                var collection = new List<ResolvedServiceEndpoint>();
                var nativeArray = (NativeTypes.FABRIC_RESOLVED_SERVICE_ENDPOINT*)endpoints;

                for (int i = 0; i < count; i++)
                {
                    var item = ResolvedServiceEndpointHelper.CreateFromNative((IntPtr)(nativeArray + i));
                    collection.Add(item);
                }

                return collection;
            }
        }

        internal class ResolvedServiceEndpointHelper
        {
            internal static unsafe ResolvedServiceEndpoint CreateFromNative(IntPtr endpointIntPtr)
            {
                var endpoint = new ResolvedServiceEndpoint();
                var nativeEndpoint = (NativeTypes.FABRIC_RESOLVED_SERVICE_ENDPOINT*)endpointIntPtr;

                string address = NativeTypes.FromNativeString(nativeEndpoint->Address);

                endpoint.Address = address;

                endpoint.Role = (ServiceEndpointRole)nativeEndpoint->Role;

                return endpoint;
            }
        }

        internal class ExpressionValidatorHelper
        {
            public static bool IsValidExpression(string expression)
            {
                using (var pin = new PinCollection())
                {
                    return NativeTypes.FromBOOLEAN(NativeCommon.FabricIsValidExpression(pin.AddBlittable(expression)));
                }
            }
        }

        internal class GetClusterManifestHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static string CreateFromNative(NativeCommon.IFabricStringResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var result = NativeTypes.FromNativeString(nativeResult.get_String());

                GC.KeepAlive(nativeResult);

                return result;
            }
        }

        internal static Exception TranslateError(int errorCode)
        {
            return TranslateError(errorCode, "Operation failed");
        }

        internal static Exception TranslateError(int errorCode, string message)
        {
            if (errorCode == 0)
            {
                return null;
            }

            Func<Exception, string, Exception> translateExceptionFunction = null;

            if (!System.Fabric.Interop.InteropExceptionMap.NativeToManagedConversion.TryGetValue(errorCode, out translateExceptionFunction))
            {
                throw new System.Fabric.FabricException(StringResources.Error_Unknown, errorCode);
            }

            // Creates an inner com exception with the native error code as well and error message.
            // This can be improved by somehow getting the original exception thrown which was handled by the code
            Exception exceptionToThrow = translateExceptionFunction(new COMException(message, errorCode), message);
            return exceptionToThrow;
        }

        internal class OrchestrationUpgradeProgressHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static FabricOrchestrationUpgradeProgress CreateFromNative(NativeClient.IFabricOrchestrationUpgradeStatusResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var progress = FabricOrchestrationUpgradeProgress.FromNative(nativeResult.get_Progress());

                GC.KeepAlive(nativeResult);
                return progress;
            }
        }

        internal class FabricUpgradeOrchestrationServiceStateHelper
        {
            [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive,
                Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
            internal static FabricUpgradeOrchestrationServiceState CreateFromNative(NativeClient.IFabricUpgradeOrchestrationServiceStateResult nativeResult)
            {
                if (nativeResult == null)
                {
                    return null;
                }

                var state = FabricUpgradeOrchestrationServiceState.FromNative(nativeResult.get_State());

                GC.KeepAlive(nativeResult);
                return state;
            }
        }

        internal static NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE ToNativeServicePackageActivationMode(
            ServicePackageActivationMode activationMode)
        {
            switch (activationMode)
            {
                case ServicePackageActivationMode.SharedProcess:
                    return NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE.FABRIC_ISOLATION_LEVEL_SHARED_PROCESS;

                case ServicePackageActivationMode.ExclusiveProcess:
                    return NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE.FABRIC_ISOLATION_LEVEL_EXCLUSIVE_PROCESS;

                default:
                    var errorMessage = string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_EnumInvalid_Formatted,
                        activationMode,
                        "ServicePackageActivationMode");

                    AppTrace.TraceSource.WriteError(
                       TraceComponent,
                       "InteropHelpersToNativeServicePackageActivationMode(): " + errorMessage,
                       activationMode);

                    throw new ArgumentException(errorMessage);
            }
        }

        internal static ServicePackageActivationMode FromNativeServicePackageActivationMode(
            NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE nativeActivationMode)
        {
            switch (nativeActivationMode)
            {
                case NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE.FABRIC_ISOLATION_LEVEL_SHARED_PROCESS:
                    return ServicePackageActivationMode.SharedProcess;

                case NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE.FABRIC_ISOLATION_LEVEL_EXCLUSIVE_PROCESS:
                    return ServicePackageActivationMode.ExclusiveProcess;

                default:
                    var errorMessage = string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_EnumInvalid_Formatted,
                        nativeActivationMode,
                        "NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE");

                    AppTrace.TraceSource.WriteError(
                        TraceComponent,
                        "InteropHelpers.FromNativeServicePackageActivationMode(): " + errorMessage,
                        nativeActivationMode);

                    throw new ArgumentException(errorMessage);
            }
        }

        internal static ContainerIsolationMode FromNativeContainerIsolationMode(
            NativeTypes.FABRIC_CONTAINER_ISOLATION_MODE nativeIsolationMode)
        {
            switch (nativeIsolationMode)
            {
                case NativeTypes.FABRIC_CONTAINER_ISOLATION_MODE.FABRIC_CONTAINER_ISOLATION_MODE_UNKNOWN:
                    return ContainerIsolationMode.Unknown;

                case NativeTypes.FABRIC_CONTAINER_ISOLATION_MODE.FABRIC_CONTAINER_ISOLATION_MODE_PROCESS:
                    return ContainerIsolationMode.Process;

                case NativeTypes.FABRIC_CONTAINER_ISOLATION_MODE.FABRIC_CONTAINER_ISOLATION_MODE_HYPER_V:
                    return ContainerIsolationMode.HyperV;

                default:
                    throw new ArgumentException("Invalid value for FABRIC_CONTAINER_ISOLATION_MODE");
            }
        }

        internal static ContainerNetworkType FromNativeContainerNetworkType(
            NativeTypes.FABRIC_CONTAINER_NETWORK_TYPE nativeContainerNetworkType)
        {
            ContainerNetworkType networkType = ContainerNetworkType.Other;

            if ((nativeContainerNetworkType & NativeTypes.FABRIC_CONTAINER_NETWORK_TYPE.FABRIC_CONTAINER_NETWORK_TYPE_OPEN)
                == NativeTypes.FABRIC_CONTAINER_NETWORK_TYPE.FABRIC_CONTAINER_NETWORK_TYPE_OPEN)
            {
                networkType = networkType | ContainerNetworkType.Open;
            }

            if ((nativeContainerNetworkType & NativeTypes.FABRIC_CONTAINER_NETWORK_TYPE.FABRIC_CONTAINER_NETWORK_TYPE_ISOLATED)
                == NativeTypes.FABRIC_CONTAINER_NETWORK_TYPE.FABRIC_CONTAINER_NETWORK_TYPE_ISOLATED)
            {
                networkType = networkType | ContainerNetworkType.Isolated;
            }

            return networkType;
        }

        internal static NativeTypes.FABRIC_CONTAINER_EVENT_TYPE ToNativeContainerEventType(
            ContainerEventType eventType)
        {
            switch (eventType)
            {
                case ContainerEventType.None:
                    return NativeTypes.FABRIC_CONTAINER_EVENT_TYPE.FABRIC_CONTAINER_EVENT_TYPE_NONE;

                case ContainerEventType.Stop:
                    return NativeTypes.FABRIC_CONTAINER_EVENT_TYPE.FABRIC_CONTAINER_EVENT_TYPE_STOP;

                case ContainerEventType.Die:
                    return NativeTypes.FABRIC_CONTAINER_EVENT_TYPE.FABRIC_CONTAINER_EVENT_TYPE_DIE;

                case ContainerEventType.Health:
                    return NativeTypes.FABRIC_CONTAINER_EVENT_TYPE.FABRIC_CONTAINER_EVENT_TYPE_HEALTH;

                default:
                    var errorMessage = string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_EnumInvalid_Formatted,
                        eventType,
                        "ContainerEventType");

                    AppTrace.TraceSource.WriteError(
                       TraceComponent,
                       "InteropHelpers.ToNativeContainerEventType(): " + errorMessage,
                       eventType);

                    throw new ArgumentException(errorMessage);
            }
        }
    }
}
