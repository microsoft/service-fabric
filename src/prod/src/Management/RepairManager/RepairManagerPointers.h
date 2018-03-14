// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {  
        class RepairManagerServiceReplica;
        typedef std::shared_ptr<RepairManagerServiceReplica> RepairManagerServiceReplicaSPtr;

        class RepairManagerServiceFactory;
        typedef Common::RootedObjectHolder<RepairManagerServiceFactory> RepairManagerServiceFactoryHolder;
    }
}
