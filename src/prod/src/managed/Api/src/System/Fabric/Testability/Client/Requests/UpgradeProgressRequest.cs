// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;

    public abstract class UpgradeProgressRequest<T> : FabricRequest
    {
        protected UpgradeProgressRequest(IFabricClient fabricClient, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
        }

        public T PreviousProgress
        {
            get;
            set;
        }
    }
}


#pragma warning restore 1591