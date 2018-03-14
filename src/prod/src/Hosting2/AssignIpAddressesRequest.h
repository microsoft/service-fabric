// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class AssignIpAddressesRequest : public Serialization::FabricSerializable
    {
    public:
        AssignIpAddressesRequest();
        AssignIpAddressesRequest(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & codePackages,
            bool cleanup);

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_ServicePackageId)) std::wstring const & ServicePackageId;
        std::wstring const & get_ServicePackageId() const { return servicePackageId_; }

        __declspec(property(get=get_CodePackageNames)) std::vector<std::wstring> const & CodePackageNames;
        std::vector<std::wstring> const & get_CodePackageNames() const { return codePackageNames_; }


        __declspec(property(get = get_Cleanup)) bool Cleanup;
        bool get_Cleanup() const { return cleanup_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(nodeId_, servicePackageId_, codePackageNames_, cleanup_);

    private:
        std::wstring nodeId_;
        std::wstring servicePackageId_;
        std::vector<std::wstring> codePackageNames_;
        bool cleanup_;
    };
}
