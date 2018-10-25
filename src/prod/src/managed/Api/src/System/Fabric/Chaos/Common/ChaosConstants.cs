// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.Common
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.RandomActionGenerator;
    using System.Fabric.Common;
    using System.Fabric.Health;

    internal static class ChaosConstants
    {
        public const string PreAssertAmble = "Pre-assert snapshot...";
        public const string PostAssertAmble = "Post-assert snapshot...";

        public const string DummyFMCodePackageVersion = "-1.0.0.0";
        public const string DummyCMCodePackageVersion = "0.-1.0.0";
        public const string DummyNSCodePackageVersion = "0.0.-1.0";

        public const string NodeNameUsingIdPrefix = "nodeid:";
        public const int StringLengthLimit = 3 * 1024;
        public const int MaxNumberOfStartChaosContextEntries = 100;

        public const string KeyForDummyContextEntry = "87D880B3-4F7A-492C-AD13-C3D61467118B";
        public const string ValueForDummyContextEntry = "C60A8B28-1BA9-406B-807E-616308F3A83A";

        /// <summary>
        /// These are the special keys that get passed through the context dictionary of ChaosParameters
        /// in StartChaosAsync; this is internal and for test purpose only.
        /// </summary>
        ///
        public const string ServiceFabricChaosInternalContextKeyPrefix = "ServiceFabric_Chaos_Internal_Context_Key_";
        public const string EnableForcedAssertKey = ServiceFabricChaosInternalContextKeyPrefix+"2B5321B6-B76A-4AC0-A9BB-D7B3661ABCEE";
        public const string EnableForceMoveOfReplicaKey = ServiceFabricChaosInternalContextKeyPrefix+ "Enable_Force_Move_Of_Replica";
        public const string ChaosShouldFaultSystemKey = ServiceFabricChaosInternalContextKeyPrefix + "Should_Fault_System_Services";
        public static readonly string MaxNumberOfRetriesInGetClusterSnapshotActionKey = ServiceFabricChaosInternalContextKeyPrefix+"MaxNumberOfRetriesInGetClusterSnapshotAction";
        public static readonly string FailoverAfterChaosFaultsKey = ServiceFabricChaosInternalContextKeyPrefix + "FailoverAfterChaosFaults";
        public static readonly string DisableInputValidationForChaosTargetFilterKey = ServiceFabricChaosInternalContextKeyPrefix + "DisableInputValidationForChaosTargetFilter";
        public const int DefaultMaxNumberOfRetriesInGetClusterSnapshotAction = 15;

        public const string ManagedClientTypeName = "managed";
        public const string RestClientTypeName = "rest";
        public const string NativeClientTypeName = "native";

        public static readonly HashSet<string> ReservedKeys = new HashSet<string>()
                                                                {
                                                                    EnableForcedAssertKey,
                                                                    KeyForDummyContextEntry
                                                                };

        public static readonly HashSet<string> UpgradingInfrastructureJobStatus = new HashSet<string>(StringComparer.InvariantCultureIgnoreCase)
                                                                                    {
                                                                                        "Alerted",
                                                                                        "Executing",
                                                                                        "Pending"
                                                                                    };

        public const string InfrastructureServiceType = "InfrastructureServiceType";
        public const string InfrastructureServiceGetCurrentStateCommand = "GetCurrentState";
        public const string InfrastructureServiceGetJobsCommand = "GetJobs";

#region ChaosEngineError Telemetry IDs
        public const string StartNode_InvalidActionType_TelemetryId = "StartNode_InvalidActionType";
        public const string StartNode_InvalidNodeName_TelemetryId = "StartNode_InvalidNodeName";
        public const string StopNode_InvalidActionType_TelemetryId = "StopNode_InvalidActionType";
        public const string StopNode_InvalidNodeName_TelemetryId = "StopNode_InvalidNodeName";
        public const string RestartNode_InvalidActionType_TelemetryId = "RestartNode_InvalidActionType";
        public const string RestartNode_InvalidNodeName_TelemetryId = "RestartNode_InvalidNodeName";

        public const string FaultCodePackage_CodepackageEntityInTransition_TelemetryId = "FaultCodePackage_CodepackageEntityInTransition";
        public const string FaultCodePackage_NullNode_TelemetryId = "FaultCodePackage_NullNode";
        public const string FaultCodePackage_FaultedNode_TelemetryId = "FaultCodePackage_FaultedNode";
        public const string FaultCodePackage_PartitionNotFaultTolerant_TelemetryId = "FaultCodePackage_PartitionNotFaultTolerant";

        public const string FaultNode_NodeInTransition_TelemetryId = "FaultNode_NodeInTransition";
        public const string FaultNode_UnsafeNode_TelemetryId = "FaultNode_UnsafeNode";

        public const string MoveReplica_Unavailable_TelemetryId = "MoveReplica_Unavailable";
        public const string MoveReplica_PartitionNotFaultTolerant_TelemetryId = "MoveReplica_PartitionNotFaultTolerant";
        public const string MoveReplica_InvalidNode_TelemetryId = "MoveReplica_InvalidNode";
        public const string FaultReplica_ReplicaInTransition_TelemetryId = "FaultReplica_ReplicaInTransition";
        public const string FaultReplica_PartitionNotFaultTolerant_TelemetryId = "FaultReplica_PartitionNotFaultTolerant";
        public const string MarkReplciaAsInTransition_UnavailableTargetNode_TelemetryId = "MarkReplciaAsInTransition_UnavailableTargetNode";
        public const string MarkReplicaAsInTransition_UnavailableSourceNode_TelemetryId = "MarkReplicaAsInTransition_UnavailableSourceNode";
        public const string ClearReplicaOfExclusionFlags_UnavailableSourceCodePackage_TelemetryId = "ClearReplicaOfExclusionFlags_UnavailableSourceCodePackage";
        public const string MarkReplicaAsInTransition_NodeFaulted_TelemetryId = "MarkReplicaAsInTransition_NodeFaulted";
        public const string MarkReplicaAsInTransition_CodepackageFaulted_TelemetryId = "MarkReplicaAsInTransition_CodepackageFaulted";

        public const string MarkAllUnsafeEntities_NodeNotFoundForCodePackage_TelemetryId = "MarkAllUnsafeEntities_NodeNotFoundForCodePackage";

        public const string GetClusterSnapshotAction_MaximumNumberOfRetriesAchieved_TelemetryId = "GetClusterSnapshotAction_MaximumNumberOfRetriesAchieved";

        public const string ClearCodePackageOfExclusionFlags_DeployedReplicaNotFound_TelemetryId = "ClearCodePackageOfExclusionFlags_DeployedReplicaNotFound";

        public const string UnexpectedNumberOfInternalChaosParametersCtor_TelemetryId = "UnexpectedNumberOfInternalChaosParametersCtor";

        public const string ChaosScheduler_StateResetOnRestart_TelemetryId = "ChaosScheduler_StateResetOnRestart";
#endregion ChaosEngineError Telemetry IDs

        public const double RequestTimeoutFactorDefault = 0.2;
        public const long MaxConcurrentFaultsDefault = 1;
        public static readonly TimeSpan RequestTimeoutDefault = TimeSpan.FromMinutes(1);
        public static readonly TimeSpan OperationTimeoutDefault = TimeSpan.FromMinutes(5);
        public static readonly TimeSpan DefaultClusterStabilizationTimeout = TimeSpan.FromSeconds(60);
        public static readonly TimeSpan WaitTimeBetweenIterationsDefault = TimeSpan.FromSeconds(30);
        public static readonly TimeSpan WaitTimeBetweenFaultsDefault = TimeSpan.FromSeconds(20);
        public static readonly ClusterHealthPolicy ClusterHealthPolicyDefault = new ClusterHealthPolicy
                                                                                    {
                                                                                        ConsiderWarningAsError = true
                                                                                    };

        public static readonly Dictionary<Type, HashSet<FabricErrorCode>> ExpectedExceptionTypeMap
            = new Dictionary<Type, HashSet<FabricErrorCode>>
                  {
                      {
                          typeof(MovePrimaryAction),
                          new HashSet<FabricErrorCode>
                              {
                                  FabricErrorCode.ConstraintNotSatisfied
                              }
                      },
                      {
                          typeof(MoveSecondaryAction),
                          new HashSet<FabricErrorCode>
                              {
                                  FabricErrorCode.ConstraintNotSatisfied
                              }
                      }
                  };

        #region Chaos Scheduler

        public const string ChaosScheduler_DefaultParameterKey = "adhoc";

        public const int SchedulerCycleWaitIntervalMilliseconds = 1000; // 1 second
        public const int SchedulerLockWaitMilliseconds = 20 * 1000;
        public static readonly DateTime FileTimeMinDateTime = DateTime.FromFileTimeUtc(0);

        public const decimal ApiVersion62 = 6.2M;
        public const decimal ApiVersionCurrent = 6.2M;

        public const string DateTimeFormat_ISO8601 = "yyyy'-'MM'-'dd'T'HH':'mm':'ss.fffK";
        #endregion
    }
}