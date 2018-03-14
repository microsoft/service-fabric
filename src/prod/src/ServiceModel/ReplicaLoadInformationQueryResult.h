// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReplicaLoadInformationQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ReplicaLoadInformationQueryResult();

        ReplicaLoadInformationQueryResult(
            Common::Guid partitionId,
            int64 replicaOrInstanceId,
            std::vector<ServiceModel::LoadMetricReport> && loadMetricReports);
                
        ReplicaLoadInformationQueryResult(ReplicaLoadInformationQueryResult const & other);
        ReplicaLoadInformationQueryResult(ReplicaLoadInformationQueryResult && other);

        ReplicaLoadInformationQueryResult const & operator = (ReplicaLoadInformationQueryResult const & other);
        ReplicaLoadInformationQueryResult const & operator = (ReplicaLoadInformationQueryResult && other);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REPLICA_LOAD_INFORMATION & ) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_REPLICA_LOAD_INFORMATION const& );
             
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY(Constants::ReplicaOrInstanceId, replicaOrInstanceId_)
            SERIALIZABLE_PROPERTY(Constants::ReportedLoad, loadMetricReports_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_03(partitionId_, replicaOrInstanceId_, loadMetricReports_)

    private:
        Common::Guid partitionId_;
        int64 replicaOrInstanceId_;
        std::vector<ServiceModel::LoadMetricReport> loadMetricReports_;
    };
}
