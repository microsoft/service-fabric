// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NodeHealthQueryDescription
    {
        DENY_COPY(NodeHealthQueryDescription)

    public:
        NodeHealthQueryDescription();

        NodeHealthQueryDescription(
            std::wstring && nodeName,
            std::unique_ptr<ClusterHealthPolicy> && healthPolicy);

        NodeHealthQueryDescription(NodeHealthQueryDescription && other) = default;
        NodeHealthQueryDescription & operator = (NodeHealthQueryDescription && other) = default;

        ~NodeHealthQueryDescription();

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_HealthPolicy)) std::unique_ptr<ClusterHealthPolicy> const & HealthPolicy;
        std::unique_ptr<ClusterHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_EventsFilter)) std::unique_ptr<HealthEventsFilter> const & EventsFilter;
        std::unique_ptr<HealthEventsFilter> const & get_EventsFilter() const { return eventsFilter_; }

        void SetEventsFilter(std::unique_ptr<HealthEventsFilter> && eventsFilter) { eventsFilter_ = move(eventsFilter); }

        Common::ErrorCode FromPublicApi(FABRIC_NODE_HEALTH_QUERY_DESCRIPTION const & publicNodeHealthQueryDescription);

        void SetQueryArguments(__in QueryArgumentMap & argMap) const;

    private:
        std::wstring nodeName_;
        std::unique_ptr<ClusterHealthPolicy> healthPolicy_;
        std::unique_ptr<HealthEventsFilter> eventsFilter_;
    };
}

