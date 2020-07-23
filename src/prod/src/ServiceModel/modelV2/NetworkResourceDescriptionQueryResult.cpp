//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(NetworkResourceDescriptionQueryResult)
INITIALIZE_SIZE_ESTIMATION(NetworkResourceDescriptionQueryResult::NetworkProperties)

NetworkResourceDescriptionQueryResult::NetworkProperties::NetworkProperties(
    FABRIC_NETWORK_TYPE networkType,
    wstring const & networkAddressPrefix,
    FABRIC_NETWORK_STATUS networkStatus)
    : networkType_(networkType)
    , networkAddressPrefix_(networkAddressPrefix)
    , networkStatus_(networkStatus)
{
}

NetworkResourceDescriptionQueryResult::NetworkResourceDescriptionQueryResult(
    wstring const & networkName,
    FABRIC_NETWORK_TYPE networkType,
    wstring const & networkAddressPrefix,
    FABRIC_NETWORK_STATUS networkStatus)
    : networkName_(networkName),
    properties_(networkType, networkAddressPrefix, networkStatus)
{
}
