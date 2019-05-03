// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    public sealed class MdsTableEventStoreConnectionInformation : AzureEventStoreConnectionInformation
    {
        public MdsTableEventStoreConnectionInformation(string moniker, string instanceUrl, string tableNamePrefix)
            : this(moniker, instanceUrl, tableNamePrefix, null)
        {
        }

        public MdsTableEventStoreConnectionInformation(string moniker, string instanceUrl, string tableNamePrefix, string deploymentId)
            : base(tableNamePrefix, deploymentId)
        {
            this.Moniker = moniker;
            this.InstanceUrl = instanceUrl;
        }

        public string Moniker { get; private set; }

        public string InstanceUrl { get; private set; }
    }
}
#pragma warning restore 1591