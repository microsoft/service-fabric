//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ContainerServiceQueryResult)

ContainerServiceQueryResult::ContainerServiceQueryResult(
    std::wstring const &serviceUri,
    std::wstring const &applicationUri,
    ContainerServiceDescription const &base,
    FABRIC_QUERY_SERVICE_STATUS status)
    : ContainerServiceDescription(base)
    , ServiceUri(serviceUri)
    , ApplicationUri(applicationUri)
{
    Properties.Status = status;
}

wstring ContainerServiceQueryResult::CreateContinuationToken() const
{
    return ServiceUri;
}
