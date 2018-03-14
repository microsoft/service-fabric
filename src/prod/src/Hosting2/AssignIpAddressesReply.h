// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class AssignIpAddressesReply : public Serialization::FabricSerializable
    {
    public:
        AssignIpAddressesReply();
        AssignIpAddressesReply(
            Common::ErrorCode const & error,
            std::vector<std::wstring> const & assignedIps);

        __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get=get_AssignedIps)) std::vector<std::wstring> const & AssignedIps;
        std::vector<std::wstring> const & get_AssignedIps() const { return assignedIps_; }


        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(error_, assignedIps_);

    private:
        Common::ErrorCode error_;
        std::vector<std::wstring> assignedIps_;
    };
}
