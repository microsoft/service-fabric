// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GetDeployedNetworksReply : public Serialization::FabricSerializable
    {
    public:
        GetDeployedNetworksReply();
        GetDeployedNetworksReply(std::vector<std::wstring> const & networkNames);

        __declspec(property(get=get_NetworkNames)) std::vector<std::wstring> const & NetworkNames;
        std::vector<std::wstring> const & get_NetworkNames() const { return networkNames_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(networkNames_);

    private:
        std::vector<std::wstring> networkNames_;
    };
}