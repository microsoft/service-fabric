// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedApplicationHealthQueryDescription");

DeployedApplicationHealthQueryDescription::DeployedApplicationHealthQueryDescription()
    : applicationName_()
    , nodeName_()
    , healthPolicy_()
    , eventsFilter_()
    , deployedServicePackagesFilter_()
    , healthStatsFilter_()
{
}

DeployedApplicationHealthQueryDescription::DeployedApplicationHealthQueryDescription(
    Common::NamingUri && applicationName,
    std::wstring && nodeName,
    std::unique_ptr<ApplicationHealthPolicy> && healthPolicy)
    : applicationName_(move(applicationName))
    , nodeName_(move(nodeName))
    , healthPolicy_(move(healthPolicy))
    , eventsFilter_()
    , deployedServicePackagesFilter_()
    , healthStatsFilter_()
{
}

DeployedApplicationHealthQueryDescription::~DeployedApplicationHealthQueryDescription()
{
}

ErrorCode DeployedApplicationHealthQueryDescription::FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION const & publicDeployedApplicationHealthQueryDescription)
{
    // ApplicationName
    {
        auto error = NamingUri::TryParse(
            publicDeployedApplicationHealthQueryDescription.ApplicationName,
            "ApplicationName",
            applicationName_);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // NodeName
    {
        HRESULT hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationHealthQueryDescription.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr))
        {
            ErrorCode error(ErrorCode::FromHResult(hr));
            Trace.WriteInfo(TraceSource, "NodeName::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // HealthPolicy
    if (publicDeployedApplicationHealthQueryDescription.HealthPolicy != NULL)
    {
        ApplicationHealthPolicy appPolicy;
        auto error = appPolicy.FromPublicApi(*publicDeployedApplicationHealthQueryDescription.HealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "HealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ApplicationHealthPolicy>(move(appPolicy));
    }

    // EventsFilter
    if (publicDeployedApplicationHealthQueryDescription.EventsFilter != NULL)
    {
        HealthEventsFilter eventsFilter;
        auto error = eventsFilter.FromPublicApi(*publicDeployedApplicationHealthQueryDescription.EventsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "EventsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        eventsFilter_ = make_unique<HealthEventsFilter>(move(eventsFilter));
    }

    // DeployedServicePackagesFilter
    if (publicDeployedApplicationHealthQueryDescription.DeployedServicePackagesFilter != NULL)
    {
        DeployedServicePackageHealthStatesFilter deployedServicePackagesFilter;
        auto error = deployedServicePackagesFilter.FromPublicApi(*publicDeployedApplicationHealthQueryDescription.DeployedServicePackagesFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "DeployedServicePackagesFilter::FromPublicApi error: {0}", error);
            return error;
        }

        deployedServicePackagesFilter_ = make_unique<DeployedServicePackageHealthStatesFilter>(move(deployedServicePackagesFilter));
    }

    if (publicDeployedApplicationHealthQueryDescription.Reserved != nullptr)
    {
        auto ex1 = static_cast<FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION_EX1*>(publicDeployedApplicationHealthQueryDescription.Reserved);

        // DeployedApplicationHealthStatisticsFilter
        if (ex1->HealthStatisticsFilter != nullptr)
        {
            healthStatsFilter_ = make_unique<DeployedApplicationHealthStatisticsFilter>();
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

void DeployedApplicationHealthQueryDescription::SetQueryArguments(__in QueryArgumentMap & argMap) const
{
    argMap.Insert(Query::QueryResourceProperties::Application::ApplicationName, applicationName_.Name);
    argMap.Insert(Query::QueryResourceProperties::Node::Name, nodeName_);

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

    if (deployedServicePackagesFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::DeployedServicePackagesFilter,
            deployedServicePackagesFilter_->ToString());
    }

    if (healthStatsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::HealthStatsFilter,
            healthStatsFilter_->ToString());
    }
}

