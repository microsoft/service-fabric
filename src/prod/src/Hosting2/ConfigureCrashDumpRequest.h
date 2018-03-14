// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureCrashDumpRequest : public Serialization::FabricSerializable
    {
    public:
        ConfigureCrashDumpRequest();
        ConfigureCrashDumpRequest(
            bool enable,
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & exeNames);

        __declspec(property(get=get_Enable)) bool const & Enable;
        bool const & get_Enable() { return enable_; }

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() { return nodeId_; }

        __declspec(property(get=get_ServicePackageId)) std::wstring const & ServicePackageId;
        std::wstring const & get_ServicePackageId() { return servicePackageId_; }

        __declspec(property(get=get_ExeNames)) std::vector<std::wstring> const & ExeNames;
        std::vector<std::wstring> const & get_ExeNames() { return exeNames_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(enable_, nodeId_, servicePackageId_, exeNames_);

    private:
        bool enable_;
        std::wstring nodeId_;
        std::wstring servicePackageId_;
        std::vector<std::wstring> exeNames_;
    };
}
