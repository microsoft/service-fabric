// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class NodeLoadInformationQueryResult
      : public Serialization::FabricSerializable
      , public Common::IFabricJsonSerializable
    {
    public:
        NodeLoadInformationQueryResult();

        NodeLoadInformationQueryResult(
            std::wstring const & nodeName,
            std::vector<ServiceModel::NodeLoadMetricInformation> && nodeLoadMetricInformation);

        NodeLoadInformationQueryResult(NodeLoadInformationQueryResult const & other) = default;
        NodeLoadInformationQueryResult(NodeLoadInformationQueryResult && other) = default;

        NodeLoadInformationQueryResult & operator = (NodeLoadInformationQueryResult const & other) = default;
        NodeLoadInformationQueryResult & operator = (NodeLoadInformationQueryResult && other) = default;
        
        __declspec(property(get=get_NodeName, put=put_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }
        void put_NodeName(std::wstring const& nodeName) { nodeName_ = nodeName; }
        
        __declspec(property(get=get_NodeLoadMetricInformation, put=put_NodeLoadMetricInformation)) std::vector<ServiceModel::NodeLoadMetricInformation> & NodeLoadMetricInformation;
        std::vector<ServiceModel::NodeLoadMetricInformation> & get_NodeLoadMetricInformation() { return nodeLoadMetricInformation_; }        
        void put_NodeLoadMetricInformation(std::vector<ServiceModel::NodeLoadMetricInformation> && nodeLoadMetricInformation) { nodeLoadMetricInformation_ = move(nodeLoadMetricInformation); }

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_NODE_LOAD_INFORMATION & publicNodeQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_NODE_LOAD_INFORMATION const& publicNodeQueryResult);

        FABRIC_FIELDS_02(
            nodeName_,
            nodeLoadMetricInformation_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY(Constants::NodeLoadMetricInformation, nodeLoadMetricInformation_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring nodeName_;
        std::vector<ServiceModel::NodeLoadMetricInformation> nodeLoadMetricInformation_;
    };

}
