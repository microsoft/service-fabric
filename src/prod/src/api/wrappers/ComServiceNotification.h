// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComServiceNotification
        : public IFabricServiceNotification,
          private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComServiceNotification);

        BEGIN_COM_INTERFACE_LIST(ComServiceNotification)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricServiceNotification)
            COM_INTERFACE_ITEM(IID_IFabricServiceNotification,IFabricServiceNotification)
        END_COM_INTERFACE_LIST()
        
    public:

        ComServiceNotification();

        explicit ComServiceNotification(IServiceNotificationPtr const & impl);

        IServiceNotificationPtr const & get_Impl() const { return impl_; }

        const FABRIC_SERVICE_NOTIFICATION * STDMETHODCALLTYPE get_Notification();

        HRESULT STDMETHODCALLTYPE GetVersion(
            __out /* [out, retval] */ IFabricServiceEndpointsVersion ** result);

    private:

        IServiceNotificationPtr impl_;
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_SERVICE_NOTIFICATION> notification_;
        Common::RwLock lock_;
    };
}

