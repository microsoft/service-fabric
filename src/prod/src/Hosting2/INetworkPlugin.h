// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class provides the virtual methods which can be overridden
    // in specialized platform specific classes. The virtual methods provide mechanisms for
    // initializing the OS specific networking plugin. Since network plugins do not maintain state,
    // a singleton instance can be used across multiple drivers.
    class INetworkPlugin
    {
        DENY_COPY(INetworkPlugin)

    public:
        INetworkPlugin() {};
        virtual ~INetworkPlugin() {};
    };
}
