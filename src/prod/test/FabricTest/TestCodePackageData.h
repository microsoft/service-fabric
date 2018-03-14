// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class TestCodePackageData : public Serialization::FabricSerializable
    {
    public:
        TestCodePackageData(
            ServiceModel::ServiceManifestDescription const& description, 
            TestCodePackageContext const& codePackageContext)
            : description_(description),
            codePackageContext_(codePackageContext)
        {
        }

        TestCodePackageData() {}

        __declspec(property(get=get_Description)) ServiceModel::ServiceManifestDescription const& Description;
        inline ServiceModel::ServiceManifestDescription const & get_Description() const { return description_; };

        __declspec(property(get=get_CodePackageContext)) TestCodePackageContext const& CodePackageContext;
        inline TestCodePackageContext const & get_CodePackageContext() const { return codePackageContext_; };

        FABRIC_FIELDS_02(description_, codePackageContext_);

    private:
        ServiceModel::ServiceManifestDescription description_;
        TestCodePackageContext codePackageContext_;
    };
};
