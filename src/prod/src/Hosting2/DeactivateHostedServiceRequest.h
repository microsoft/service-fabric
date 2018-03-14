// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class DeactivateHostedServiceRequest : public Serialization::FabricSerializable
    {
    public:

        DeactivateHostedServiceRequest();

        DeactivateHostedServiceRequest(std::wstring const & serviceName, bool graceful = true);

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_Graceful)) bool Graceful;
        bool get_Graceful() const { return graceful_; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(serviceName_, graceful_);

    private:

        std::wstring serviceName_;
        bool graceful_;
    };
}
