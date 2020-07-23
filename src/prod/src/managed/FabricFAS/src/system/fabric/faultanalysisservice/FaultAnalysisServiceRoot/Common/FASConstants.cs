// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Common
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common.Exceptions;
    using System.Fabric.Interop;
    using System.Threading.Tasks;

    internal class FASConstants
    {
        public const string TestabilityFabricClientName = "FaultAnalysisServiceFabricClient";
        public const string ActionReliableDictionaryName = "ActionTable";
        public const string HistoryReliableDictionaryName = "HistoryTable";
        public const string FaultAnalysisServiceConfigSectionName = "FaultAnalysisService";
        public const string StoppedNodeTable = "StoppedNodeTable";

        public const string UnavailableMessage = "Unavailable until action is in either Completed or Faulted state";

        public const string RequestTimeoutInSecondsName = "RequestTimeoutInSeconds";
        public const string OperationTimeoutInSecondsName = "OperationTimeoutInSeconds";

        public const int DefaultRequestTimeoutInSeconds = 60;
        public const int DefaultOperationTimeoutInSeconds = 300;

        public const string MaxStoredActionCountName = "MaxStoredActionCount";
        public const string StoredActionCleanupIntervalInSecondsName = "StoredActionCleanupIntervalInSeconds";
        public const string CompletedActionKeepDurationInSecondsName = "CompletedActionKeepDurationInSeconds";

        public const long DefaultMaxStoredActionCount = 10000;
        public const int DefaultStoredActionCleanupIntervalInSeconds = 3600;
        public const int DefaultCompletedActionKeepDurationInSeconds = 604800;

        public const string CommandStepRetryBackoffInSecondsName = "CommandStepRetryBackoffInSeconds";
        public const int DefaultCommandStepRetryBackoffInSeconds = 1;

        public const string ConcurrentRequestsName = "ConcurrentRequests";
        public const int DefaultConcurrentRequests = 100;

        public const string DataLossCheckWaitDurationInSecondsName = "DataLossCheckWaitDurationInSeconds";
        public const int DefaultDataLossCheckWaitDurationInSeconds = 25;

        public const string DataLossCheckPollIntervalInSecondsName = "DataLossCheckPollIntervalInSeconds";
        public const int DefaultDataLossCheckPollIntervalInSeconds = 5;

        public const string ReplicaDropWaitDurationInSecondsName = "ReplicaDropWaitDurationInSeconds";
        public const int DefaultReplicaDropWaitDurationInSeconds = 600;

        // These are used in the script
        public const long TestMaxStoredActionCountValue = 2;
        public const int TestStoredActionCleanupIntervalInSeconds = 5;
        public const int CompletedActionKeepDurationInSeconds = 10;

        // User cancellation check locations
        public const string OuterLoop = "Outer";
        public const string ForwardLoop = "Forward";
        public const string ForwardLoopExceptionBlock = "ForwardLoopExceptionBlock";
        public const string OuterCleanupLoop = "OuterCleanup";
        public const string InnerCleanupLoop = "InnerCleanup";
        public const string InnerForwardLoop = "InnerForwardLoop";

        public const int QuorumLossCheckRetryCount = 10;

        // API Test Modes
        public const int ChaosApiTestMode = 1;

        /// <summary>
        /// This is the name of reliable dictionary that holds the state of the scheduler and the schedule
        /// </summary>
        public const string ChaosSchedulerStateName = "ChaosSchedulerStateName";

        /// <summary>
        /// This is the name of the single-row reliable dictionary that holds the ChaosParameters object.
        /// </summary>
        public const string ChaosParametersTableName = "ChaosParametersTable";

        /// <summary>
        /// This is the key to the single entry in the reliable dictonary that holds the ChaosParameters object.
        /// </summary>
        public const string ChaosParametersKey = "ChaosParametersKey";

        /// <summary>
        /// This is the name of the single-row reliable dictionary that holds the timestamp of the started event;
        /// a distinct table is needed because whenever the FAS primary fails over, it may need to adjust time to run
        /// for Chaos.
        /// </summary>
        public const string StartTimeTableName = "StartTimeTable";

        /// <summary>
        /// This is the name of the property in the ChaosSchedulerStatusDictionary that contains the state of the scheduler.
        /// </summary>
        public const string ChaosSchedulerStatusDictionaryStateKey = "State";

        /// <summary>
        /// This is the name of the property in the ChaosSchedulerStatusDictionary that contains the schedule of the scheduler.
        /// </summary>
        public const string ChaosSchedulerStatusDictionaryScheduleKey = "Schedule";

        /// <summary>
        /// This is the key to the single entry in the reliable dictionary that holds the timestamp of the started event.
        /// </summary>
        public const string StartTimeKey = "StartTimeKey";

        public const string NextItearationTimeStampRDName = "NextItearationTimeStampRD";
        public const string NextItearationTimeStampKey = "NextItearationTimeStampKey";

        /// <summary>
        /// This is the maximum number of Chaos events that will be returned as the result of any one
        /// invocation of the GetChaosStatus API; if there are more events to return, the continuation token
        /// in the ChaosStatus object will be non-null.
        /// </summary>
        public const int ChaosEventBatchLength = 100;

        public const int ChaosEventBatchLengthTest = 2;

        /// <summary>
        /// This is the name of the reliable dictionary that holds the Chaos events
        /// </summary>
        public const string ChaosEventsDictionaryName = "ChaosEventsDictionary";

        /// <summary>
        /// This is the name of the single row reliable dictionary that holds the current task that Chaos is performing
        /// </summary>
        public const string ChaosStatusDictionaryName = "ChaosStatusDictionary";

        /// <summary>
        /// This is the key to the single entry of the ChaosTask reliable dicionary
        /// </summary>
        public const string ChaosStatusKeyName = "ChaosStatusKey";

        /// <summary>
        /// Last event is used to decide between state transitions; this reliable dictioanry has a single entry
        /// for the last event key, so that whenever the last event is required, we could fetch it from the
        /// events dictionary.
        /// </summary>
        public const string ChaosLastEventKeyDictionaryName = "ChaosLastEventKeyDictionary";

        /// <summary>
        /// This is the key to the single entry in the ChaosLastEventKeyDictionary
        /// </summary>
        public const string ChaosLastEventKeyName = "ChaosLastEventKey";

        public const string ChaosReportFilterDictionaryName = "ChaosReportFilterDictionary";

        public const int DefaultTelemetryReportPeriodInSeconds = 3600;
        public const int TestTelemetryReportPeriodInSeconds = 60;

        public const string ChaosTelemetrySamplingProbabilityConfigKeyName = "ChaosTelemetrySamplingProbability";
        public const string ChaostelemetryReportPeriodInSecondsConfigKeyName = "ChaostelemetryReportPeriodInSeconds";

        public const double DefaultChaosTelemetrySamplingProbability = 0.05;

        /// <summary>
        /// This is to limit the size of the EventsDictionary; assuming 4 KB
        /// as the max size of a single event, we are limiting the size to 1 GB
        /// </summary>
        public const int MaxNumberOfEventsInDictionary = 8 * 1024;

        public const int TestMaxNumberOfEventsInDictionary = 5;

        public const int MaxRetriesForReliableDictionary = 5;

        public const string MaxStoredChaosEventCountConfigName = "MaxStoredChaosEventCount";
        public const int MaxStoredChaosEventCountDefault = 25000;
        public const string StoredChaosEventCleanupIntervalInSecondsConfigName = "StoredChaosEventCleanupIntervalInSeconds";
        public const int StoredChaosEventCleanupIntervalInSecondsDefault = 3600;
        public const int TestTuncationPeriodInSeconds = 60;

        // The min allowed duration for stop node through StartNodeTransition().  Api calls that provide a value below this will fail.
        public const string StopNodeMinDurationInSecondsName = "StopNodeMinDurationInSeconds";
        public const int DefaultStopNodeMinDurationInSeconds = 5 * 60;

        // The max allowed duration for stop node through StartNodeTransition().  Api calls that provide a value above this will fail.
        public const string StopNodeMaxDurationInSecondsName = "StopNodeMaxDurationInSeconds";
        public const int DefaultStopNodeMaxDurationInSeconds = 60 * 60 * 4;
        public const string FaultStringDelimeter = ",";
        public const string ChaosTargetFilterPropertyName = "ChaosTargetFilter";

        public static readonly Guid FmPartitionId = new Guid("00000000-0000-0000-0000-000000000001");

        public static readonly TimeSpan DefaultDictionaryTimeout = TimeSpan.FromSeconds(4.0d);

        public static readonly TimeSpan ReliableDictionaryTimeout = TimeSpan.FromSeconds(4.0);
        public static readonly TimeSpan TestOperationTimeout = TimeSpan.FromSeconds(30);

        public static readonly HashSet<FabricErrorCode> ExpectedErrorCodes = new HashSet<FabricErrorCode>()
        {
            FabricErrorCode.ReplicaDoesNotExist,
            FabricErrorCode.InvalidReplicaStateForReplicaOperation,
            FabricErrorCode.CodePackageNotFound
        };

        public static readonly HashSet<NativeTypes.FABRIC_ERROR_CODE> ExpectedErrorCodesInternal = new HashSet<NativeTypes.FABRIC_ERROR_CODE>()
        {
            NativeTypes.FABRIC_ERROR_CODE.S_OK,
            NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_REPLICA_STATE,
            NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PRIMARY_ALREADY_EXISTS,
            NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPLICA_DOES_NOT_EXIST,
            NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_NOT_FOUND,
            NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CODE_PACKAGE_NOT_FOUND
        };

        public static readonly HashSet<Type> ExpectedExceptions = new HashSet<Type>()
        {
            typeof(TimeoutException),
            typeof(FabricTransientException),
            typeof(OperationCanceledException),
            typeof(TaskCanceledException),
            typeof(FabricValidationException),
            typeof(ChaosInconsistentClusterSnapshotException)
        };

        public static readonly TimeSpan DefaultTestTimeout = TimeSpan.FromSeconds(10);

        public static readonly TimeSpan DefaultChaosInternalRequestTimeout = TimeSpan.FromSeconds(2.0);
        public static readonly TimeSpan DefaultMaximumBackoffForChaosIterations = TimeSpan.FromHours(1.0);

        public static readonly string ChaosReportContinuationTokenFormat = "x{0}-{1}"; // xTimeStamp-GUID
    }
}