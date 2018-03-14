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
            bool setAsPreferredDns,
            std::wstring const & sid);

        __declspec(property(get = get_IsDnsServiceEnabled)) bool const IsDnsServiceEnabled;
        bool const get_IsDnsServiceEnabled() const { return isDnsServiceEnabled_; }

        __declspec(property(get = get_SetAsPreferredDns)) bool const SetAsPreferredDns;
        bool const get_SetAsPreferredDns() const { return setAsPreferredDns_; }

        __declspec(property(get = get_Sid)) std::wstring const & Sid;
        std::wstring const & get_Sid() const { return sid_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(isDnsServiceEnabled_, setAsPreferredDns_, sid_);

    private:
        bool isDnsServiceEnabled_;
        bool setAsPreferredDns_;
        std::wstring sid_;
    };
}
