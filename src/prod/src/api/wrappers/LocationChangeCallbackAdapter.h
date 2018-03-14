// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class LocationChangeCallbackAdapter
        : public std::enable_shared_from_this<LocationChangeCallbackAdapter>
    {
        DENY_COPY(LocationChangeCallbackAdapter)
    
    public :
        LocationChangeCallbackAdapter(
            ComFabricClient &owner,
            Common::NamingUri &&name,
            Naming::PartitionKey &&key,
            Common::ComPointer<IFabricServicePartitionResolutionChangeHandler> &&comCallback);

        Common::ErrorCode Register(LONGLONG *handle);
        Common::ErrorCode Unregister();
        
        void Cancel();
        void OnServiceLocationChanged(LONGLONG id, IResolvedServicePartitionResultPtr result, Common::ErrorCode error);

    private:
        LONGLONG handle_;
        Common::ComPointer<IFabricServicePartitionResolutionChangeHandler> comCallback_;
        Common::ReferencePointer<ITentativeFabricServiceClient> comSourceWPtr_;
        bool isCancelled_;
        Common::RwLock lock_;
        Common::NamingUri name_;
        Naming::PartitionKey key_;
        ComFabricClient &owner_;
    };
}
