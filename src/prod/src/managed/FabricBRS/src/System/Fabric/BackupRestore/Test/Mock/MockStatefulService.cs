// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.ServiceFabric.Data;
using Microsoft.ServiceFabric.Services.Runtime;

namespace System.Fabric.BackupRestore.Test.Mock
{

    internal class MockStatefulService : StatefulService
    {
        public MockStatefulService(StatefulServiceContext serviceContext) : base(serviceContext)
        {
        }

        public MockStatefulService(StatefulServiceContext serviceContext, IReliableStateManagerReplica reliableStateManagerReplica) : base(serviceContext, reliableStateManagerReplica)
        {
        }
    }
}