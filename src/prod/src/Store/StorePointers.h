// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{ 
    class ILocalStore;
    typedef std::unique_ptr<ILocalStore> ILocalStoreUPtr;
    typedef std::shared_ptr<ILocalStore> ILocalStoreSPtr;

    class IReplicatedStore;
    typedef std::unique_ptr<IReplicatedStore> IReplicatedStoreUPtr;

    class IStoreFactory;
    typedef std::shared_ptr<IStoreFactory> IStoreFactorySPtr;

    class ReplicatedStore;
    typedef std::unique_ptr<ReplicatedStore> ReplicatedStoreUPtr;

    class FabricTimeController;
    typedef std::shared_ptr<FabricTimeController> FabricTimeControllerSPtr;

    class EseDefragmenter;
    typedef std::shared_ptr<EseDefragmenter> EseDefragmenterSPtr;

    typedef std::function<void(bool shouldThrottle)> ThrottleCallback;
}
