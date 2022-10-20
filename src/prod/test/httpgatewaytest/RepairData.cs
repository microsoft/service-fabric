// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.HttpGatewayTest
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [Serializable]
    [DataContract]
    class RepairTaskScopeData
    {
        [DataMember]
        public Repair.RepairScopeIdentifierKind Kind { get; set; }
    }

    [Serializable]
    [DataContract]
    class RepairTaskTargetData
    {
        [DataMember]
        public Repair.RepairTargetKind Kind { get; set; }

        [DataMember]
        public IList<string> NodeNames { get; set; }
    }

    [Serializable]
    [DataContract]
    class NodeImpactData
    {
        [DataMember]
        public string NodeName { get; set; }

        [DataMember]
        public Repair.NodeImpactLevel ImpactLevel { get; set; }
    }

    [Serializable]
    [DataContract]
    class RepairTaskImpactData
    {
        [DataMember]
        public Repair.RepairImpactKind Kind { get; set; }

        [DataMember]
        public IList<NodeImpactData> NodeImpactList { get; set; }
    }

    [Serializable]
    [DataContract]
    class RepairTaskHistoryData
    {
        [DataMember]
        public DateTime CreatedUtcTimestamp { get; set; }

        [DataMember]
        public DateTime ClaimedUtcTimestamp { get; set; }

        [DataMember]
        public DateTime PreparingUtcTimestamp { get; set; }

        [DataMember]
        public DateTime ApprovedUtcTimestamp { get; set; }

        [DataMember]
        public DateTime ExecutingUtcTimestamp { get; set; }

        [DataMember]
        public DateTime RestoringUtcTimestamp { get; set; }

        [DataMember]
        public DateTime CompletedUtcTimestamp { get; set; }
    }

    [Serializable]
    [DataContract]
    class RepairTaskData
    {
        [DataMember(EmitDefaultValue = false)]
        public RepairTaskScopeData Scope { get; set; }

        [DataMember]
        public string TaskId { get; set; }

        // ISSUE WinFab JSON deserializer expects 64-bit integers to be serialized as strings
        [DataMember(EmitDefaultValue = false)]
        public string Version { get; set; }

        [DataMember(EmitDefaultValue = false)]
        public string Description { get; set; }

        [DataMember]
        public Repair.RepairTaskState State { get; set; }

        [DataMember]
        public Repair.RepairTaskFlags Flags { get; set; }

        [DataMember(EmitDefaultValue = false)]
        public string Action { get; set; }

        [DataMember(EmitDefaultValue = false)]
        public RepairTaskTargetData Target { get; set; }

        [DataMember(EmitDefaultValue = false)]
        public string Executor { get; set; }

        [DataMember(EmitDefaultValue = false)]
        public string ExecutorData { get; set; }

        [DataMember(EmitDefaultValue = false)]
        public RepairTaskImpactData Impact { get; set; }

        [DataMember]
        public Repair.RepairTaskResult ResultStatus { get; set; }

        [DataMember]
        public int ResultCode { get; set; }

        [DataMember(EmitDefaultValue = false)]
        public string ResultDetails { get; set; }

        [DataMember]
        public bool PerformPreparingHealthCheck { get; set; }

        [DataMember]
        public bool PerformRestoringHealthCheck { get; set; }
    }

    [Serializable]
    [DataContract]
    class RepairIdentifierData
    {
        [DataMember]
        public string TaskId { get; set; }

        // ISSUE WinFab JSON deserializer expects 64-bit integers to be serialized as strings
        [DataMember(EmitDefaultValue = false)]
        public string Version { get; set; }
    }

    [Serializable]
    [DataContract]
    class RepairTaskHealthPolicyData
    {
        [DataMember]
        public string TaskId { get; set; }

        // ISSUE WinFab JSON deserializer expects 64-bit integers to be serialized as strings
        [DataMember(EmitDefaultValue = false)]
        public string Version { get; set; }

        /// <remarks>
        /// EmitDefaultValue is false since we don't want the Json body to contain this property at all if
        /// the prepare health check value shouldn't be updated from whatever is set currently.
        /// </remarks>
        [DataMember(EmitDefaultValue = false)]
        public bool? PerformPreparingHealthCheck { get; set; }

        /// <remarks>
        /// <see cref="PerformPreparingHealthCheck"/> for reason for EmitDefaultValue.
        /// </remarks>>
        [DataMember(EmitDefaultValue = false)]
        public bool? PerformRestoringHealthCheck { get; set; }
    }
    
}