// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System;

    public class ApiErrorInformation
    {
        public ApiErrorInformation(string partitionId, DateTime failureTime, string operation, string error) 
        {
            this.PartitionId = partitionId;
            this.FailureTime = failureTime;
            this.Operation = operation;
            this.Error = error;
        }

        public string PartitionId { get; internal set; }

        public DateTime FailureTime { get; internal set; }

        public string Operation { get; internal set; }

        public string Error { get; internal set; }

        public override string ToString()
        {
            return string.Format(
                "Partition Id : {0}, FailureTime : {1}, Operation : {2}, Error: {3}", 
                this.PartitionId, 
                this.FailureTime, 
                this.Operation, 
                this.Error);
        }
    }
}


#pragma warning restore 1591