// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("NodeHealthQueryDescription");

NodeHealthQueryDescription::NodeHealthQueryDescription()
    : nodeName_()
    , healthPolicy_()
    , eventsFilter_()
{
}

NodeHealthQueryDescription::NodeHealthQueryDescription(
    std::wstring && nodeName,
    std::unique_ptr<ClusterHealthPolicy> && healthPolicy)
    : nodeName_(move(nodeName))
    , healthPolicy_(move(healthPolicy))
    , eventsFilter_()
{
}

NodeHealthQueryDescription::~NodeHealthQueryDescription()
{
}

ErrorCode NodeHealthQueryDescription::FromPublicApi(FABRIC_NODE_HEALTH_QUERY_DESCRIPTION const & publicNodeHealthQueryDescription)
{
    // NodeName
    {
        auto hr = StringUtility::LpcwstrToWstring(publicNodeHealthQueryDescription.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr);
            Trace.WriteInfo(TraceSource, "NodeName::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // HealthPolicy
    if (publicNodeHealthQueryDescription.HealthPolicy != NULL)
    {
        ClusterHealthPolicy clusterPolicy;
        auto error = clusterPolicy.FromPublicApi(*publicNodeHealthQueryDescription.HealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "HealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ClusterHealthPolicy>(move(clusterPolicy));
    }

    // EventsFilter
    if (publicNodeHealthQueryDescription.EventsFilter != NULL)
    {
        HealthEventsFilter eventsFilter;
        auto error = eventsFilter.FromPublicApi(*publicNodeHealthQueryDescription.EventsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "EventsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        eventsFilter_ = make_unique<HealthEventsFilter>(move(eventsFilter));
    }

    return ErrorCode::Success();
}


void NodeHealthQueryDescription::SetQueryArguments(__in QueryArgumentMap & argMap) const
{
    argMap.Insert(Query::QueryResourceProperties::Node::Name, nodeName_);

    if (healthPolicy_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ClusterHealthPolicy,
            healthPolicy_->ToString());
    }

    if (eventsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::EventsFilter,
            eventsFilter_->ToString());
    }
}


