// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ResourceGovernancePolicy> element.
    struct DigestedCodePackageDescription;
    struct ServicePackagePoliciesDescription;
    struct ServiceManifestImportDescription;
    struct ResourceGovernancePolicyDescription : public Serialization::FabricSerializable
    {
    public:
        ResourceGovernancePolicyDescription(); 
        ResourceGovernancePolicyDescription(ResourceGovernancePolicyDescription const & other);
        ResourceGovernancePolicyDescription(ResourceGovernancePolicyDescription && other);

        ResourceGovernancePolicyDescription const & operator = (ResourceGovernancePolicyDescription const & other);
        ResourceGovernancePolicyDescription const & operator = (ResourceGovernancePolicyDescription && other);

        bool operator == (ResourceGovernancePolicyDescription const & other) const;
        bool operator != (ResourceGovernancePolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        std::wstring ToString() const;

        bool ShouldSetupCgroup() const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_RESOURCE_GOVERNANCE_POLICY_DESCRIPTION & fabricResourceGovernancePolicyDesc) const;

        FABRIC_FIELDS_12(CodePackageRef, MemoryInMB, MemorySwapInMB, MemoryReservationInMB, CpuShares,
            CpuPercent, MaximumIOps, MaximumIOBytesps, BlockIOWeight, CpusetCpus, NanoCpus, CpuQuota);

    public:
        std::wstring CodePackageRef;
        uint MemoryInMB;
        uint MemorySwapInMB;
        uint MemoryReservationInMB;
        uint CpuShares;
        uint CpuPercent;
        uint MaximumIOps;
        uint MaximumIOBytesps;
        uint BlockIOWeight;
        std::wstring CpusetCpus;
        uint64 NanoCpus;
        uint CpuQuota;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ServiceManifestImportDescription;
    
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
