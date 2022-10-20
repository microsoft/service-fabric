// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    public abstract class AzureEventStoreConnectionInformation : EventStoreConnectionInformation
    {
        protected AzureEventStoreConnectionInformation(string tableNamePrefix, string deploymentId)
            : base(tableNamePrefix)
        {
            this.DeploymentId = deploymentId;
        }

        public string DeploymentId { get; private set; }
    }
}

#pragma warning restore 1591