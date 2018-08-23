// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedServicePackageHealthQueryDescription");

DeployedServicePackageHealthQueryDescription::DeployedServicePackageHealthQueryDescription()
    : applicationName_()
    , nodeName_()
    , serviceManifestName_()
    , servicePackageActivationId_()
    , healthPolicy_()
    , eventsFilter_()
{
}

DeployedServicePackageHealthQueryDescription::DeployedServicePackageHealthQueryDescription(
    Common::NamingUri && applicationName,
    std::wstring && serviceManifestName,
    std::wstring && servicePackageActivationId,
    std::wstring && nodeName,
    std::unique_ptr<ApplicationHealthPolicy> && healthPolicy)
    : applicationName_(move(applicationName))
    , nodeName_(move(nodeName))
    , serviceManifestName_(move(serviceManifestName))
    , servicePackageActivationId_(move(servicePackageActivationId))
    , healthPolicy_(move(healthPolicy))
    , eventsFilter_()
{
}

DeployedServicePackageHealthQueryDescription::~DeployedServicePackageHealthQueryDescription()
{
}

ErrorCode DeployedServicePackageHealthQueryDescription::FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION const & publicDeployedServicePackageHealthQueryDescription)
{
    // ApplicationName
    {
        auto error = NamingUri::TryParse(
            publicDeployedServicePackageHealthQueryDescription.ApplicationName,
            "ApplicationName",
            applicationName_);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // NodeName
    {
        auto error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealthQueryDescription.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "NodeName::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // ServiceManifestName
    {
        auto error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealthQueryDescription.ServiceManifestName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceManifestName_);
		if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ServiceManifestName::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // HealthPolicy
    if (publicDeployedServicePackageHealthQueryDescription.HealthPolicy != NULL)
    {
        ApplicationHealthPolicy appPolicy;
        auto error = appPolicy.FromPublicApi(*publicDeployedServicePackageHealthQueryDescription.HealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "HealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ApplicationHealthPolicy>(move(appPolicy));
    }

    // EventsFilter
    if (publicDeployedServicePackageHealthQueryDescription.EventsFilter != NULL)
    {
        HealthEventsFilter eventsFilter;
        auto error = eventsFilter.FromPublicApi(*publicDeployedServicePackageHealthQueryDescription.EventsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "EventsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        eventsFilter_ = make_unique<HealthEventsFilter>(move(eventsFilter));
    }

    // ServicePackageActivationId
    if (publicDeployedServicePackageHealthQueryDescription.Reserved == nullptr)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto healthQueryDescriptionEx1 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION_EX1*)publicDeployedServicePackageHealthQueryDescription.Reserved;

    auto error = StringUtility::LpcwstrToWstring2(healthQueryDescriptionEx1->ServicePackageActivationId, true, servicePackageActivationId_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing ServicePackageActivationId in FromPublicAPI: {0}", error);
        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void DeployedServicePackageHealthQueryDescription::SetQueryArguments(__in QueryArgumentMap & argMap) const
{
    argMap.Insert(Query::QueryResourceProperties::Application::ApplicationName, applicationName_.Name);
    argMap.Insert(Query::QueryResourceProperties::Node::Name, nodeName_);
    argMap.Insert(Query::QueryResourceProperties::ServicePackage::ServiceManifestName, serviceManifestName_);
    
	if (!servicePackageActivationId_.empty())
	{
		argMap.Insert(Query::QueryResourceProperties::ServicePackage::ServicePackageActivationId, servicePackageActivationId_);
	}
	
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
}

