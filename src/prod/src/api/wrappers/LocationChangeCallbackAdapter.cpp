// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

LocationChangeCallbackAdapter::LocationChangeCallbackAdapter(
    ComFabricClient &owner,
    NamingUri &&name,
    Naming::PartitionKey &&partitionKey,
    ComPointer<IFabricServicePartitionResolutionChangeHandler> &&comCallback)
    : handle_(-1)
    , lock_()
    , name_(move(name))
    , key_(partitionKey)
    , comSourceWPtr_(ReferencePointer<ITentativeFabricServiceClient>(&owner))
    , isCancelled_(false)
    , comCallback_(move(comCallback))
    , owner_(owner)
{
}

ErrorCode LocationChangeCallbackAdapter::Register(LONGLONG *handle)
{
    auto thisSPtr = this->shared_from_this();

    auto error = owner_.ServiceMgmtClient->RegisterServicePartitionResolutionChangeHandler(
        name_,
        key_,
        [thisSPtr](LONGLONG handlerId, IResolvedServicePartitionResultPtr const& result, Common::ErrorCode error)
        {
            thisSPtr->OnServiceLocationChanged(handlerId, result, error);
        },
        &handle_);

    *handle = handle_;
    return error;
}

ErrorCode LocationChangeCallbackAdapter::Unregister()
{
    return owner_.ServiceMgmtClient->UnRegisterServicePartitionResolutionChangeHandler(handle_);
}

void LocationChangeCallbackAdapter::Cancel()
{
    {
        AcquireWriteLock lock(lock_);
        isCancelled_ = true;
    }
    Unregister();
}

void LocationChangeCallbackAdapter::OnServiceLocationChanged(LONGLONG id, IResolvedServicePartitionResultPtr result, Common::ErrorCode error)
{
    //
    // owner_ member cannot be accessed here directly, since this method is called when the location
    // change event gets fired. We need to ensure that the owner is not getting destructed concurrently
    // before issueing the callback to the user.
    //

    ComPointer<IFabricServiceManagementClient> comSource;
    {
        AcquireWriteLock lock(lock_);
        if (isCancelled_ || comSourceWPtr_->TryAddRef() == 0)
        {
            return;
        }
        comSource.SetNoAddRef(comSourceWPtr_.GetRawPointer());
    }

    if (error.IsSuccess())
    {
        auto comResult = make_com<ComResolvedServicePartitionResult, IFabricResolvedServicePartitionResult>(result);
        comCallback_->OnChange(
            comSourceWPtr_.GetRawPointer(),
            id,
            comResult.GetRawPointer(),
            error.ToHResult());
    }
    else
    {
        comCallback_->OnChange(
            comSourceWPtr_.GetRawPointer(),
            id,
            NULL,
            error.ToHResult());
    }
} 
