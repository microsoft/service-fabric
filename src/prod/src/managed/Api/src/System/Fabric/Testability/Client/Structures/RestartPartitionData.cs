// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Numerics;
    using System.Runtime.Serialization;

    internal class RestartPartitionData : StartCallDataBase
    {
        public RestartPartitionData(string operationid, int partitionSelectorType, string partitionKey, int mode)
            : base(operationid, partitionSelectorType, partitionKey)
        {
            this.RestartPartitionMode = mode;
        }

        public int RestartPartitionMode
        {
            get;
            set;
        }
    }
}

#pragma warning restore 1591