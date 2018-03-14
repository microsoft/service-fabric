// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationLoadInformationQueryResult
      : public Serialization::FabricSerializable
      , public Common::IFabricJsonSerializable
    {
    public:
        ApplicationLoadInformationQueryResult();

        ApplicationLoadInformationQueryResult(
            std::wstring const & applicationName,
            uint minimumNodes,
            uint maximumNodes,
            uint nodeNodes,
            std::vector<ServiceModel::ApplicationLoadMetricInformation> && applicationLoadMetricInformation);

        ApplicationLoadInformationQueryResult(ApplicationLoadInformationQueryResult const & other);
        ApplicationLoadInformationQueryResult(ApplicationLoadInformationQueryResult && other);

        ApplicationLoadInformationQueryResult const & operator = (ApplicationLoadInformationQueryResult const & other);
        ApplicationLoadInformationQueryResult const & operator = (ApplicationLoadInformationQueryResult && other);

        __declspec(property(get=get_ApplicationName, put=put_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }
        void put_ApplicationName(std::wstring const& applicationName) { applicationName_ = applicationName; }

        __declspec(property(get = get_MinimumNodes, put = put_MinimumNodes)) uint MinimumNodes;
        uint get_MinimumNodes() const { return minimumNodes_; }
        void put_MinimumNodes(uint minimumNodes) { minimumNodes_ = minimumNodes; }

        __declspec(property(get = get_MaximumNodes, put = put_MaximumNodes)) uint MaximumNodes;
        uint get_MaximumNodes() const { return maximumNodes_; }
        void put_MaximumNodes(uint maximumNodes) { maximumNodes_ = maximumNodes; }

        __declspec(property(get = get_NodeCount, put = put_NodeCount)) uint NodeCount;
        uint get_NodeCount() const { return nodeCount_; }
        void put_NodeCount(uint nodeCount) { nodeCount_ = nodeCount; }

        __declspec(property(get=get_ApplicationLoadMetricInformation, put=put_ApplicationLoadMetricInformation)) std::vector<ServiceModel::ApplicationLoadMetricInformation> & ApplicationLoadMetricInformation;
        std::vector<ServiceModel::ApplicationLoadMetricInformation> & get_ApplicationLoadMetricInformation() { return applicationLoadMetricInformation_; }
        void put_ApplicationLoadMetricInformation(std::vector<ServiceModel::ApplicationLoadMetricInformation> && applicationLoadMetricInformation) { applicationLoadMetricInformation_ = move(applicationLoadMetricInformation); }

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_APPLICATION_LOAD_INFORMATION & publicApplicationQueryResult) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_APPLICATION_LOAD_INFORMATION const& publicApplicationQueryResult);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_05(
            applicationName_,
            minimumNodes_,
            maximumNodes_,
            nodeCount_,
            applicationLoadMetricInformation_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::MinimumNodes, minimumNodes_)
            SERIALIZABLE_PROPERTY(Constants::MaximumNodes, maximumNodes_)
            SERIALIZABLE_PROPERTY(Constants::NodeCount, nodeCount_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationLoadMetricInformation, applicationLoadMetricInformation_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:

        std::wstring applicationName_;
        uint minimumNodes_;
        uint maximumNodes_;
        uint nodeCount_;
        std::vector<ServiceModel::ApplicationLoadMetricInformation> applicationLoadMetricInformation_;
    };

}
