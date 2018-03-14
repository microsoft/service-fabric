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

StoreDataServicePackage::StoreDataServicePackage()
    : StoreData()
    , applicationId_()
    , applicationName_()
    , typeName_() 
    , packageName_() 
    , packageVersion_()
    , typeDescription_()
{
}

StoreDataServicePackage::StoreDataServicePackage(
    ServiceModelApplicationId const & appId,
    ServiceModelTypeName const & typeName)
    : StoreData()
    , applicationId_(appId)
    , applicationName_()
    , typeName_(typeName) 
    , packageName_() 
    , packageVersion_()
    , typeDescription_()
{
}

StoreDataServicePackage::StoreDataServicePackage(
    ServiceModelApplicationId const & appId,
    Common::NamingUri const & appName,
    ServiceModelTypeName const & typeName,
    ServiceModelPackageName const & packageName,
    ServiceModelVersion const & packageVersion,
    ServiceModel::ServiceTypeDescription const & typeDescription)
    : StoreData()
    , applicationId_(appId)
    , applicationName_(appName)
    , typeName_(typeName) 
    , packageName_(packageName) 
    , packageVersion_(packageVersion) 
    , typeDescription_(typeDescription)
{
}

Common::ErrorCode StoreDataServicePackage::ReadServicePackages(
    StoreTransaction const & storeTx, 
    ServiceModelApplicationId const & appId, 
    __out std::vector<StoreDataServicePackage> & packages)
{
    return storeTx.ReadPrefix(Constants::StoreType_ServicePackage, ApplicationContext::GetKeyPrefix(appId), packages);
}

std::wstring const & StoreDataServicePackage::get_Type() const
{
    return Management::ClusterManager::Constants::StoreType_ServicePackage;
}

std::wstring StoreDataServicePackage::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        ApplicationContext::GetKeyPrefix(applicationId_),
        typeName_);
    return temp;
}

void StoreDataServicePackage::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("StoreDataServicePackage[{0}({1}):{2}->({3}, {4})]", applicationId_, applicationName_, typeName_, packageName_, packageVersion_);
}
