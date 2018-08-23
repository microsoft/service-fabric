// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ApplicationHealthQueryDescription");

ApplicationHealthQueryDescription::ApplicationHealthQueryDescription()
    : applicationName_()
    , healthPolicy_()
    , eventsFilter_()
    , servicesFilter_()
    , deployedApplicationsFilter_()
    , healthStatsFilter_()
{
}

ApplicationHealthQueryDescription::ApplicationHealthQueryDescription(
    Common::NamingUri && applicationName,
    std::unique_ptr<ApplicationHealthPolicy> && healthPolicy)
    : applicationName_(move(applicationName))
    , healthPolicy_(move(healthPolicy))
    , eventsFilter_()
    , servicesFilter_()
    , deployedApplicationsFilter_()
    , healthStatsFilter_()
{
}

ApplicationHealthQueryDescription::~ApplicationHealthQueryDescription()
{
}

ErrorCode ApplicationHealthQueryDescription::FromPublicApi(FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION const & publicApplicationHealthQueryDescription)
{
    // ApplicationName
    {
        auto error = NamingUri::TryParse(
            publicApplicationHealthQueryDescription.ApplicationName, 
            "ApplicationName",
            applicationName_);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // HealthPolicy
    if (publicApplicationHealthQueryDescription.HealthPolicy != NULL)
    {
        ApplicationHealthPolicy appPolicy;
        auto error = appPolicy.FromPublicApi(*publicApplicationHealthQueryDescription.HealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "HealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ApplicationHealthPolicy>(move(appPolicy));
    }

    // EventsFilter
    if (publicApplicationHealthQueryDescription.EventsFilter != NULL)
    {
        HealthEventsFilter eventsFilter;
        auto error = eventsFilter.FromPublicApi(*publicApplicationHealthQueryDescription.EventsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "EventsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        eventsFilter_ = make_unique<HealthEventsFilter>(move(eventsFilter));
    }

    // ServicesFilter
    if (publicApplicationHealthQueryDescription.ServicesFilter != NULL)
    {
        ServiceHealthStatesFilter servicesFilter;
        auto error = servicesFilter.FromPublicApi(*publicApplicationHealthQueryDescription.ServicesFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ServicesFilter::FromPublicApi error: {0}", error);
            return error;
        }

        servicesFilter_ = make_unique<ServiceHealthStatesFilter>(move(servicesFilter));
    }

    // DeployedApplicationsFilter
    if (publicApplicationHealthQueryDescription.DeployedApplicationsFilter != NULL)
    {
        DeployedApplicationHealthStatesFilter deployedApplicationsFilter;
        auto error = deployedApplicationsFilter.FromPublicApi(*publicApplicationHealthQueryDescription.DeployedApplicationsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "DeployedApplicationsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        deployedApplicationsFilter_ = make_unique<DeployedApplicationHealthStatesFilter>(move(deployedApplicationsFilter));
    }

    if (publicApplicationHealthQueryDescription.Reserved != nullptr)
    {
        auto ex1 = static_cast<FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION_EX1*>(publicApplicationHealthQueryDescription.Reserved);

        // ClusterHealthStatisticsFilter
        if (ex1->HealthStatisticsFilter != nullptr)
        {
            healthStatsFilter_ = make_unique<ApplicationHealthStatisticsFilter>();
            auto error = healthStatsFilter_->FromPublicApi(*ex1->HealthStatisticsFilter);
            if (!error.IsSuccess())
            {
                Trace.WriteInfo(TraceSource, "HealthStatisticsFilter::FromPublicApi error: {0}", error);
                return error;
            }
        }
    }

    return ErrorCode::Success();
}

void ApplicationHealthQueryDescription::SetQueryArguments(__in QueryArgumentMap & argMap) const
{
    argMap.Insert(Query::QueryResourceProperties::Application::ApplicationName, applicationName_.Name);

    if (healthPolicy_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ApplicationHealthPolicy,
            healthPolicy_->ToString());
    }

    if (eventsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::EventsFilter,
            eventsFilter_->ToString());
    }

    if (servicesFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ServicesFilter,
            servicesFilter_->ToString());
    }

    if (deployedApplicationsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::DeployedApplicationsFilter,
            deployedApplicationsFilter_->ToString());
    }

    if (healthStatsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::HealthStatsFilter,
            healthStatsFilter_->ToString());
    }
}
