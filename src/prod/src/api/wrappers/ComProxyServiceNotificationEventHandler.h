// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyServiceNotificationEventHandler :
        public Common::ComponentRoot,
        public IServiceNotificationEventHandler
    {
        DENY_COPY(ComProxyServiceNotificationEventHandler);

    public:
        ComProxyServiceNotificationEventHandler(Common::ComPointer<IFabricServiceNotificationEventHandler> const & comImpl);

        virtual ~ComProxyServiceNotificationEventHandler();

        //
        // IServiceNotificationEventHandler
        //
        Common::ErrorCode OnNotification(IServiceNotificationPtr const &);
            
    private:
        Common::ComPointer<IFabricServiceNotificationEventHandler> const comImpl_;
    };
}
