// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceNotificationEventHandler
    {
    public:
        virtual ~IServiceNotificationEventHandler() {};

        virtual Common::ErrorCode OnNotification(IServiceNotificationPtr const &) = 0;
    };
}
