// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ClusterLoadInformationQueryResult
      : public Serialization::FabricSerializable
      , public Common::IFabricJsonSerializable
    {
    public:
        ClusterLoadInformationQueryResult();

        ClusterLoadInformationQueryResult(
            Common::DateTime startTimeUtc,
            Common::DateTime endTimeUtc,
            std::vector<LoadMetricInformation> && loadMetricInformation);
                    
        ClusterLoadInformationQueryResult(ClusterLoadInformationQueryResult const & other);
        ClusterLoadInformationQueryResult(ClusterLoadInformationQueryResult && other);

        ClusterLoadInformationQueryResult const & operator = (ClusterLoadInformationQueryResult const & other);
        ClusterLoadInformationQueryResult const & operator = (ClusterLoadInformationQueryResult && other);
        
        __declspec(property(get=get_LastBalancingStartTimeUtc, put=put_LastBalancingStartTimeUtc)) Common::DateTime LastBalancingStartTimeUtc;
        Common::DateTime get_LastBalancingStartTimeUtc() const { return lastBalancingStartTimeUtc_; }
        void put_LastBalancingStartTimeUtc(Common::DateTime lastBalancingStartTimeUtc) { lastBalancingStartTimeUtc_ = lastBalancingStartTimeUtc; }
        
        __declspec(property(get=get_LastBalancingEndTimeUtc, put=put_LastBalancingEndTimeUtc)) Common::DateTime LastBalancingEndTimeUtc;
        Common::DateTime get_LastBalancingEndTimeUtc() const { return lastBalancingEndTimeUtc_; }
        void put_LastBalancingEndTimeUtc(Common::DateTime lastBalancingEndTimeUtc) { lastBalancingEndTimeUtc_ = lastBalancingEndTimeUtc; }
        
        __declspec(property(get=get_LoadMetricInformation)) std::vector<LoadMetricInformation> & LoadMetric;
        std::vector<ServiceModel::LoadMetricInformation> & get_LoadMetricInformation() { return loadMetricInformation_; }        

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_CLUSTER_LOAD_INFORMATION & publicNodeQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_CLUSTER_LOAD_INFORMATION const& publicNodeQueryResult);

        FABRIC_FIELDS_03(
            lastBalancingStartTimeUtc_,
            lastBalancingEndTimeUtc_,
            loadMetricInformation_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::LastBalancingStartTimeUtc, lastBalancingStartTimeUtc_)
            SERIALIZABLE_PROPERTY(Constants::LastBalancingEndTimeUtc, lastBalancingEndTimeUtc_)
            SERIALIZABLE_PROPERTY(Constants::LoadMetricInformation, loadMetricInformation_)
            END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        Common::DateTime lastBalancingStartTimeUtc_;
        Common::DateTime lastBalancingEndTimeUtc_;
        std::vector<ServiceModel::LoadMetricInformation> loadMetricInformation_;
    };

}
