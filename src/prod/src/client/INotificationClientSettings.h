// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class INotificationClientSettings
    {
    public:
        virtual FabricClientInternalSettings * operator -> () const = 0;

        virtual ~INotificationClientSettings() { }
    };

    typedef std::unique_ptr<INotificationClientSettings> INotificationClientSettingsUPtr;
}
