// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    internal class StartCallDataBase
    {
        public StartCallDataBase(string operationid, int partitionSelectorType, string partitionKey)
        {
            this.OperationId = operationid;
            this.PartitionSelectorType = partitionSelectorType;
            this.PartitionKey = partitionKey;
        }

        public string OperationId
        {
            get;
            set;
        }

        public int PartitionSelectorType
        {
            get;
            set;
        }

        public string PartitionKey
        {
            get;
            set;
        }
    }
}

#pragma warning restore 1591