// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Runtime.Serialization;

    [Serializable]
    [DataContract]
    public class WinFabricRepairTask
    {
        [DataMember(Name = "TaskId", IsRequired = true)]
        public string TaskId { get; set; }

        [DataMember(Name = "Version", IsRequired = true)]
        public Int64 Version { get; set; }

        [DataMember(Name = "Description", IsRequired = true)]
        public string Description { get; set; }

        [DataMember(Name = "State", IsRequired = true)]
        public WinFabricRepairTaskState State { get; set; }

        [DataMember(Name = "Flags", IsRequired = true)]
        public WinFabricRepairTaskFlags Flags { get; set; }

        [DataMember(Name = "Action", IsRequired = true)]
        public string Action { get; set; }

        [DataMember(Name = "Target", IsRequired = true)]
        public WinFabricRepairTargetDescription Target { get; set; }

        [DataMember(Name = "Executor", IsRequired = true)]
        public string Executor { get; set; }

        [DataMember(Name = "ExecutorData", IsRequired = true)]
        public string ExecutorData { get; set; }

        [DataMember(Name = "Impact", IsRequired = true)]
        public WinFabricRepairImpactDescription Impact { get; set; }

        [DataMember(Name = "ResultStatus", IsRequired = true)]
        public WinFabricRepairTaskResult ResultStatus { get; set; }

        [DataMember(Name = "ResultCode", IsRequired = true)]
        public int ResultCode { get; set; }

        [DataMember(Name = "ResultDetails", IsRequired = true)]
        public string ResultDetails { get; set; }

        [DataMember(Name = "CreatedTimestamp", IsRequired = true)]
        public DateTime? CreatedTimestamp { get; set; }

        [DataMember(Name = "ClaimedTimestamp", IsRequired = true)]
        public DateTime? ClaimedTimestamp { get; set; }

        [DataMember(Name = "PreparingTimestamp ", IsRequired = true)]
        public DateTime? PreparingTimestamp { get; set; }

        [DataMember(Name = "ApprovedTimestamp", IsRequired = true)]
        public DateTime? ApprovedTimestamp { get; set; }

        [DataMember(Name = "ExecutingTimestamp ", IsRequired = true)]
        public DateTime? ExecutingTimestamp { get; set; }

        [DataMember(Name = "RestoringTimestamp", IsRequired = true)]
        public DateTime? RestoringTimestamp { get; set; }

        [DataMember(Name = "CompletedTimestamp", IsRequired = true)]
        public DateTime? CompletedTimestamp { get; set; }
    }
}

#pragma warning restore 1591