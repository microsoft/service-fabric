// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Repair;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading.Tasks;

    using ThrowIf = System.Fabric.Common.ThrowIf;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    internal class FabricClientState
    {
        private const string TraceSource = "FabricClientState";

        public const int ServiceKindStateless = unchecked((int)0x0001);
        public const int ServiceKindStateful = unchecked((int)0x0002);

        public FabricClientState()
        {
            this.ApplicationUpgradeProgressHolder = new ObjectHolder<ApplicationUpgradeProgress>();
            this.FabricUpgradeProgressHolder = new ObjectHolder<FabricUpgradeProgress>();
            this.PartitionHolder = new ObjectHolder<ResolvedServicePartition>();
            this.EnumerationHolder = new ObjectHolder<EnumerationWrapper>();
            this.BatchResultHolder = new ObjectHolder<PropertyBatchResult>();
        }

        public ObjectHolder<ResolvedServicePartition> PartitionHolder { get; private set; }

        public ObjectHolder<ApplicationUpgradeProgress> ApplicationUpgradeProgressHolder { get; private set; }

        public ObjectHolder<FabricUpgradeProgress> FabricUpgradeProgressHolder { get; private set; }

        public ObjectHolder<EnumerationWrapper> EnumerationHolder { get; private set; }

        public ObjectHolder<PropertyBatchResult> BatchResultHolder { get; private set; }


        public static OperationResult<T> CreateOperationResultFromNativeErrorCode<T>(NativeTypes.FABRIC_ERROR_CODE errorCode, T result)
        {
            return new OperationResult<T>((uint)errorCode, result);
        }

        public static OperationResult<T> CreateOperationResultFromNativeErrorCode<T>(NativeTypes.FABRIC_ERROR_CODE errorCode)
        {
            return new OperationResult<T>((uint)errorCode, default(T));
        }

        public static OperationResult<T> CreateOperationResultFromNativeErrorCode<T>(T result)
        {
            return new OperationResult<T>(0, result);
        }

        public static OperationResult CreateOperationResultFromNativeErrorCode(NativeTypes.FABRIC_ERROR_CODE errorCode)
        {
            return new OperationResult((uint)errorCode);
        }

        public static T ConvertToEnum<T>(string name)
        {
            ThrowIf.NullOrEmpty(name, "name");
            return (T)Enum.Parse(typeof(T), name);
        }

        public static OperationResult<TestServicePartitionInfo[]> Convert(ResolvedServicePartition resolvedServicePartition, int partitionIdentifierNumber)
        {
            OperationResult<TestServicePartitionInfo[]> result = null;

            if (resolvedServicePartition != null)
            {
                List<TestServicePartitionInfo> partitionInfos = new List<TestServicePartitionInfo>();

                foreach (ResolvedServiceEndpoint resolvedEndpoint in resolvedServicePartition.Endpoints)
                {
                    partitionInfos.Add(FabricClientState.Convert(resolvedServicePartition, resolvedEndpoint, partitionIdentifierNumber));
                }

                result = FabricClientState.CreateOperationResultFromNativeErrorCode<TestServicePartitionInfo[]>(partitionInfos.ToArray());
            }

            return result;
        }

        public static TestServicePartitionInfo Convert(ResolvedServicePartition resolvedPartition, ResolvedServiceEndpoint resolvedEndpoint, int partitionIdentifierNumber)
        {
            ThrowIf.Null(resolvedPartition, "resolvedPartition");
            ThrowIf.Null(resolvedEndpoint, "resolvedEndpoint");
            ServicePartitionKind keyType = ServicePartitionKind.Invalid;

            object rangeHighKey = null;
            object rangeLowKey = null;

            switch (resolvedPartition.Info.Kind)
            {
                case ServicePartitionKind.Singleton:
                    keyType = ServicePartitionKind.Singleton;
                    break;

                case ServicePartitionKind.Int64Range:
                    keyType = ServicePartitionKind.Int64Range;
                    rangeHighKey = ((System.Fabric.Int64RangePartitionInformation)resolvedPartition.Info).HighKey;
                    rangeLowKey = ((System.Fabric.Int64RangePartitionInformation)resolvedPartition.Info).LowKey;
                    break;

                case ServicePartitionKind.Named:
                    keyType = ServicePartitionKind.Named;
                    break;

                default:
                    throw new InvalidOperationException("Unknown ServicePartitionKind " + resolvedPartition.Info.Kind.ToString() + ".");
            }

            TestServicePartitionInfo testServicePartitionInfo = new TestServicePartitionInfo()
            {
                Location = resolvedEndpoint.Address,
                Name = resolvedPartition.ServiceName,
                Id = resolvedPartition.Info.Id,
                KeyType = keyType,
                RangeHighKey = rangeHighKey,
                RangeLowKey = rangeLowKey,
                Role = resolvedEndpoint.Role,
                PartitionIdentifier = partitionIdentifierNumber,
            };

            if (resolvedPartition.Endpoints.Any(e => e.Role == ServiceEndpointRole.StatefulPrimary))
            {
                testServicePartitionInfo.IsPrimaryEndpoint = resolvedPartition.GetEndpoint().Address == resolvedEndpoint.Address;
            }
            else
            {
                testServicePartitionInfo.IsPrimaryEndpoint = false;
            }

            return testServicePartitionInfo;
        }
       
        public static async Task<NativeTypes.FABRIC_ERROR_CODE> HandleExceptionAsync(Func<Task> operation)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode;
            try
            {
                await operation();
                errorCode = 0; // Success
            }
            catch (AggregateException e)
            {
                if(!TryGetErrorCode(e.InnerException, out errorCode))
                {
                    throw;
                }
            }
            catch (Exception e)
            {
                if (!TryGetErrorCode(e, out errorCode))
                {
                    throw;
                }
            }

            return errorCode;
        }

        public static NativeTypes.FABRIC_ERROR_CODE HandleException(Action action)
        {
            Task<NativeTypes.FABRIC_ERROR_CODE> task = FabricClientState.HandleExceptionAsync(delegate
            {
                action();
                return TaskUtility.CreateCompletedTask();
            });

            return task.Result;
        }

        public NameEnumerationResult Convert(WinFabricNameEnumerationResult previousResult)
        {
            NameEnumerationResult result = null;

            if (previousResult != null)
            {
                result = this.EnumerationHolder.RetrieveObject(previousResult.Identifier).RetrieveEnumeration<NameEnumerationResult>();
            }

            return result;
        }

        public WinFabricNameEnumerationResult Convert(NameEnumerationResult result)
        {
            WinFabricNameEnumerationResult actualResult = null;

            if (result != null)
            {
                int id = this.EnumerationHolder.StoreObject(new EnumerationWrapper(result));
                actualResult = new WinFabricNameEnumerationResult(id, result);
            }

            return actualResult;
        }

        public OperationResult<WinFabricPropertyEnumerationResult> ConvertToWinFabricPropertyEnumerationResult(PropertyEnumerationResult enumerationResult, uint errorCode)
        {
            OperationResult<WinFabricPropertyEnumerationResult> actualResult = new OperationResult<WinFabricPropertyEnumerationResult>(errorCode);
            if (enumerationResult != null)
            {
                WinFabricPropertyEnumerationResult testProperty = this.Convert(enumerationResult);
                actualResult = FabricClientState.CreateOperationResultFromNativeErrorCode<WinFabricPropertyEnumerationResult>(testProperty);
            }

            return actualResult;
        }

        public PropertyEnumerationResult Convert(WinFabricPropertyEnumerationResult previousResult)
        {
            PropertyEnumerationResult result = null;

            if (previousResult != null)
            {
                result = this.EnumerationHolder.RetrieveObject(previousResult.Identifier).RetrieveEnumeration<PropertyEnumerationResult>();
            }

            return result;
        }

        public WinFabricPropertyEnumerationResult Convert(PropertyEnumerationResult result)
        {
            WinFabricPropertyEnumerationResult actualResult = null;

            if (result != null)
            {
                int id = this.EnumerationHolder.StoreObject(new EnumerationWrapper(result));
                actualResult = new WinFabricPropertyEnumerationResult(id, result);
            }

            return actualResult;
        }

        public OperationResult<WinFabricPropertyBatchResult> ConvertToWinFabricPropertyBatchResult(uint errorCode, PropertyBatchResult batchResult)
        {
            var result = new OperationResult<WinFabricPropertyBatchResult>(errorCode);
            if (batchResult != null)
            {
                int id = this.BatchResultHolder.StoreObject(batchResult);

                FabricErrorCode indexErrorCode = 0;
                if (batchResult.FailedOperationException != null)
                {
                    if (batchResult.FailedOperationException is OperationCanceledException || batchResult.FailedOperationException is TimeoutException)
                    {
                        throw batchResult.FailedOperationException;
                    }

                    FabricException exception = batchResult.FailedOperationException as FabricException;
                    if (exception == null)
                    {
                        throw new ArgumentException("batchResult.FailedOperationException {0} should be a FabricException", batchResult.FailedOperationException);
                    }

                    indexErrorCode = exception.ErrorCode;
                }

                WinFabricPropertyBatchResult testBatch = new WinFabricPropertyBatchResult(id)
                {
                    Result = batchResult,
                    FailedOperationErrorCode = indexErrorCode,
                };

                result = FabricClientState.CreateOperationResultFromNativeErrorCode<WinFabricPropertyBatchResult>(testBatch);
            }

            return result;
        }

        public OperationResult<WinFabricNameEnumerationResult> ConvertToWinFabricNameEnumerationResult(NameEnumerationResult enumerationResult, uint errorCode)
        {
            OperationResult<WinFabricNameEnumerationResult> actualResult = new OperationResult<WinFabricNameEnumerationResult>(errorCode);
            if (enumerationResult != null)
            {
                WinFabricNameEnumerationResult testProperty = this.Convert(enumerationResult);
                actualResult = FabricClientState.CreateOperationResultFromNativeErrorCode<WinFabricNameEnumerationResult>(testProperty);
            }

            return actualResult;
        }

        public OperationResult<TestServicePartitionInfo[]> ConvertToServicePartitionResult(NativeTypes.FABRIC_ERROR_CODE errorCode, ResolvedServicePartition resolvedPartition)
        {
            OperationResult<TestServicePartitionInfo[]> result = FabricClientState.CreateOperationResultFromNativeErrorCode<TestServicePartitionInfo[]>(errorCode);
            if (resolvedPartition != null)
            {
                // Store ResolvedServicePartition object when the users need to call apis on this object
                int id = this.PartitionHolder.StoreObject(resolvedPartition);
                result = FabricClientState.Convert(resolvedPartition, id);
            }

            return result;
        }

        public static OperationResult<WinFabricRepairTask[]> ConvertToWinFabricRepairTaskList(IList<RepairTask> repairTaskList, NativeTypes.FABRIC_ERROR_CODE errorCode)
        {
            OperationResult<WinFabricRepairTask[]> actualResult = FabricClientState.CreateOperationResultFromNativeErrorCode<WinFabricRepairTask[]>(errorCode);
            if (repairTaskList != null)
            {
                IList<WinFabricRepairTask> winFabricRepairTasks = new List<WinFabricRepairTask>(repairTaskList.Count);

                foreach (RepairTask repairTask in repairTaskList)
                {
                    WinFabricRepairTask winFabricRepairTask = Convert(repairTask);
                    winFabricRepairTasks.Add(winFabricRepairTask);
                }

                actualResult = FabricClientState.CreateOperationResultFromNativeErrorCode<WinFabricRepairTask[]>(winFabricRepairTasks.ToArray());
            }

            return actualResult;
        }

        public static RepairTask Convert(WinFabricRepairTask winFabricRepairTask)
        {
            ThrowIf.Null(winFabricRepairTask, "winFabricRepairTask");

            var repairTask = new ClusterRepairTask(winFabricRepairTask.TaskId, winFabricRepairTask.Action)
                {
                    Version = winFabricRepairTask.Version, 
                    Description = winFabricRepairTask.Description, 
                    State = Convert(winFabricRepairTask.State), 
                    Target = Convert(winFabricRepairTask.Target), 
                    Executor = winFabricRepairTask.Executor, 
                    ExecutorData = winFabricRepairTask.ExecutorData,
                    Impact = Convert(winFabricRepairTask.Impact), 
                    ResultStatus = Convert(winFabricRepairTask.ResultStatus), 
                    ResultCode = winFabricRepairTask.ResultCode, 
                    ResultDetails = winFabricRepairTask.ResultDetails
                };

            return repairTask;
        }

        public static void UpdateRepairTask(WinFabricRepairTask winFabricRepairTask, RepairTask repairTask)
        {
            ThrowIf.Null(winFabricRepairTask, "winFabricRepairTask");

            repairTask.Version = winFabricRepairTask.Version;
            repairTask.Description = winFabricRepairTask.Description;
            repairTask.State = Convert(winFabricRepairTask.State);
            repairTask.Target = Convert(winFabricRepairTask.Target);
            repairTask.Executor = winFabricRepairTask.Executor;
            repairTask.ExecutorData = winFabricRepairTask.ExecutorData;
            repairTask.Impact = Convert(winFabricRepairTask.Impact);
            repairTask.ResultStatus = Convert(winFabricRepairTask.ResultStatus);
            repairTask.ResultCode = winFabricRepairTask.ResultCode;
            repairTask.ResultDetails = winFabricRepairTask.ResultDetails;
        }

        public static RepairTaskState Convert(WinFabricRepairTaskState winFabricRepairTaskState)
        {
            if (!Enum.IsDefined(typeof(WinFabricRepairTaskState), winFabricRepairTaskState))
            {
                throw new ArgumentException("Invalid WinFabricRepairTaskState value " + winFabricRepairTaskState, "winFabricRepairTaskState");
            }

            return (RepairTaskState) winFabricRepairTaskState;
        }

        public static RepairTaskStateFilter Convert(WinFabricRepairTaskStateFilter winFabricRepairTaskStateFilter)
        {
            if (winFabricRepairTaskStateFilter != (winFabricRepairTaskStateFilter & WinFabricRepairTaskStateFilter.All))
            {
                throw new ArgumentException("Invalid WinFabricRepairTaskStateFilter value " + winFabricRepairTaskStateFilter, "winFabricRepairTaskStateFilter");
            }

            return (RepairTaskStateFilter)winFabricRepairTaskStateFilter;
        }

        private static WinFabricRepairTask Convert(RepairTask repairTask)
        {
            ThrowIf.Null(repairTask, "repairTask");

            return new WinFabricRepairTask()
                {
                    TaskId = repairTask.TaskId,
                    Version = repairTask.Version,
                    Description = repairTask.Description,
                    State = Convert(repairTask.State),
                    Flags = Convert(repairTask.Flags),
                    Action = repairTask.Action,
                    Target = Convert(repairTask.Target),
                    Executor = repairTask.Executor,
                    ExecutorData = repairTask.ExecutorData,
                    Impact = Convert(repairTask.Impact),
                    ResultStatus = Convert(repairTask.ResultStatus),
                    ResultCode = repairTask.ResultCode,
                    ResultDetails = repairTask.ResultDetails,
                    CreatedTimestamp = repairTask.CreatedTimestamp,
                    ClaimedTimestamp = repairTask.ClaimedTimestamp,
                    PreparingTimestamp = repairTask.PreparingTimestamp,
                    ApprovedTimestamp = repairTask.ApprovedTimestamp,
                    ExecutingTimestamp = repairTask.ExecutingTimestamp,
                    RestoringTimestamp = repairTask.RestoringTimestamp,
                    CompletedTimestamp = repairTask.CompletedTimestamp
                };
        }

        private static RepairImpactDescription Convert(WinFabricRepairImpactDescription winFabricRepairImpactDescription)
        {
            // Impact can be null, so we just return null here
            if (winFabricRepairImpactDescription == null) return null;

            switch (winFabricRepairImpactDescription.Kind)
            {
                case WinFabricRepairImpactKind.Node:
                    var result = new NodeRepairImpactDescription();
                    var impactedNodes = (winFabricRepairImpactDescription as WinFabricNodeRepairImpactDescription).ImpactedNodes;
                    if (impactedNodes != null)
                    {
                        foreach (var winFabricNodeImpact in impactedNodes)
                        {
                            result.ImpactedNodes.Add(Convert(winFabricNodeImpact));
                        }
                    }
                    return result;
                case WinFabricRepairImpactKind.Invalid:
                default:
                    throw new ArgumentOutOfRangeException("Invalid WinFabricRepairImpactKind value " + winFabricRepairImpactDescription.Kind, "winFabricRepairImpactDescription.Kind");
            }
        }

        private static WinFabricRepairImpactDescription Convert(RepairImpactDescription repairImpactDescription)
        {
            // Impact can be null, so we just return null here
            if (repairImpactDescription == null) return null;

            switch (repairImpactDescription.Kind)
            {
                case RepairImpactKind.Invalid:
                    return new WinFabricRepairImpactDescription()
                    {
                        Kind = WinFabricRepairImpactKind.Invalid
                    };
                case RepairImpactKind.Node:
                    var impactedNodes = (repairImpactDescription as NodeRepairImpactDescription).ImpactedNodes;
                    IList<WinFabricNodeImpact> winFabricImpactedNodes = null;
                    if (impactedNodes != null)
                    {
                        winFabricImpactedNodes = new List<WinFabricNodeImpact>();
                        foreach (var nodeImpact in impactedNodes)
                        {
                            winFabricImpactedNodes.Add(Convert(nodeImpact));
                        }
                    }

                    return new WinFabricNodeRepairImpactDescription()
                    {
                        Kind = WinFabricRepairImpactKind.Node,
                        ImpactedNodes = winFabricImpactedNodes
                    };
                default:
                    throw new ArgumentOutOfRangeException("Invalid RepairImpactKind value " + repairImpactDescription.Kind, "repairImpactDescription.Kind");
            }
        }

        private static NodeImpact Convert(WinFabricNodeImpact winFabricNodeImpact)
        {
            ThrowIf.Null(winFabricNodeImpact, "winFabricNodeImpact");
            return new NodeImpact()
                {
                    NodeName = winFabricNodeImpact.NodeName,
                    ImpactLevel = Convert(winFabricNodeImpact.ImpactLevel)
                };
        }

        private static WinFabricNodeImpact Convert(NodeImpact nodeImpact)
        {
            ThrowIf.Null(nodeImpact, "nodeImpact");
            return new WinFabricNodeImpact()
                {
                    NodeName = nodeImpact.NodeName,
                    ImpactLevel = Convert(nodeImpact.ImpactLevel)
                };
        }

        private static WinFabricNodeImpactLevel Convert(NodeImpactLevel nodeImpactLevel)
        {
            if (!Enum.IsDefined(typeof(NodeImpactLevel), nodeImpactLevel))
            {
                throw new ArgumentException("Invalid NodeImpactLevel value " + nodeImpactLevel, "nodeImpactLevel");
            }

            return (WinFabricNodeImpactLevel)nodeImpactLevel;
        }

        private static NodeImpactLevel Convert(WinFabricNodeImpactLevel winFabricNodeImpactLevel)
        {
            if (!Enum.IsDefined(typeof(WinFabricNodeImpactLevel), winFabricNodeImpactLevel))
            {
                throw new ArgumentException("Invalid WinFabricNodeImpactLevel value " + winFabricNodeImpactLevel, "winFabricNodeImpactLevel");
            }

            return (NodeImpactLevel)winFabricNodeImpactLevel;
        }

        private static RepairTargetDescription Convert(WinFabricRepairTargetDescription winFabricRepairTargetDescription)
        {
            if (winFabricRepairTargetDescription == null) return null;

            switch (winFabricRepairTargetDescription.Kind)
            {
                case WinFabricRepairTargetKind.Node:
                    var winFabricNodeRepairTargetDescription = winFabricRepairTargetDescription as WinFabricNodeRepairTargetDescription;
                    return new NodeRepairTargetDescription(winFabricNodeRepairTargetDescription.Nodes);
                case WinFabricRepairTargetKind.Invalid:
                default:
                    throw new ArgumentOutOfRangeException("Invalid WinFabricRepairTargetKind value " + winFabricRepairTargetDescription.Kind, "winFabricRepairTargetDescription");
            }
        }

        private static WinFabricRepairTargetDescription Convert(RepairTargetDescription repairTargetDescription)
        {
            if (repairTargetDescription == null) return null;

            switch (repairTargetDescription.Kind)
            {
                case RepairTargetKind.Invalid:
                    return new WinFabricRepairTargetDescription()
                        {
                            Kind = WinFabricRepairTargetKind.Invalid
                        };
                case RepairTargetKind.Node:
                    return new WinFabricNodeRepairTargetDescription()
                        {
                            Kind = WinFabricRepairTargetKind.Node,
                            Nodes = (repairTargetDescription as NodeRepairTargetDescription).Nodes
                        };
                default:
                    throw new ArgumentOutOfRangeException("Invalid RepairTargetKind value " + repairTargetDescription.Kind, "repairTargetDescription");
            }
        }

        private static RepairTaskResult Convert(WinFabricRepairTaskResult winFabricRepairTaskResult)
        {
            if (!Enum.IsDefined(typeof(WinFabricRepairTaskResult), winFabricRepairTaskResult))
            {
                throw new ArgumentException("Invalid WinFabricRepairTaskResult value " + winFabricRepairTaskResult, "winFabricRepairTaskResult");
            }

            return (RepairTaskResult)winFabricRepairTaskResult;
        }

        private static WinFabricRepairTaskResult Convert(RepairTaskResult repairTaskResult)
        {
            if (!Enum.IsDefined(typeof (RepairTaskResult), repairTaskResult))
            {
                throw new ArgumentOutOfRangeException("Invalid RepairTaskResult value " + repairTaskResult, "repairTaskResult");
            }

            return (WinFabricRepairTaskResult) repairTaskResult;
        }

        private static RepairTaskFlags Convert(WinFabricRepairTaskFlags winFabricRepairTaskFlags)
        {
            if (winFabricRepairTaskFlags != (winFabricRepairTaskFlags & WinFabricRepairTaskFlags.ValidMask))
            {
                throw new ArgumentOutOfRangeException("Invalid WinFabricRepairTaskFlags value " + winFabricRepairTaskFlags, "winFabricRepairTaskFlags");
            }

            return (RepairTaskFlags) winFabricRepairTaskFlags;
        }

        private static WinFabricRepairTaskFlags Convert(RepairTaskFlags repairTaskFlags)
        {
            if (repairTaskFlags != (repairTaskFlags & RepairTaskFlags.ValidMask))
            {
                throw new ArgumentOutOfRangeException("Invalid RepairTaskFlags value " + repairTaskFlags, "repairTaskFlags");
            }

            return (WinFabricRepairTaskFlags) repairTaskFlags;
        }

        private static WinFabricRepairTaskState Convert(RepairTaskState repairTaskState)
        {
            if (!Enum.IsDefined(typeof(RepairTaskState), repairTaskState))
            {
                throw new ArgumentOutOfRangeException("Invalid RepairTaskState value " + repairTaskState, "repairTaskState");
            }

            return (WinFabricRepairTaskState)repairTaskState;
        }

        public static bool TryGetErrorCode(Exception exception, out NativeTypes.FABRIC_ERROR_CODE errorCode)
        {
            ReleaseAssert.AssertIf(exception == null, "Exception cannot be null in TryGetErrorCode");
            errorCode = NativeTypes.FABRIC_ERROR_CODE.E_FAIL;

            COMException comException = exception.InnerException as COMException;
            if (comException == null)
            {
                TestabilityTrace.TraceSource.WriteWarning(
                    TraceSource,
                    "Client operation failed with exception which does not have inner com exception to extract error code for handling: {0}",
                    exception);
                return false;
            }

            errorCode = (NativeTypes.FABRIC_ERROR_CODE)comException.ErrorCode;
            ReleaseAssert.AssertIf(errorCode == 0, "HRESULT cannot be 0 for exception: {0}", exception);

            return true;
        }

        public static string GetStringDescription(PutPropertyOperation operation)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Property={0}; OpType={1}; PropertyType={2}; PropertyValue={3}",
                operation.PropertyName,
                operation.Kind,
                operation.PropertyType,
                operation.PropertyValue);
        }

        public static string GetStringDescription(PutCustomPropertyOperation operation)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Property={0}; OpType={1}; PropertyType={2}; PropertyValue={3}; CustomTypeId={4}",
                operation.PropertyName,
                operation.Kind,
                operation.PropertyType,
                operation.PropertyValue,
                operation.CustomTypeId);
        }

        public static string GetStringDescription(CheckValuePropertyOperation operation)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Property={0}; OpType={1}; PropertyType={2}; PropertyValue={3}",
                operation.PropertyName,
                operation.Kind,
                operation.PropertyType,
                operation.PropertyValue);
        }

        public static string GetStringDescription(CheckExistsPropertyOperation operation)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Property={0}; OpType={1}; ExistenceCheck={2}",
                operation.PropertyName,
                operation.Kind,
                operation.ExistenceCheck);
        }

        public static string GetStringDescription(CheckSequencePropertyOperation operation)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Property={0}; OpType={1}; SequenceNumber={2}",
                operation.PropertyName,
                operation.Kind,
                operation.SequenceNumber);
        }

        public static string GetStringDescription(DeletePropertyOperation operation)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Property={0}; OpType={1}",
                operation.PropertyName,
                operation.Kind);
        }

        public static string GetStringDescription(GetPropertyOperation operation)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Property={0}; OpType={1}; IncludeValue={2}",
                operation.PropertyName,
                operation.Kind,
                operation.IncludeValue);
        }

        public static string GetStringDescription(PropertyBatchOperation operation)
        {
            switch (operation.Kind)
            {
                case PropertyBatchOperationKind.CheckExists:
                    return GetStringDescription((CheckExistsPropertyOperation)operation);
                case PropertyBatchOperationKind.CheckSequence:
                    return GetStringDescription((CheckSequencePropertyOperation)operation);
                case PropertyBatchOperationKind.CheckValue:
                    return GetStringDescription((CheckValuePropertyOperation)operation);
                case PropertyBatchOperationKind.Delete:
                    return GetStringDescription((DeletePropertyOperation)operation);
                case PropertyBatchOperationKind.Get:
                    return GetStringDescription((GetPropertyOperation)operation);
                case PropertyBatchOperationKind.Put:
                    return GetStringDescription((PutPropertyOperation)operation);
                case PropertyBatchOperationKind.PutCustom:
                    return GetStringDescription((PutCustomPropertyOperation)operation);
                default:
                    return string.Format(
                        CultureInfo.InvariantCulture,
                        "Property={0}; OpType={1}",
                        operation.PropertyName,
                        operation.Kind);
            }
        }

        public static string GetStringDescription(Uri parentName, ICollection<PropertyBatchOperation> operations)
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendFormat(CultureInfo.InvariantCulture, "ParentName='{0}', operations=(", parentName);
            if (operations.Count > 0)
            {
                sb.AppendLine();
                foreach (PropertyBatchOperation operation in operations)
                {
                    sb.AppendLine(GetStringDescription(operation));
                }
            }

            sb.Append(")");
            return sb.ToString();
        }
    }
}


#pragma warning restore 1591