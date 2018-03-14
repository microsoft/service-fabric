// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ResolveSystemServiceReplyBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        ResolveSystemServiceReplyBody() : serviceEndpoint_() { }

        ResolveSystemServiceReplyBody(std::wstring const & serviceEndpoint) : serviceEndpoint_(serviceEndpoint) { }

        __declspec(property(get=get_ServiceEndpoint)) std::wstring const & ServiceEndpoint;
        std::wstring const & get_ServiceEndpoint() const { return serviceEndpoint_; }

        FABRIC_FIELDS_01(serviceEndpoint_);

    private:
        std::wstring serviceEndpoint_;
    };
}
