// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class PartitionLoadInformationQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        PartitionLoadInformationQueryResult();

        PartitionLoadInformationQueryResult(
            Common::Guid partitionId,
            std::vector<ServiceModel::LoadMetricReport> && primaryLoadMetricReports,
            std::vector<ServiceModel::LoadMetricReport> && secondaryLoadMetricReports);
                
        PartitionLoadInformationQueryResult(PartitionLoadInformationQueryResult const & other);
        PartitionLoadInformationQueryResult(PartitionLoadInformationQueryResult && other);

        PartitionLoadInformationQueryResult const & operator = (PartitionLoadInformationQueryResult const & other);
        PartitionLoadInformationQueryResult const & operator = (PartitionLoadInformationQueryResult && other);

        __declspec(property(get = get_PrimaryLoadMetricReports)) std::vector<LoadMetricReport> const & PrimaryLoadMetricReports;
        std::vector<LoadMetricReport> const & get_PrimaryLoadMetricReports() const { return primaryLoadMetricReports_; }

        __declspec(property(get = get_SecondaryLoadMetricReports)) std::vector<LoadMetricReport> const & SecondaryLoadMetricReports;
        std::vector<LoadMetricReport> const & get_SecondaryLoadMetricReports() const { return secondaryLoadMetricReports_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_PARTITION_LOAD_INFORMATION & ) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_PARTITION_LOAD_INFORMATION const& );

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY(Constants::PrimaryLoadMetricReports, primaryLoadMetricReports_)
            SERIALIZABLE_PROPERTY(Constants::SecondaryLoadMetricReports, secondaryLoadMetricReports_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_03(partitionId_, primaryLoadMetricReports_, secondaryLoadMetricReports_);

    private:
        Common::Guid partitionId_;
        std::vector<LoadMetricReport> primaryLoadMetricReports_;
        std::vector<LoadMetricReport> secondaryLoadMetricReports_;
    };
}
