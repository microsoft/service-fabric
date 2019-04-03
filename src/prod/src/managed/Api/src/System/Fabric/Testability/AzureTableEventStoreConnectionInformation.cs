// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    public sealed class AzureTableEventStoreConnectionInformation : AzureEventStoreConnectionInformation
    {
        public AzureTableEventStoreConnectionInformation(string accountName, string accountKey, string tableNamePrefix)
            : this(accountName, accountKey, tableNamePrefix, null)
        {
        }

        public AzureTableEventStoreConnectionInformation(string accountName, string accountKey, string tableNamePrefix, string deploymentId)
            : base(tableNamePrefix, deploymentId)
        {
            this.AccountName = accountName;
            this.AccountKey = accountKey;
        }

        public string AccountName { get; private set; }

        public string AccountKey { get; private set; }
    }
}

#pragma warning restore 1591