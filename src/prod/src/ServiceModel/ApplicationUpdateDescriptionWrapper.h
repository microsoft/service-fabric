// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationUpdateDescriptionWrapper : public Common::IFabricJsonSerializable, public Serialization::FabricSerializable
    {
    public:
        ApplicationUpdateDescriptionWrapper();

        ApplicationUpdateDescriptionWrapper(ApplicationUpdateDescriptionWrapper const &);

        ApplicationUpdateDescriptionWrapper(
            std::wstring const &appName,
            bool removeApplicationCapacity,
            uint mimimumScaleoutCount,
            uint maximumNodes,
            std::vector<Reliability::ApplicationMetricDescription> const & metrics);

        ~ApplicationUpdateDescriptionWrapper() {};

        Common::ErrorCode FromPublicApi(
            __in FABRIC_APPLICATION_UPDATE_DESCRIPTION const &appUpdateDesc);

        __declspec(property(get = get_UpdateMinimumNodes)) bool UpdateMinimumNodes;
        __declspec(property(get = get_UpdateMaximumNodes)) bool UpdateMaximumNodes;
        __declspec(property(get = get_UpdateMetrics)) bool UpdateMetrics;

        __declspec(property(get = get_AppName)) std::wstring const &ApplicationName;
        __declspec(property(get = get_RemoveApplicationCapacity)) bool RemoveApplicationCapacity;
        __declspec(property(get = get_MinimumNodes)) uint MinimumNodes;
        __declspec(property(get = get_MaximumNodes)) uint MaximumNodes;
        __declspec(property(get = get_Metrics)) std::vector<Reliability::ApplicationMetricDescription> const &Metrics;

        bool get_UpdateMinimumNodes() const { return (flags_ & FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS::FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_MINNODES) != 0; }
        bool get_UpdateMaximumNodes() const { return (flags_ & FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS::FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_MAXNODES) != 0; }
        bool get_UpdateMetrics() const { return (flags_ & FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS::FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_METRICS) != 0; }

        std::wstring const& get_AppName() const { return applicationName_; }
        bool get_RemoveApplicationCapacity() const { return removeApplicationCapacity_; }
        uint get_MinimumNodes() const { return minimumNodes_; }
        uint get_MaximumNodes() const { return maximumNodes_; }
        std::vector<Reliability::ApplicationMetricDescription> const& get_Metrics() const { return metrics_; };

        bool TryValidate(__out std::wstring & validationErrorMessage) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::RemoveApplicationCapacity, removeApplicationCapacity_)
            SERIALIZABLE_PROPERTY(Constants::MinimumNodes, minimumNodes_)
            SERIALIZABLE_PROPERTY(Constants::MaximumNodes, maximumNodes_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationMetrics, metrics_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_06(flags_, applicationName_, removeApplicationCapacity_, minimumNodes_, maximumNodes_, metrics_)

    private:
        LONG flags_;
        std::wstring applicationName_;
        bool removeApplicationCapacity_;
        uint maximumNodes_;
        uint minimumNodes_;
        std::vector<Reliability::ApplicationMetricDescription> metrics_;
    };
}
