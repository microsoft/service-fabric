// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class TestMultiPackageHostContext : public Serialization::FabricSerializable
    {
    public:
        TestMultiPackageHostContext(
            std::wstring const& nodeId,
            std::wstring const& hostId);

        TestMultiPackageHostContext() {}

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        inline std::wstring const & get_NodeId() const { return nodeId_; };

        __declspec(property(get=get_HostId)) std::wstring const & HostId;
        inline std::wstring const & get_HostId() const { return hostId_; };


        static bool FromClientId(std::wstring const& clientId, TestMultiPackageHostContext &);
        std::wstring ToClientId() const;

        bool operator == (TestMultiPackageHostContext const & other) const;
        bool operator != (TestMultiPackageHostContext const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(nodeId_, hostId_);

    private:
        std::wstring nodeId_;
        std::wstring hostId_;

        static std::wstring const ParamDelimiter;
    };
};
