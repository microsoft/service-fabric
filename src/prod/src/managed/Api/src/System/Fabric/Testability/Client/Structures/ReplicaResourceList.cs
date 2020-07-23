// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Testability.Client.Structures
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Query;

    public sealed class ReplicaResourceList : PagedList<ReplicaResource>
    {
        public ReplicaResourceList() : base()
        {
        }

        internal ReplicaResourceList(IList<ReplicaResource> list) : base(list)
        {
        }
    }
}

#pragma warning restore 1591