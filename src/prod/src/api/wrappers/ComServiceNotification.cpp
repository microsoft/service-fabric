// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
    
using namespace std;
using namespace Common;
using namespace Api;
using namespace Reliability;

ComServiceNotification::ComServiceNotification()
    : impl_()
    , heap_()
    , notification_()
    , lock_()
{
}

ComServiceNotification::ComServiceNotification(IServiceNotificationPtr const & impl)
    : impl_(impl)
    , heap_()
    , notification_()
    , lock_()
{
}

const FABRIC_SERVICE_NOTIFICATION * STDMETHODCALLTYPE ComServiceNotification::get_Notification()
{
    {
        AcquireReadLock lock(lock_);

        if (notification_.GetRawPointer())
        {
            return notification_.GetRawPointer();
        }
    }

    {
        AcquireWriteLock lock(lock_);

        if (!notification_.GetRawPointer())
        {
            notification_ = heap_.AddItem<FABRIC_SERVICE_NOTIFICATION>();

            impl_->GetNotification(heap_, *(notification_.GetRawPointer()));
        }

        return notification_.GetRawPointer();
    }
}

HRESULT STDMETHODCALLTYPE ComServiceNotification::GetVersion(
    __out /* [out, retval] */ IFabricServiceEndpointsVersion ** result)
{
    if (result == NULL) { return E_POINTER; }

    *result = WrapperFactory::create_com_wrapper(impl_->GetVersion()).DetachNoRelease();

    return S_OK;
}
