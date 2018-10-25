// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureNodeForDnsServiceRequest : public Serialization::FabricSerializable
    {
    public:
        ConfigureNodeForDnsServiceRequest();
        ConfigureNodeForDnsServiceRequest(
            bool isDnsServiceEnabled,
            bool setAsPreferredDns);

        __declspec(property(get = get_IsDnsServiceEnabled)) bool const IsDnsServiceEnabled;
        bool const get_IsDnsServiceEnabled() const { return isDnsServiceEnabled_; }

        __declspec(property(get = get_SetAsPreferredDns)) bool const SetAsPreferredDns;
        bool const get_SetAsPreferredDns() const { return setAsPreferredDns_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(isDnsServiceEnabled_, setAsPreferredDns_);

    private:
        bool isDnsServiceEnabled_;
        bool setAsPreferredDns_;
    };
}
