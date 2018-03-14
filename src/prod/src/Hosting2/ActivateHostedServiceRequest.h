// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ActivateHostedServiceRequest : public Serialization::FabricSerializable
    {
    public:

        ActivateHostedServiceRequest();

        ActivateHostedServiceRequest(HostedServiceParameters const & params);

        __declspec(property(get=get_Parameters)) HostedServiceParameters const & Parameters;
        HostedServiceParameters const & get_Parameters() const { return params_; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(params_);

    private:

        HostedServiceParameters params_;
    };
}
