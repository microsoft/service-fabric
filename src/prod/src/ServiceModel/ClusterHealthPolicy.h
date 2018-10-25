// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    typedef std::map<std::wstring, BYTE> ApplicationTypeHealthPolicyMap;

    class ClusterHealthPolicy 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ClusterHealthPolicy();

        ClusterHealthPolicy(
            bool considerWarningAsError,
            BYTE maxPercentUnhealthyNodes,
            BYTE maxPercentUnhealthyApplications);

        ClusterHealthPolicy(ClusterHealthPolicy const & other) = default;
        ClusterHealthPolicy & operator = (ClusterHealthPolicy const & other) = default;
        
        ClusterHealthPolicy(ClusterHealthPolicy && other) = default;
        ClusterHealthPolicy & operator = (ClusterHealthPolicy && other) = default;

        ~ClusterHealthPolicy();

        __declspec(property(get = get_ApplicationTypeMap)) ApplicationTypeHealthPolicyMap const & ApplicationTypeMap;
        inline ApplicationTypeHealthPolicyMap const & get_ApplicationTypeMap() const { return applicationTypeMap_; }

        __declspec(property(get=get_ConsiderWarningAsError)) bool ConsiderWarningAsError;
        inline bool get_ConsiderWarningAsError() const { return considerWarningAsError_; }
        
        __declspec(property(get=get_MaxPercentUnhealthyNodes)) BYTE MaxPercentUnhealthyNodes;
        inline BYTE get_MaxPercentUnhealthyNodes() const { return maxPercentUnhealthyNodes_; }

        __declspec(property(get=get_MaxPercentUnhealthyApplications)) BYTE MaxPercentUnhealthyApplications;
        inline BYTE get_MaxPercentUnhealthyApplications() const { return maxPercentUnhealthyApplications_; }

        Common::ErrorCode AddApplicationTypeHealthPolicy(std::wstring && applicationTypeName, BYTE maxPercentUnhealthyApplications);

        bool operator == (ClusterHealthPolicy const & other) const;
        bool operator != (ClusterHealthPolicy const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CLUSTER_HEALTH_POLICY const &);
        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_CLUSTER_HEALTH_POLICY &) const;

        bool TryValidate(__out std::wstring & validationErrorMessage) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const &, __out ClusterHealthPolicy &);
        
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ConsiderWarningAsError, considerWarningAsError_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyNodes, maxPercentUnhealthyNodes_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyApplications, maxPercentUnhealthyApplications_)
            SERIALIZABLE_PROPERTY_IF(Constants::ApplicationTypeHealthPolicyMap, applicationTypeMap_, Common::CommonConfig::GetConfig().EnableApplicationTypeHealthEvaluation)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_04(considerWarningAsError_, maxPercentUnhealthyNodes_, maxPercentUnhealthyApplications_, applicationTypeMap_);

    private:
        bool considerWarningAsError_;
        BYTE maxPercentUnhealthyNodes_;
        BYTE maxPercentUnhealthyApplications_;
        ApplicationTypeHealthPolicyMap applicationTypeMap_;
    };

    typedef std::shared_ptr<ClusterHealthPolicy> ClusterHealthPolicySPtr;
    typedef std::unique_ptr<ClusterHealthPolicy> ClusterHealthPolicyUPtr;
}

DEFINE_USER_MAP_UTILITY( std::wstring, BYTE );

