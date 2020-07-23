// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;

    internal interface IServiceCatalog
    {
        object FindService(string name, IComparable partitionId);

        void AddService(string name, IComparable partitionId, object service);

        bool RemoveService(string name, IComparable partitionId);
    }
}