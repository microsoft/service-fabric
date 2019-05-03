// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class IHttpApplicationGatewayHandlerOperation
    {
    public:
        virtual ~IHttpApplicationGatewayHandlerOperation() = default;

        virtual void GetAdditionalHeadersToSend(__out std::unordered_map<std::wstring, std::wstring>& headers) = 0;

        virtual void GetServiceName(std::wstring & serviceName) = 0;
    };
}
