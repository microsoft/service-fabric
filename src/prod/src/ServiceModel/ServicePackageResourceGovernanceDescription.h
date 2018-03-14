// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ServicePackageResourceGovernanceDescription : public Serialization::FabricSerializable
    {
    public:
        ServicePackageResourceGovernanceDescription();
        ServicePackageResourceGovernanceDescription(ServicePackageResourceGovernanceDescription const & other);
        ServicePackageResourceGovernanceDescription(ServicePackageResourceGovernanceDescription && other);

        ServicePackageResourceGovernanceDescription const & operator = (ServicePackageResourceGovernanceDescription const & other);
        ServicePackageResourceGovernanceDescription const & operator = (ServicePackageResourceGovernanceDescription && other);

        bool operator == (ServicePackageResourceGovernanceDescription const & other) const;
        bool operator != (ServicePackageResourceGovernanceDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        std::wstring ToString() const;

        // Set MemoryInMB limit based on values that are present in code package descriptions:
        //  - If current value is zero, means that user did not provide it in the manifest: set the new value.
        //  - If current value is not zero, user provided it in the manifest: do not change anything.
        void SetMemoryInMB(std::vector<DigestedCodePackageDescription> const& codePackages);

        bool HasRgChange(
            ServicePackageResourceGovernanceDescription const & other,
            std::vector<DigestedCodePackageDescription> const& myCodePackages,
            std::vector<DigestedCodePackageDescription> const& otherCodePackages
        ) const;


        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);

        // Used when upgrading from 5.7 to 6.0 release:
        //  - In 5.7 and earlier, CPU limits are stored in notUsed_ field (uint, previously called CpuCores).
        //  - In 6.0 a new field is introduced (double, CpuCores).
        // When deserializing after upgrade from 5.7 to 6.0 - old field needs to be read and new field needs to be set.
        void CorrectCPULimits();

        __declspec (property(get = get_CpuCores, put = put_CpuCores)) double CpuCores;
        double get_CpuCores() const { return cpuCores_ < 0.0001 ? notUsed_ : cpuCores_; }
        void put_CpuCores(double value) { cpuCores_ = value; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SERVICE_PACKAGE_RESOURCE_GOVERNANCE_DESCRIPTION & fabricSvcPkgResGovDesc) const;

        FABRIC_FIELDS_04(IsGoverned, notUsed_, MemoryInMB, cpuCores_);

    public:
        bool IsGoverned;
        uint MemoryInMB;

    private:
        friend struct ServiceManifestDescription;
        friend struct ServicePackageDescription;

        double cpuCores_;
        uint notUsed_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServicePackageResourceGovernanceDescription);
DEFINE_USER_MAP_UTILITY(ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription);

