// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.DataReader
{
    public enum EntityType
    {
        Cluster,

        Application,

        Service,

        Partition,

        Replica,

        Node,

        Correlation
    }
}
