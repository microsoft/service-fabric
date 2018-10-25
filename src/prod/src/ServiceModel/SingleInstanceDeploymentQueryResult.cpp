//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

SingleInstanceDeploymentQueryResult::SingleInstanceDeploymentQueryResult(
    wstring const & deploymentName,
    NamingUri const & applicationUri,
    SingleInstanceDeploymentStatus::Enum const status,
    wstring const & statusDetails)
    : deploymentName_(deploymentName)
    , applicationUri_(applicationUri)
    , status_(status)
    , statusDetails_(statusDetails)
{
}

SingleInstanceDeploymentQueryResult & SingleInstanceDeploymentQueryResult::operator=(
    SingleInstanceDeploymentQueryResult const & other)
{
    if (this != &other)
    {
        deploymentName_ = other.deploymentName_;
        applicationUri_ = other.applicationUri_;
        status_ = other.status_;
        statusDetails_ = other.statusDetails_;
    }
    return *this;
}

SingleInstanceDeploymentQueryResult & SingleInstanceDeploymentQueryResult::operator=(
    SingleInstanceDeploymentQueryResult && other)
{
    if (this != &other)
    {
        deploymentName_ = move(other.deploymentName_);
        applicationUri_ = move(other.applicationUri_);
        status_ = other.status_;
        statusDetails_ = move(other.statusDetails_);
    }
    return *this;
}

wstring SingleInstanceDeploymentQueryResult::CreateContinuationToken() const
{
    return deploymentName_;
}
