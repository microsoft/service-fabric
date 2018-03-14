// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTypeInfo : public Serialization::FabricSerializable
    {
    public:
        ServiceTypeInfo();

        ServiceTypeInfo(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & codePackageName);

        ServiceTypeInfo(ServiceTypeInfo const & other);
        ServiceTypeInfo & operator=(ServiceTypeInfo const & other);

        ServiceTypeInfo(ServiceTypeInfo && other);
        ServiceTypeInfo & operator=(ServiceTypeInfo && other);

        __declspec(property(get = get_VersionedServiceTypeId)) ServiceModel::VersionedServiceTypeIdentifier const & VersionedServiceTypeId;
        ServiceModel::VersionedServiceTypeIdentifier const & get_VersionedServiceTypeId() const { return versionedServiceTypeId_; }

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        std::wstring const & get_CodePackageName() const { return codePackageName_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_02(versionedServiceTypeId_, codePackageName_);

    private:
        ServiceModel::VersionedServiceTypeIdentifier versionedServiceTypeId_;
        std::wstring codePackageName_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ServiceTypeInfo);
