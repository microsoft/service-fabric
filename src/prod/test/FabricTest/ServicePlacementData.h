// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ServicePlacementData : public Serialization::FabricSerializable
    {
    public:
        ServicePlacementData(
            std::map<std::wstring, TestServiceInfo> const& placedServices, 
            TestCodePackageContext const& codePackageContext);

        ServicePlacementData() {}

        __declspec(property(get=get_PlacedServices)) std::map<std::wstring, TestServiceInfo> const& PlacedServices;
        inline std::map<std::wstring, TestServiceInfo> const & get_PlacedServices() const { return placedServices_; };

        __declspec(property(get=get_CodePackageContext)) TestCodePackageContext const& CodePackageContext;
        inline TestCodePackageContext const & get_CodePackageContext() const { return codePackageContext_; };

        FABRIC_FIELDS_02(codePackageContext_, placedServices_);

    private:
        TestCodePackageContext codePackageContext_;
        std::map<std::wstring, TestServiceInfo> placedServices_;
    };
};

DEFINE_USER_MAP_UTILITY(std::wstring, FabricTest::TestServiceInfo);
