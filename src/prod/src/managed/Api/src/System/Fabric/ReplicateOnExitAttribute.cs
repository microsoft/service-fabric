// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Transactions;

    [AttributeUsage(AttributeTargets.Method)]
    public sealed class ReplicateOnExitAttribute : Attribute
    {
        public ReplicateOnExitAttribute()
        {
            this.Async = false;
        }

        public bool Async
        {
            get;
            set;
        }
    }
}