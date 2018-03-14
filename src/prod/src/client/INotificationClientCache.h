// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class INotificationClientCache 
    {
    public:
        virtual Common::AsyncOperationSPtr BeginUpdateCacheEntries(
            __in Naming::ServiceNotification &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndUpdateCacheEntries(
            Common::AsyncOperationSPtr const &,
            __out std::vector<ServiceNotificationResultSPtr> &) = 0;

        virtual ~INotificationClientCache() { }
    };
}
