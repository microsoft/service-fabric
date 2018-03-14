// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceConnectionEventHandler
    {
    public:
        virtual ~IServiceConnectionEventHandler() {};

        virtual void OnConnected(std::wstring const & connectionAddress) = 0;

        virtual void OnDisconnected(
            std::wstring const & connectionAddress,
            Common::ErrorCode const &) = 0;
    };
}
