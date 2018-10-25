//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ApplicationDescriptionQueryResult)
INITIALIZE_SIZE_ESTIMATION(ApplicationDescriptionQueryResult::ApplicationProperties)

ApplicationDescriptionQueryResult::ApplicationProperties::ApplicationProperties(
    std::wstring const &description,
    std::vector<std::wstring> &&serviceNames,
    std::wstring const &statusDetails,
    ServiceModel::SingleInstanceDeploymentStatus::Enum status)
    : description_(description)
    , serviceNames_(move(serviceNames))
    , statusDetails_(statusDetails)
    , status_(status)
    , UnhealthyEvaluation()
{
}


ApplicationDescriptionQueryResult::ApplicationDescriptionQueryResult(
    SingleInstanceDeploymentQueryResult &&deploymentResult,
    wstring const &description,
    vector<std::wstring> &&serviceNames)
    : applicationUri_(deploymentResult.ApplicationUri)
    , deploymentName_(deploymentResult.DeploymentName)
    , properties_(
        description,
        move(serviceNames),
        deploymentResult.StatusDetails,
        deploymentResult.Status)
{
    NamingUri::FabricNameToId(deploymentResult.ApplicationUri.ToString(), applicationName_);
}

wstring ApplicationDescriptionQueryResult::CreateContinuationToken() const
{
    return deploymentName_;
}
