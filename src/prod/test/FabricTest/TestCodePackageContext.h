// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class TestCodePackageContext : public Serialization::FabricSerializable
    {
    public:
        TestCodePackageContext(
            std::wstring const& nodeId, 
            Hosting2::CodePackageInstanceIdentifier const& codePackageId, 
            std::wstring const& instanceId,
            std::wstring const& codePackageVersion);

        TestCodePackageContext(
            std::wstring const& nodeId, 
            Hosting2::CodePackageInstanceIdentifier const& codePackageId,
            std::wstring const& instanceId,
            std::wstring const& codePackageVersion,
            TestMultiPackageHostContext const& testMultiPackageHostContext);

        TestCodePackageContext() {}

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        inline std::wstring const & get_NodeId() const { return nodeId_; };

        __declspec(property(get = get_CodePackageIdentifier)) Hosting2::CodePackageInstanceIdentifier const & CodePackageId;
        inline Hosting2::CodePackageInstanceIdentifier const & get_CodePackageIdentifier() const { return codePackageId_; }

        __declspec(property(get=get_InstanceId)) std::wstring const& InstanceId;
        inline std::wstring const& get_InstanceId() const { return instanceId_; };

        __declspec(property(get=get_CodePackageVersion)) std::wstring const& CodePackageVersion;
        inline std::wstring const& get_CodePackageVersion() const { return codePackageVersion_; };

        __declspec(property(get=get_IsMultiHost)) bool IsMultiHost;
        inline bool get_IsMultiHost() const { return isMultiHost_; };

        __declspec(property(get=get_MultiPackageHostContext)) TestMultiPackageHostContext const& MultiPackageHostContext;
        TestMultiPackageHostContext const& get_MultiPackageHostContext() const 
        { 
            TestCommon::TestSession::FailTestIfNot(isMultiHost_, "get_MultiPackageHostContext called non multi host context");
            return testMultiPackageHostContext_; 
        }

        static bool FromClientId(std::wstring const& clientId, TestCodePackageContext &);
        std::wstring ToClientId() const;

        bool operator == (TestCodePackageContext const & other) const;
        bool operator != (TestCodePackageContext const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_06(nodeId_, codePackageId_, instanceId_, codePackageVersion_, isMultiHost_, testMultiPackageHostContext_);

    private:
        std::wstring nodeId_;
        Hosting2::CodePackageInstanceIdentifier codePackageId_;
        std::wstring instanceId_;
        std::wstring codePackageVersion_;
        bool isMultiHost_;
        TestMultiPackageHostContext testMultiPackageHostContext_;

        static std::wstring const ParamDelimiter;
    };
};
