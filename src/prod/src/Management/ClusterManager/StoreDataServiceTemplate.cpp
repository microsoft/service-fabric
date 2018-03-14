// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Store;
using namespace Management::ClusterManager;

StoreDataServiceTemplate::StoreDataServiceTemplate()
    : StoreData()
    , applicationId_()
    , serviceTypeName_() 
    , partitionedServiceDescriptor_()
{
}

StoreDataServiceTemplate::StoreDataServiceTemplate(
    ServiceModelApplicationId const & appId,
    ServiceModelTypeName const & typeName)
    : StoreData()
    , applicationId_(appId)
    , serviceTypeName_(typeName) 
{
}

StoreDataServiceTemplate::StoreDataServiceTemplate(
    ServiceModelApplicationId const & appId,
    NamingUri const & appName,
    ServiceModelTypeName const & typeName,
    Naming::PartitionedServiceDescriptor && descriptor)
    : StoreData()
    , applicationId_(appId)
    , applicationName_(appName)
    , serviceTypeName_(typeName) 
    , partitionedServiceDescriptor_(std::move(descriptor))
{
}

Common::ErrorCode StoreDataServiceTemplate::ReadServiceTemplates(
    StoreTransaction const & storeTx, 
    ServiceModelApplicationId const & appId, 
    __out std::vector<StoreDataServiceTemplate> & templates)
{
    return storeTx.ReadPrefix(Constants::StoreType_ServiceTemplate, ApplicationContext::GetKeyPrefix(appId), templates);
}

std::wstring const & StoreDataServiceTemplate::get_Type() const
{
    return Management::ClusterManager::Constants::StoreType_ServiceTemplate;
}

std::wstring StoreDataServiceTemplate::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        ApplicationContext::GetKeyPrefix(applicationId_),
        serviceTypeName_);
    return temp;
}

void StoreDataServiceTemplate::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("StoreDataServiceTemplate[{0}:{1} ({2})]", applicationId_, serviceTypeName_, partitionedServiceDescriptor_);
}
