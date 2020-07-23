// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    public sealed class LocalEventStoreConnectionInformation : EventStoreConnectionInformation
    {
        public LocalEventStoreConnectionInformation()
            : base(null)
        {
        }
    }
}

#pragma warning restore 1591