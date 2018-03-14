// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::ClusterManager;


StoreDataApplicationIdMap::StoreDataApplicationIdMap(
    ServiceModelApplicationId const & appId)
    : StoreData()
    , appId_(appId)
    , appName_()
{
}

StoreDataApplicationIdMap::StoreDataApplicationIdMap(
    ServiceModelApplicationId const & appId, 
    Common::NamingUri const & appName)
    : StoreData()
    , appId_(appId)
    , appName_(appName)
{
}

wstring const & StoreDataApplicationIdMap::get_Type() const
{
    return Constants::StoreType_ApplicationIdMap;
}

wstring StoreDataApplicationIdMap::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}", appId_);
    return temp;
}

void StoreDataApplicationIdMap::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("ApplicationIdMap[{0} -> {1}]", appId_, appName_);
}
