// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ResolveSystemServiceRequestBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        ResolveSystemServiceRequestBody() : name_() { }

        ResolveSystemServiceRequestBody(std::wstring const & name) : name_(name) { }

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return name_; }

        FABRIC_FIELDS_01(name_);

    private:
        std::wstring name_;
    };
}
