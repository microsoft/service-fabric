// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ApplicationCapacityDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ApplicationCapacityDescription();

        ApplicationCapacityDescription(
            uint minimumNodes,
            uint maximumNodes,
            std::vector<Reliability::ApplicationMetricDescription> metrics);

        ApplicationCapacityDescription(ApplicationCapacityDescription const& other);

        ApplicationCapacityDescription(ApplicationCapacityDescription && other);

        ApplicationCapacityDescription & operator = (ApplicationCapacityDescription const& other);

        ApplicationCapacityDescription & operator = (ApplicationCapacityDescription && other);

        bool operator == (ApplicationCapacityDescription const & other) const;
        bool operator != (ApplicationCapacityDescription const & other) const;

        __declspec(property(get = get_MinimumNodes, put = put_MinimumNodes)) uint MinimumNodes;
        uint get_MinimumNodes() const { return minimumNodes_; }
        void put_MinimumNodes(uint minNodes) { minimumNodes_ = minNodes; }

        __declspec(property(get = get_MaximumNodes, put = put_MaximumNodes)) uint MaximumNodes;
        uint get_MaximumNodes() const { return maximumNodes_; }
        void put_MaximumNodes(uint maxNodes) { maximumNodes_ = maxNodes; }

        __declspec(property(get = get_Metrics, put = put_Metrics)) std::vector<Reliability::ApplicationMetricDescription> const &Metrics;
        std::vector<Reliability::ApplicationMetricDescription> const& get_Metrics() const { return metrics_; };
        void put_Metrics(std::vector<Reliability::ApplicationMetricDescription> const& metrics) { metrics_ = metrics; }

        Common::ErrorCode FromPublicApi(__in const FABRIC_APPLICATION_CAPACITY_DESCRIPTION * nativeDescription);

        bool TryValidate(__out std::wstring & validationErrorMessage) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(minimumNodes_, maximumNodes_, metrics_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::MinimumNodes, minimumNodes_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaximumNodes, maximumNodes_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationMetrics, metrics_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        uint maximumNodes_;
        uint minimumNodes_;
        std::vector<Reliability::ApplicationMetricDescription> metrics_;
    };
}

