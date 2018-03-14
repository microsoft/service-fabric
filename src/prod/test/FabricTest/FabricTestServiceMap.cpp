// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace std;
using namespace Naming;

class FabricTestServiceMap::TestServiceData
{
    DENY_COPY(TestServiceData);

public:
    TestServiceData(PartitionedServiceDescriptor && descriptor)
        : descriptor_(move(descriptor)), isDeleted_(false), blockList_()
    {
    }

    __declspec (property(get=get_ServiceDescription, put=set_ServiceDescription)) ServiceDescription const& Description;
    ServiceDescription const& get_ServiceDescription() const { return descriptor_.Service; }
    void set_ServiceDescription(ServiceDescription const& value) { descriptor_.MutableService = value; }

    __declspec (property(get=get_ServiceDescriptor, put=set_ServiceDescriptor)) PartitionedServiceDescriptor const& ServiceDescriptor;
    PartitionedServiceDescriptor const& get_ServiceDescriptor() const { return descriptor_; }
    void set_ServiceDescriptor(PartitionedServiceDescriptor const& value) { descriptor_ = value; }

    __declspec (property(get=get_IsDeleted, put=set_IsDeleted)) bool IsDeleted;
    bool get_IsDeleted() const { return isDeleted_; }
    void set_IsDeleted(bool isDeleted) { isDeleted_ = isDeleted; }

    __declspec (property(get=get_BlockListSize)) size_t BlockListSize;
    size_t get_BlockListSize() const { return blockList_.size(); }

    void SetBlockList(vector<NodeId> && blockList) { blockList_ = move(blockList); }

    bool IsBlockedNode(NodeId nodeId) const
    { 
        auto iterator = find(blockList_.begin(), blockList_.end(), nodeId); 
        return iterator != blockList_.end();
    }

private:
    bool isDeleted_;
    vector<NodeId> blockList_;
    PartitionedServiceDescriptor descriptor_;
};

void FabricTestServiceMap::AddPartitionedServiceDescriptor(wstring const& name, PartitionedServiceDescriptor && partitionedServiceDescriptor, bool overWriteExisting)
{
    AcquireWriteLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(name);
    TestSession::FailTestIf(!overWriteExisting && iterator != serviceDataMap_.end() && !iterator->second->IsDeleted, "Service with name {0} already exists", name);
    serviceDataMap_[name] = make_shared<TestServiceData>(move(partitionedServiceDescriptor));
    vector<NodeId> blockList;
    FABRICSESSION.FabricDispatcher.Federation.GetBlockListForService(serviceDataMap_[name]->Description.PlacementConstraints, blockList);
    serviceDataMap_[name]->SetBlockList(move(blockList));
    FABRICSESSION.FabricDispatcher.ResetTestStoreClient(name);
}   

void FabricTestServiceMap::UpdatePartitionedServiceDescriptor(wstring const& serviceName, Reliability::ServiceDescription const & serviceDescription)
{
    AcquireWriteLock grab(serviceDataMapLock_);

    auto it = serviceDataMap_.find(serviceName);

    TestSession::FailTestIf(it == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);

    it->second->Description = serviceDescription;
}

void FabricTestServiceMap::UpdatePartitionedServiceDescriptor(wstring const& serviceName, Naming::PartitionedServiceDescriptor && psd)
{
    AcquireWriteLock grab(serviceDataMapLock_);

    auto it = serviceDataMap_.find(serviceName);

    TestSession::FailTestIf(it == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);

    it->second->ServiceDescriptor = psd;
}

void FabricTestServiceMap::MarkServiceAsDeleted(wstring const& name, bool verifyExists)
{
    AcquireWriteLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(name);

    if (iterator == serviceDataMap_.end())
    {
        TestSession::FailTestIf(verifyExists, "Service with name {0} does not exist", name);
    }
    else
    {
        (*iterator).second->IsDeleted = true;
    }
}

bool FabricTestServiceMap::IsCreatedService(wstring const& name)
{
    AcquireReadLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(name);
    if(iterator != serviceDataMap_.end())
    {
        return !(*iterator).second->IsDeleted;
    }

    return false;
}

bool FabricTestServiceMap::IsDeletedService(wstring const& name)
{
    AcquireReadLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(name);
    if(iterator != serviceDataMap_.end())
    {
        return (*iterator).second->IsDeleted;
    }

    return false;
}
int FabricTestServiceMap::GetTargetReplicaForService(wstring const& name)
{
    AcquireWriteLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(name);
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", name);
    return (*iterator).second->Description.TargetReplicaSetSize;
}

int FabricTestServiceMap::GetMinReplicaSetSizeForService(wstring const& serviceName)
{
    AcquireReadLock lock(serviceDataMapLock_);

    auto it = serviceDataMap_.find(serviceName);
    TestSession::FailTestIf(it == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);
    return it->second->Description.MinReplicaSetSize;
}

size_t FabricTestServiceMap::GetBlockListSize(wstring const& serviceName)
{
    AcquireReadLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(serviceName);
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);
    TestServiceDataSPtr const& testServiceDataSPtr = (*iterator).second;
    return testServiceDataSPtr->BlockListSize;
}

void FabricTestServiceMap::GetCreatedServiceNames(std::vector<std::wstring> & names)
{
    AcquireReadLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++ )
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if(!testServiceDataSPtr->IsDeleted)
        {
            names.push_back(testServiceDataSPtr->Description.Name);
        }
    }
}

void FabricTestServiceMap::GetDeletedServiceNames(std::vector<std::wstring> & names)
{
    AcquireReadLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++ )
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if(testServiceDataSPtr->IsDeleted)
        {
            names.push_back(testServiceDataSPtr->Description.Name);
        }
    }
}

void FabricTestServiceMap::GetCreatedServicesDescription(vector<ServiceDescription> & serviceDescriptions)
{
    AcquireReadLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++ )
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if(!testServiceDataSPtr->IsDeleted)
        {
            serviceDescriptions.push_back(testServiceDataSPtr->Description);
        }
    }
}

bool FabricTestServiceMap::IsStatefulService(wstring const& name)
{
    // Do not remove this conversion to NamingUri. This is used when the service name is a service group
    // to ensure the segments are removed before checking. Also if the name is a system service it cannot be a 
    // naming Uri hence the fallback to original name
    Common::NamingUri uri;
    wstring serviceName = name;
    if(Common::NamingUri::TryParse(name, uri))
    {
        serviceName = Common::NamingUri(uri.Path).ToString();
    }

    AcquireReadLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(serviceName);
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", name);
    return (*iterator).second->Description.IsStateful;
}

bool FabricTestServiceMap::HasPersistedState(wstring const& serviceName)
{
    AcquireReadLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(serviceName);
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);
    return (*iterator).second->Description.HasPersistedState;
}

void FabricTestServiceMap::GetAdhocStatefulServices(vector<wstring> & services)
{
    AcquireReadLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++ )
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if(!testServiceDataSPtr->IsDeleted && testServiceDataSPtr->Description.IsStateful && testServiceDataSPtr->Description.ApplicationName.empty())
        {
            services.push_back((*it).first);
        }
    }
}

wstring RemoveTrailingSlash(wstring const& uri)
{
    wstring slash(L"/");
    if(Common::StringUtility::EndsWith(uri, slash))
    {
        return uri.substr(0, uri.size() -1);
    }

    return uri;
}

ServiceModel::ServiceTypeIdentifier FabricTestServiceMap::GetServiceType(wstring const & appName, wstring const & typeName)
{
    AcquireReadLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++)
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if (StringUtility::CompareCaseInsensitive(RemoveTrailingSlash(testServiceDataSPtr->Description.ApplicationName), RemoveTrailingSlash(appName)) == 0 &&
            testServiceDataSPtr->Description.Type.ServiceTypeName == typeName)
        {
            return testServiceDataSPtr->Description.Type;
        }
    }

    TestSession::FailTest("Application {0} ServiceType {1} not found", appName, typeName);
}

void FabricTestServiceMap::MarkAppAsDeleted(wstring const& appName)
{
    AcquireWriteLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++ )
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if(StringUtility::CompareCaseInsensitive(RemoveTrailingSlash(testServiceDataSPtr->Description.ApplicationName), RemoveTrailingSlash(appName)) == 0)
        {
            testServiceDataSPtr->IsDeleted = true;
        }
    }
}

void FabricTestServiceMap::RecalculateBlockList()
{
    AcquireWriteLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++ )
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if(!testServiceDataSPtr->IsDeleted)
        {
            vector<NodeId> blockList;
            FABRICSESSION.FabricDispatcher.Federation.GetBlockListForService(testServiceDataSPtr->Description.PlacementConstraints, blockList);
            testServiceDataSPtr->SetBlockList(move(blockList));
        }
    }
}

bool FabricTestServiceMap::IsBlockedNode(wstring const& serviceName, NodeId nodeId)
{
    AcquireReadLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(serviceName);
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);
    TestServiceDataSPtr const& testServiceDataSPtr = (*iterator).second;
    if(!testServiceDataSPtr->IsDeleted)
    {
        return testServiceDataSPtr->IsBlockedNode(nodeId);
    }
    else
    {
        //Request is for a deleted service. We ignore this for now and the Verify command will validate that this
        //service should not be placed
        return false;
    }
}

Guid FabricTestServiceMap::ResolveFuidForKey(wstring const& serviceName, __int64 key)
{
    TestSession::FailTestIf(serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName, "GetFUIdAt for NamingService");
    TestSession::FailTestIf(serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName, "GetFUIdAt for CMService");
    TestSession::FailTestIf(serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName, "GetFUIdAt for FMService");

    AcquireReadLock grab(serviceDataMapLock_);

    Common::NamingUri uri;
    TestSession::FailTestIf(!NamingUri::TryParse(serviceName, uri), "Cannot parse service name {0}", serviceName);

    Common::NamingUri withoutFragment(uri.Path);

    auto iterator = serviceDataMap_.find(withoutFragment.ToString());
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);
    TestServiceDataSPtr const& testServiceDataSPtr = (*iterator).second;
    ConsistencyUnitId cuid;
    TestSession::FailTestIfNot(testServiceDataSPtr->ServiceDescriptor.TryGetCuid(PartitionKey(key), cuid), "Could not resolve key {0} to a CUID", key);
    return cuid.Guid;
}

Common::Guid FabricTestServiceMap::GetFUIdAt(wstring const& serviceName, int index)
{
    if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName)
    {
        return Reliability::Constants::FMServiceGuid;
    }
    else if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName)
    {
        auto const & range = *Reliability::ConsistencyUnitId::NamingIdRange;
        return Reliability::ConsistencyUnitId::CreateReserved(range, index).Guid;
    }
    else if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName)
    {
        auto const & range = *Reliability::ConsistencyUnitId::CMIdRange;
        return Reliability::ConsistencyUnitId::CreateReserved(range, index).Guid;
    }

    AcquireReadLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(serviceName);
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);
    TestServiceDataSPtr const& testServiceDataSPtr = (*iterator).second;
    TestSession::FailTestIf(testServiceDataSPtr->ServiceDescriptor.Cuids.size() < static_cast<size_t>(index + 1), "FailoverUnit not found at index {0}", index);
    return testServiceDataSPtr->ServiceDescriptor.Cuids[index].Guid;
}

vector<wstring> FabricTestServiceMap::GetReportMetricNames(wstring const& serviceName)
{
    AcquireReadLock grab(serviceDataMapLock_);
    vector<wstring> metricNames;
    auto iterator = serviceDataMap_.find(serviceName);
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);
    TestServiceDataSPtr const& testServiceDataSPtr = (*iterator).second;

    ServiceDescription const& sd = testServiceDataSPtr->Description;

    // hjzeng TODO:  revisit this

    for (auto const& metric : sd.Metrics)
    {
        metricNames.push_back(metric.Name);
    }

    return metricNames;
}

Naming::PartitionedServiceDescriptor FabricTestServiceMap::GetServiceDescriptor(wstring const& serviceName)
{
    AcquireReadLock grab(serviceDataMapLock_);
    auto iterator = serviceDataMap_.find(ServiceModel::SystemServiceApplicationNameHelper::GetInternalServiceName(serviceName));
    TestSession::FailTestIf(iterator == serviceDataMap_.end(), "Service with name {0} does not exist", serviceName);
    return (*iterator).second->ServiceDescriptor;
}

void FabricTestServiceMap::GetAllFUs(vector<Guid> & fus)
{
    AcquireReadLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++ )
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if(testServiceDataSPtr->Description.Name.compare(ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName) != 0 &&
            testServiceDataSPtr->Description.Name.compare(ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName))
        {
            if(!testServiceDataSPtr->IsDeleted)
            {
                for (auto it2 = testServiceDataSPtr->ServiceDescriptor.Cuids.begin() ; it2 != testServiceDataSPtr->ServiceDescriptor.Cuids.end(); it2++ )
                {
                    Reliability::ConsistencyUnitId const& cuid = *it2;
                    fus.push_back(cuid.Guid);
                }
            }
        }
    }
}

void FabricTestServiceMap::GetAllFUsForService(wstring const& serviceName, vector<Guid> & fus)
{
    AcquireReadLock grab(serviceDataMapLock_);
    for (auto it = serviceDataMap_.begin() ; it != serviceDataMap_.end(); it++ )
    {
        TestServiceDataSPtr const& testServiceDataSPtr = (*it).second;
        if(StringUtility::CompareCaseInsensitive(testServiceDataSPtr->Description.Name, serviceName) == 0)
        {
            for (auto it2 = testServiceDataSPtr->ServiceDescriptor.Cuids.begin() ; it2 != testServiceDataSPtr->ServiceDescriptor.Cuids.end(); it2++ )
            {
                Reliability::ConsistencyUnitId const& cuid = *it2;
                fus.push_back(cuid.Guid);
            }
        }
    }
}
