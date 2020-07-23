// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class UpdateOverlayNetworkRoutesReply : public Serialization::FabricSerializable
    {
    public:
        UpdateOverlayNetworkRoutesReply() {}
        UpdateOverlayNetworkRoutesReply(
            std::wstring const & networkName,
            std::wstring const & nodeIpAddress,
            Common::ErrorCode const & error);

        ~UpdateOverlayNetworkRoutesReply();

        __declspec(property(get = get_NetworkName)) std::wstring const & NetworkName;
        std::wstring const & get_NetworkName() const { return networkName_; }

        __declspec(property(get = get_NodeIpAddress)) std::wstring const & NodeIpAddress;
        std::wstring const & get_NodeIpAddress() const { return nodeIpAddress_; }

        __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(networkName_, nodeIpAddress_, error_);

    private:
        std::wstring networkName_;
        std::wstring nodeIpAddress_;
        Common::ErrorCode error_;
    };
}
