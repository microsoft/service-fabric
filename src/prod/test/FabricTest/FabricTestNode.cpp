// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Federation;
using namespace std;
using namespace Transport;
using namespace Common;
using namespace ServiceModel;
using namespace Fabric;
using namespace TestCommon;
using namespace FederationTestCommon;
using namespace Reliability;
using namespace Naming;
using namespace Hosting2;
using namespace Management::ImageModel;

const StringLiteral TraceSource("FabricTest.Node");

class FabricTestNode::MultiCodePackageHostData
{
public:
    MultiCodePackageHostData() : toBeKilled_(false){}

    __declspec (property(get = get_Context, put = put_Context)) TestMultiPackageHostContext const& Context;
    TestMultiPackageHostContext const& get_Context() const { return context_; }
    void put_Context(TestMultiPackageHostContext const& value) { context_ = value; }

    __declspec (property(get = get_ToBeKilled, put = put_ToBeKilled)) bool ToBeKilled;
    bool get_ToBeKilled() const { return toBeKilled_; }
    void put_ToBeKilled(bool value) { toBeKilled_ = value; }

    __declspec (property(get = get_CodePackages)) vector<wstring> & CodePackages;
    vector<wstring> & get_CodePackages() { return codePackages_; }

private:

    TestMultiPackageHostContext context_;
    bool toBeKilled_;
    vector<wstring> codePackages_;
};


FabricTestNode::FabricTestNode(
    NodeId nodeId,
    FabricNodeConfigOverride const& configOverride,
    FabricNodeConfig::NodePropertyCollectionMap const & nodeProperties,
    FabricNodeConfig::NodeCapacityCollectionMap const & nodeCapacities,
    std::vector<std::wstring> const & nodeCredentials,
    std::vector<std::wstring> const & clusterCredentials,
    bool checkCert)
    : servicesFromGFUM_(),
    nodeWrapper_(),
    isActivated_(true)
{
    FabricVersionInstance fabVerInst = FABRICSESSION.FabricDispatcher.UpgradeFabricData.GetFabricVersionInstance(nodeId);
    nodeWrapper_ = make_shared<FabricNodeWrapper>(
        fabVerInst,
        nodeId,
        configOverride,
        nodeProperties,
        nodeCapacities,
        nodeCredentials,
        clusterCredentials,
        checkCert);
}

FabricTestNode::~FabricTestNode()
{
}

void FabricTestNode::Open(TimeSpan openTimeout, bool expectNodeFailure, bool addRuntime, ErrorCodeValue::Enum openErrorCode, vector<ErrorCodeValue::Enum> const& retryErrors)
{
    nodeWrapper_->Open(openTimeout, expectNodeFailure, addRuntime, openErrorCode, retryErrors);
}


void FabricTestNode::Close(bool removeData)
{
    nodeWrapper_->Close(removeData);
}

void FabricTestNode::Abort()
{
    nodeWrapper_->Abort();
}

void FabricTestNode::RegisterServiceForVerification(TestServiceInfo const& serviceInfo)
{
    bool isInfraServiceType =
        (FABRICSESSION.FabricDispatcher.ISDescriptions.size() > 0) &&
        (FABRICSESSION.FabricDispatcher.ISDescriptions[0].Type.QualifiedServiceTypeName == serviceInfo.ServiceType);

    if (serviceInfo.ServiceType != SGServiceFactory::SGStatefulServiceGroupType &&
        serviceInfo.ServiceType != SGServiceFactory::SGStatelessServiceGroupType &&
        serviceInfo.ServiceType != FABRICSESSION.FabricDispatcher.CMDescription.Type.ServiceTypeName &&
        serviceInfo.ServiceType != FABRICSESSION.FabricDispatcher.FSDescription.Type.ServiceTypeName &&
        serviceInfo.ServiceType != FABRICSESSION.FabricDispatcher.RMDescription.Type.QualifiedServiceTypeName &&
        !isInfraServiceType)
    {
        servicesFromGFUM_.push_back(serviceInfo);
    }
}

bool FabricTestNode::TryGetFabricNodeWrapper(shared_ptr<FabricNodeWrapper> & fabricNodeWrapper) const
{
    fabricNodeWrapper = dynamic_pointer_cast<FabricNodeWrapper>(nodeWrapper_);
    return fabricNodeWrapper != nullptr;
}

bool FabricTestNode::VerifyAllServices() const
{
    if (!nodeWrapper_->IsOpened)
    {
        return false;
    }

    bool result = true;

    //Verify NamingService placement
    shared_ptr<FabricNodeWrapper> fabricNodeWrapper;
    if (TryGetFabricNodeWrapper(fabricNodeWrapper))
    {
        auto activeNamingServices = fabricNodeWrapper->Node->Test_Communication.NamingService.Test_ActiveServices;
        result = VerifyNamingServicePlacement(servicesFromGFUM_, activeNamingServices);
        if (!result) { return result; }
    }

    //Verify local service placement
    auto activeLocalServicesCopy = RuntimeManager->ServiceFactory->ActiveServicesCopy;
    result = VerifyServicePlacement(servicesFromGFUM_, activeLocalServicesCopy);
    if (!result) { return result; }

    {
        AcquireExclusiveLock grab(activeHostsLock_);
        for (auto iter = activeCodePackages_.begin(); iter != activeCodePackages_.end(); iter++)
        {
            auto const& codePackageData = iter->second;
            if (codePackageData.ToBeKilled)
            {
                TestSession::WriteInfo(TraceSource, "CodePackage {0} is marked as ToBeKilled. Wait for it to be restarted", codePackageData.Context);
                return false;
            }

            if (codePackageData.PlacedServices.size() > 0)
            {
                //Verify remote service placement
                result = VerifyServicePlacement(servicesFromGFUM_, codePackageData.PlacedServices);
                if (!result)
                {
                    FABRICSESSION.ExecuteCommandAtAllClients(FabricTestCommands::GetPlacementDataCommand);
                    return result;
                }

                ServiceManifestDescription serviceManifest;
                wstring appName;
                if (FABRICSESSION.FabricDispatcher.ApplicationMap.TryGetAppName(codePackageData.Context.CodePackageId.ServicePackageInstanceId.ApplicationId, appName))
                {
                    if (ApplicationBuilder::TryGetServiceManifestDescription(
                        FABRICSESSION.FabricDispatcher.ApplicationMap.GetCurrentApplicationBuilderName(appName),
                        codePackageData.Context.CodePackageId.ServicePackageInstanceId.ServicePackageName,
                        serviceManifest))
                    {
                        if (serviceManifest != codePackageData.Description)
                        {
                            TestSession::WriteInfo(TraceSource, "ServiceManifest do not match for context:{0}. \nExpected {1}, \nActual {2}", codePackageData.Context, serviceManifest, codePackageData.Description);
                            return false;
                        }

                        bool foundCodePackage = false;
                        for (auto cpIter = serviceManifest.CodePackages.begin(); cpIter != serviceManifest.CodePackages.end(); cpIter++)
                        {
                            if ((*cpIter).Name == codePackageData.Context.CodePackageId.CodePackageName)
                            {
                                foundCodePackage = true;

                                if ((*cpIter).Version != codePackageData.Context.CodePackageVersion)
                                {
                                    TestSession::WriteInfo(TraceSource, "CodePack versions do not match for context:{0}. Expected {1}, Actual {2}", codePackageData.Context, (*cpIter).Version, codePackageData.Context.CodePackageVersion);
                                    return false;
                                }
                            }
                        }

                        if (!foundCodePackage)
                        {
                            TestSession::WriteInfo(TraceSource, "CodePackage {0} not found", codePackageData.Context.CodePackageId.CodePackageName);
                            return false;
                        }

                    }
                    else
                    {
                        TestSession::WriteInfo(TraceSource, "ServicePackage {0} has been deleted. Waiting for CodePackage {1} to be deactivated", codePackageData.Context.CodePackageId.ServicePackageInstanceId.ServicePackageName, codePackageData.Context.CodePackageId);
                        return false;
                    }
                }
                else
                {
                    TestSession::WriteInfo(TraceSource, "App {0} has been deleted. Waiting for CodePackage {1} to be deactivated", codePackageData.Context.CodePackageId.ServicePackageInstanceId.ApplicationId, codePackageData.Context.CodePackageId);
                    return false;
                }
            }
        }
    }

    //Validate servicesFromGFUM_ is empty
    if (servicesFromGFUM_.size() != 0)
    {
        TestSession::WriteInfo(TraceSource, "Not all GFUM services found at node {0}. Services not found are", Id);
        for (auto iter = servicesFromGFUM_.begin(); iter != servicesFromGFUM_.end(); iter++)
        {
            TestServiceInfo const& serviceInfo = *iter;
            TestSession::WriteInfo(TraceSource, "{0}", serviceInfo);
        }

        AcquireExclusiveLock grab(activeHostsLock_);
        for (auto iter = activeCodePackages_.begin(); iter != activeCodePackages_.end(); iter++)
        {
            wstring clientId = iter->second.Context.ToClientId();
            FABRICSESSION.ExecuteCommandAtClient(FabricTestCommands::GetPlacementDataCommand, clientId);
        }
        return false;
    }

    return result;
}

void FabricTestNode::ClearServicePlacementList()
{
    servicesFromGFUM_.clear();
}

void FabricTestNode::PrintActiveServices() const
{
    wstring result;
    StringWriter writer(result);
    writer.WriteLine("Node:{0}: ", nodeWrapper_->Id);
    writer.WriteLine("Local Services:");
    writer.WriteLine("{0}", *RuntimeManager->ServiceFactory);

    writer.WriteLine("Remote Services:");
    {
        AcquireExclusiveLock grab(activeHostsLock_);
        for (auto iter = activeCodePackages_.begin(); iter != activeCodePackages_.end(); iter++)
        {
            //Verify remote service placement
            writer.WriteLine("{0}", iter->second.Context, iter->second.PlacedServices);
            for (auto iter1 = iter->second.PlacedServices.begin(); iter1 != iter->second.PlacedServices.end(); iter1++)
            {
                writer.WriteLine("{0}", iter1->second);
            }

            writer.WriteLine();
            //send command to refresh data
            wstring clientId = iter->second.Context.ToClientId();
            FABRICSESSION.ExecuteCommandAtClient(FabricTestCommands::GetPlacementDataCommand, clientId);
        }
    }

    TestSession::WriteInfo(TraceSource, "{0}", result);
}

bool FabricTestNode::TryFindStoreService(std::wstring const &serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr)
{
    bool result = RuntimeManager->ServiceFactory->TryFindStoreService(serviceLocation, storeServiceSPtr);
    if (!result)
    {
        AcquireExclusiveLock grab(activeHostsLock_);
        for (auto iter = activeCodePackages_.begin(); iter != activeCodePackages_.end(); iter++)
        {
            auto storeServiceIter = iter->second.StoreServices.find(serviceLocation);
            if (storeServiceIter != iter->second.StoreServices.end())
            {
                storeServiceSPtr = storeServiceIter->second;
                result = true;
                break;
            }
        }
    }

    return result;
}

bool FabricTestNode::TryFindStoreService(std::wstring const & serviceName, NodeId const & nodeId, ITestStoreServiceSPtr & storeServiceSPtr)
{
    return RuntimeManager->ServiceFactory->TryFindStoreService(serviceName, nodeId, storeServiceSPtr);
}

bool FabricTestNode::TryFindCalculatorService(std::wstring const & serviceName, NodeId const & nodeId, CalculatorServiceSPtr & calculatorServiceSPtr)
{
    return RuntimeManager->ServiceFactory->TryFindCalculatorService(serviceName, nodeId, calculatorServiceSPtr);
}

bool FabricTestNode::TryFindScriptableService(__in std::wstring const & serviceLocation, __out ITestScriptableServiceSPtr & scriptableServiceSPtr)
{
    return RuntimeManager->ServiceFactory->TryFindScriptableService(serviceLocation, scriptableServiceSPtr);
}

bool FabricTestNode::TryFindScriptableService(__in std::wstring const & serviceName, Federation::NodeId const & nodeId, __out ITestScriptableServiceSPtr & scriptableServiceSPtr)
{
    return RuntimeManager->ServiceFactory->TryFindScriptableService(serviceName, nodeId, scriptableServiceSPtr);
}

void FabricTestNode::ProcessServicePlacementData(ServicePlacementData const& placementData)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    wstring key = placementData.CodePackageContext.CodePackageId.ToString();
    auto iter = activeCodePackages_.find(key);
    if (iter != activeCodePackages_.end())
    {
        TestSession::FailTestIf(placementData.CodePackageContext.InstanceId > iter->second.Context.InstanceId, "ProcessServicePlacementData received for a newer instance than we know of: Received: {0}; Current: {1}", placementData.CodePackageContext, iter->second.Context);
        if (placementData.CodePackageContext.InstanceId < iter->second.Context.InstanceId)
        {
            TestSession::WriteNoise(TraceSource, "ProcessServicePlacementData from stale context {0}", placementData.CodePackageContext);
        }
        else
        {
            TestSession::WriteNoise(TraceSource, "ProcessServicePlacementData from context {0}", placementData.CodePackageContext);
            map<wstring, ITestStoreServiceSPtr> storeServices;
            for (auto iter1 = placementData.PlacedServices.begin(); iter1 != placementData.PlacedServices.end(); iter1++)
            {
                auto storeServiceIter = iter->second.StoreServices.find(iter1->first);
                if (storeServiceIter != iter->second.StoreServices.end())
                {
                    storeServices.insert(make_pair(storeServiceIter->first, storeServiceIter->second));
                }
                else
                {
                    ITestStoreServiceSPtr storeService = make_shared<TestStoreServiceProxy>(
                        placementData.CodePackageContext.ToClientId(),
                        iter1->second.ServiceLocation,
                        iter1->second.PartitionId,
                        iter1->second.ServiceName,
                        Id);
                    storeServices.insert(make_pair(iter1->second.ServiceLocation, storeService));
                }
            }

            iter->second.PlacedServices = placementData.PlacedServices;
            iter->second.StoreServices = move(storeServices);
        }
    }
    else
    {
        //Must be a stale message. Ignore
        TestSession::WriteNoise(TraceSource, "ProcessServicePlacementData from non existent context {0}", placementData.CodePackageContext);
    }
}

void FabricTestNode::ProcessUpdateServiceManifest(TestCodePackageData const& codePackageData)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    wstring key = codePackageData.CodePackageContext.CodePackageId.ToString();
    auto iter = activeCodePackages_.find(key);
    if (iter != activeCodePackages_.end())
    {
        TestSession::FailTestIf(codePackageData.CodePackageContext.InstanceId > iter->second.Context.InstanceId, "ProcessUpdateServiceManifest received for a newer instance than we know of: Received: {0}; Current: {1}", codePackageData.CodePackageContext, iter->second.Context);
        if (codePackageData.CodePackageContext.InstanceId < iter->second.Context.InstanceId)
        {
            TestSession::WriteNoise(TraceSource, "ProcessUpdateServiceManifest from stale context {0}", codePackageData.CodePackageContext);
        }
        else
        {
            TestSession::WriteNoise(TraceSource, "ProcessUpdateServiceManifest from context {0} with existing ServiceManifest version {1}, new version {2}", codePackageData.CodePackageContext, iter->second.Description.Version, codePackageData.Description.Version);
            iter->second.Description = codePackageData.Description;
        }
    }
    else
    {
        //Must be a stale message. Ignore
        TestSession::WriteNoise(TraceSource, "ProcessUpdateServiceManifest from non existent context {0}", codePackageData.CodePackageContext);
    }
}

void FabricTestNode::ProcessCodePackageHostInit(TestCodePackageContext const& context)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    wstring key = context.CodePackageId.ToString();
    auto iter = activeCodePackages_.find(key);
    if (iter != activeCodePackages_.end())
    {
        TestSession::FailTestIf(iter->second.Context.InstanceId == context.InstanceId, "ProcessCodePackageHostInit for same context {0}", context);
        TestSession::FailTestIf(context.InstanceId < iter->second.Context.InstanceId, "Stale ProcessCodePackageHostInit from {0} unexpected. Current is: {1}", context, iter->second.Context);
        //This means previous code package has failed. Check whether it is expected 
        ProcessCodePackageHostFailureInternal(context);

        //Previous failure was expected. Continue to register new code package
    }

    if (context.IsMultiHost)
    {
        auto multiCodeHostIter = activeMultiCodePackageHosts_.find(context.MultiPackageHostContext.HostId);
        TestSession::FailTestIf(multiCodeHostIter == activeMultiCodePackageHosts_.end(), "ProcessCodePackageHostInit for invalid multi host context {0}", context);
        multiCodeHostIter->second.CodePackages.push_back(key);
    }

    CodePackageData codePackageData;
    codePackageData.Context = context;
    activeCodePackages_[key] = move(codePackageData);
    TestSession::WriteNoise(TraceSource, "ProcessCodePackageHostInit from context {0}", context);
}

void FabricTestNode::ProcessCodePackageHostFailure(TestCodePackageContext const& context)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    ProcessCodePackageHostFailureInternal(context);
}

void FabricTestNode::ProcessCodePackageHostFailureInternal(TestCodePackageContext const& context)
{
    wstring key = context.CodePackageId.ToString();
    auto iter = activeCodePackages_.find(key);

    if (iter != activeCodePackages_.end() && context.InstanceId >= iter->second.Context.InstanceId)
    {
        CodePackageData & codePackageData = iter->second;
        if (codePackageData.PlacedServices.size() > 0)
        {
            bool forceRestart;
            bool failureExpectedDueToForceRestartUpgrade =
                FABRICSESSION.FabricDispatcher.ApplicationMap.IsUpgradeOrRollbackPending(context.CodePackageId.ServicePackageInstanceId.ApplicationId, forceRestart)
                && forceRestart;
            if (!failureExpectedDueToForceRestartUpgrade && !FabricTestSessionConfig::GetConfig().AllowHostFailureOnUpgrade)
            {
                ServiceManifestDescription serviceManifest;
                wstring appName;
                if (FABRICSESSION.FabricDispatcher.ApplicationMap.TryGetAppName(context.CodePackageId.ServicePackageInstanceId.ApplicationId, appName) &&
                    !FABRICSESSION.FabricDispatcher.ApplicationMap.IsDeleted(appName))
                {
                    if (ApplicationBuilder::TryGetServiceManifestDescription(
                        FABRICSESSION.FabricDispatcher.ApplicationMap.GetCurrentApplicationBuilderName(appName),
                        context.CodePackageId.ServicePackageInstanceId.ServicePackageName,
                        serviceManifest))
                    {
                        if (serviceManifest.ServiceTypeNames.size() == iter->second.Description.ServiceTypeNames.size())
                        {
                            for (size_t i = 0; i < serviceManifest.CodePackages.size(); ++i)
                            {
                                if (serviceManifest.CodePackages[i].Name == iter->second.Context.CodePackageId.CodePackageName)
                                {
                                    if (iter->second.Context.CodePackageVersion == serviceManifest.CodePackages[i].Version)
                                    {
                                        if (!iter->second.ToBeKilled)
                                        {
                                            bool placedServiceNotDeleted = false;
                                            for (auto serviceIter = codePackageData.PlacedServices.begin(); serviceIter != codePackageData.PlacedServices.end(); ++serviceIter)
                                            {
                                                TestServiceInfo const& serviceInfo = serviceIter->second;
                                                placedServiceNotDeleted = FABRICSESSION.FabricDispatcher.ServiceMap.IsCreatedService(serviceInfo.ServiceName);
                                            }

                                            TestSession::FailTestIf(placedServiceNotDeleted, "ProcessCodePackageHostFailure for unexpected failure {0}", context);
                                        }
                                    }
                                    else
                                    {
                                        // Versions are not equal so this kill must be part of an upgrade.
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Service manifest types have changes and it is a major version upgrade hence all processes are expected to restart
                        }

                        // If code package not found then it must have been removed hence failure is expected

                    }
                    else
                    {
                        // Service package must have been deleted. 
                    }
                }
                else
                {
                    // AppName not found hence this app app must have been deleted. Ignore this as it might be part of cleanup
                }
            }
        }
        else
        {
            // No placed services hence killing the code package is expected
        }

        TestSession::WriteNoise(TraceSource, "ProcessCodePackageHostFailure for expected failure for {0}", context);
        if (iter->second.Context.IsMultiHost)
        {
            auto multiCodeHostIter = activeMultiCodePackageHosts_.find(iter->second.Context.MultiPackageHostContext.HostId);
            if (multiCodeHostIter != activeMultiCodePackageHosts_.end())
            {
                bool found = false;
                for (auto codePackageIdIter = multiCodeHostIter->second.CodePackages.begin(); codePackageIdIter != multiCodeHostIter->second.CodePackages.end(); ++codePackageIdIter)
                {
                    if (*codePackageIdIter == key)
                    {
                        multiCodeHostIter->second.CodePackages.erase(codePackageIdIter);
                        found = true;
                        break;
                    }
                }
                TestSession::FailTestIfNot(found, "CodePckage {0} not found in multi host", iter->second.Context);
            }
            else
            {
                TestSession::WriteNoise(TraceSource, "ProcessCodePackageHostFailure for invalid multi host context for {0}", context);
            }
        }

        activeCodePackages_.erase(iter);
    }
    else
    {
        TestSession::WriteNoise(TraceSource, "ProcessCodePackageHostFailure for stale context {0}", context);
    }
}

void FabricTestNode::ProcessMultiCodePackageHostInit(TestMultiPackageHostContext const& context)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    auto iter = activeMultiCodePackageHosts_.find(context.HostId);
    TestSession::FailTestIfNot(iter == activeMultiCodePackageHosts_.end(), "ProcessMultiCodePackageHostInit for already existing context {0}", context);
    MultiCodePackageHostData multiCodePackageHostData;
    multiCodePackageHostData.Context = context;
    activeMultiCodePackageHosts_[context.HostId] = move(multiCodePackageHostData);
    TestSession::WriteNoise(TraceSource, "ProcessMultiCodePackageHostInit from context {0}", context);
}

void FabricTestNode::ProcessMultiCodePackageHostFailure(TestMultiPackageHostContext const& context)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    auto iter = activeMultiCodePackageHosts_.find(context.HostId);
    if (iter != activeMultiCodePackageHosts_.end())
    {
        TestSession::FailTestIfNot(iter->second.ToBeKilled, "ProcessMultiCodePackageHostFailureInternal for unexpected failure {0}", context);
        TestSession::WriteNoise(TraceSource, "ProcessMultiCodePackageHostFailureInternal for expected failure for {0}", context);
        activeMultiCodePackageHosts_.erase(iter);
    }
    else
    {
        TestSession::WriteNoise(TraceSource, "ProcessMultiCodePackageHostFailure for stale context {0}", context);
    }
}

bool FabricTestNode::VerifyNamingServicePlacement(
    vector<TestServiceInfo> & activeGFUMServices,
    Naming::ActiveServiceMap const & activeNamingServices) const
{
    bool done = false;
    while (!done)
    {
        bool foundNamingService = false;
        for (auto iter = activeGFUMServices.begin(); iter != activeGFUMServices.end(); iter++)
        {
            TestServiceInfo const& serviceInfo = *iter;
            if (serviceInfo.ServiceType == ServiceModel::ServiceTypeIdentifier::NamingStoreServiceTypeId->ServiceTypeName)
            {
                foundNamingService = true;
                auto it = activeNamingServices.find(serviceInfo.ServiceLocation);
                if (it == activeNamingServices.end())
                {
                    TestSession::WriteInfo(TraceSource, "No Naming service {0} on node {1} exists at location {2}.",
                        serviceInfo.PartitionId,
                        Id,
                        serviceInfo.ServiceLocation);
                    return false;
                }

                auto serviceData = it->second;
                if (!StringUtility::AreEqualCaseInsensitive(serviceData.partitionId_, serviceInfo.PartitionId))
                {
                    TestSession::WriteInfo(TraceSource, "Naming service at location {0} has PartitionId {1}. At GFUM, PartitionId {2}",
                        it->first,
                        it->second,
                        serviceInfo.PartitionId);
                    return false;
                }

                // TODO: Also verify ReplicaId when that field is added to TestServiceInfo.h
                /*
                if (std::get<1>(serviceData) != serviceInfo->ReplicaId)
                {
                return false;
                }
                */

                if (serviceData.replicaRole_ != serviceInfo.StatefulServiceRole)
                {
                    TestSession::WriteInfo(TraceSource, "Naming service at location {0} has role {1}. At GFUM, role {2}",
                        it->first,
                        static_cast<int>(serviceData.replicaRole_),
                        static_cast<int>(serviceInfo.StatefulServiceRole));
                    return false;
                }

                if (!serviceInfo.IsStateful)
                {
                    TestSession::WriteInfo(TraceSource, "Naming service at location {0} is not stateful in GFUM entry.",
                        it->first);
                    return false;
                }

                activeGFUMServices.erase(iter);
                break;
            }
        }

        if (!foundNamingService)
        {
            done = true;
        }
    }

    return true;
}

bool FabricTestNode::VerifyServicePlacement(
    vector<TestServiceInfo> & activeGFUMServices,
    std::map<std::wstring, TestServiceInfo> const& placementData) const
{
    bool done = false;
    while (!done)
    {
        bool foundService = false;
        for (auto iter = activeGFUMServices.begin(); iter != activeGFUMServices.end(); iter++)
        {
            TestServiceInfo const& serviceInfo = *iter;
            auto it = placementData.find(serviceInfo.ServiceLocation);
            if (it != placementData.end())
            {
                foundService = true;
                TestServiceInfo const & factoryServiceInfo = (*it).second;
                if (!(factoryServiceInfo == serviceInfo))
                {
                    TestSession::WriteInfo(TraceSource, "Service Info mismatch at node {0}. GFUM {1}, Actual {2}", Id, serviceInfo, factoryServiceInfo);
                    return false;
                }

                activeGFUMServices.erase(iter);
                break;
            }
        }

        if (!foundService)
        {
            done = true;
        }
    }

    return true;
}

bool FabricTestNode::KillHostAt(wstring const& codePackageId)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    if (codePackageId == L"")
    {
        vector<wstring> multiCodePackageHosts;
        for (auto iter = activeMultiCodePackageHosts_.begin(); iter != activeMultiCodePackageHosts_.end(); ++iter)
        {
            if (!iter->second.ToBeKilled)
            {
                multiCodePackageHosts.push_back(iter->first);
            }
        }

        if (multiCodePackageHosts.size() == 0)
        {
            //No code package hosts to kill
            return true;
        }

        int index = Random().Next(static_cast<int>(multiCodePackageHosts.size()));
        auto iter = activeMultiCodePackageHosts_.find(multiCodePackageHosts[index]);
        iter->second.ToBeKilled = true;
        for (auto codePackageIdIter = iter->second.CodePackages.begin(); codePackageIdIter != iter->second.CodePackages.end(); ++codePackageIdIter)
        {
            auto codePackageIter = activeCodePackages_.find(*codePackageIdIter);
            TestSession::FailTestIf(codePackageIter == activeCodePackages_.end(), "code package {0} not found", *codePackageIdIter);
            codePackageIter->second.ToBeKilled = true;
        }

        wstring clientId = iter->second.Context.ToClientId();
        wstring command = FabricTestCommands::KillCodePackageCommand + L" killhost";
        FABRICSESSION.ExecuteCommandAtClient(command, clientId);
    }
    else
    {
        auto iter = activeCodePackages_.find(codePackageId);
        if (iter != activeCodePackages_.end())
        {
            if (iter->second.Context.IsMultiHost)
            {
                iter->second.ToBeKilled = true;
                auto multiHostIter = activeMultiCodePackageHosts_.find(iter->second.Context.MultiPackageHostContext.HostId);
                TestSession::FailTestIf(multiHostIter == activeMultiCodePackageHosts_.end(), "multi host package {0} not found", iter->second.Context.MultiPackageHostContext.HostId);
                if (multiHostIter->second.ToBeKilled)
                {
                    TestSession::WriteWarning(TraceSource, "multi host {0} already marked as to be killed", multiHostIter->second.Context);
                    return false;
                }

                multiHostIter->second.ToBeKilled = true;
                for (auto codePackageIdIter = multiHostIter->second.CodePackages.begin(); codePackageIdIter != multiHostIter->second.CodePackages.end(); ++codePackageIdIter)
                {
                    auto codePackageIter = activeCodePackages_.find(*codePackageIdIter);
                    TestSession::FailTestIf(codePackageIter == activeCodePackages_.end(), "code package {0} not found", *codePackageIdIter);
                    codePackageIter->second.ToBeKilled = true;
                }

                wstring clientId = iter->second.Context.ToClientId();
                wstring command = FabricTestCommands::KillCodePackageCommand + L" killhost";
                FABRICSESSION.ExecuteCommandAtClient(command, clientId);
            }
            else
            {
                TestSession::WriteWarning(TraceSource, "CodePackageId {0} does not belong to high density host", codePackageId);
                return false;
            }
        }
        else
        {
            TestSession::WriteWarning(TraceSource, "CodePackageId {0} not found on node {1}", codePackageId, Id);
            return false;
        }
    }

    return true;
}

void FabricTestNode::FailFastOnAllCodePackages()
{
    AcquireExclusiveLock grab(activeHostsLock_);
    vector<wstring> codePackages;
    for (auto iter = activeCodePackages_.begin(); iter != activeCodePackages_.end(); ++iter)
    {
        iter->second.ToBeKilled = true;
        wstring clientId = iter->second.Context.ToClientId();
        FABRICSESSION.ExecuteCommandAtClient(FabricTestCommands::FailFastCommand, clientId);
    }
}

bool FabricTestNode::MarkKillPending(std::wstring const& codePackageId)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    auto iter = activeCodePackages_.find(codePackageId);
    if (iter != activeCodePackages_.end())
    {
        if (!iter->second.ToBeKilled)
        {
            iter->second.ToBeKilled = true;
            return true;
        }
        else
        {
            TestSession::WriteWarning(TraceSource, "CodePackageId {0} already marked as to be killed", codePackageId, Id);
            return false;
        }
    }
    else
    {
        TestSession::WriteWarning(TraceSource, "CodePackageId {0} not found on node {1}", codePackageId, Id);
        return false;
    }

    return true;
}

bool FabricTestNode::KillCodePackage(wstring const& codePackageId)
{
    AcquireExclusiveLock grab(activeHostsLock_);
    if (codePackageId == L"")
    {
        vector<wstring> codePackages;
        for (auto iter = activeCodePackages_.begin(); iter != activeCodePackages_.end(); ++iter)
        {
            if (!iter->second.ToBeKilled)
            {
                codePackages.push_back(iter->first);
            }
        }

        if (codePackages.size() == 0)
        {
            //No code packages to kill
            return true;
        }

        int index = Random().Next(static_cast<int>(codePackages.size()));
        auto iter = activeCodePackages_.find(codePackages[index]);
        iter->second.ToBeKilled = true;
        //send kill command
        wstring clientId = iter->second.Context.ToClientId();
        wstring command = FabricTestCommands::KillCodePackageCommand + L" cp=" + iter->second.Context.CodePackageId.ToString();
        FABRICSESSION.ExecuteCommandAtClient(command, clientId);
    }
    else
    {
        auto iter = activeCodePackages_.find(codePackageId);
        if (iter != activeCodePackages_.end())
        {
            if (iter->second.ToBeKilled)
            {
                iter->second.ToBeKilled = true;
                //send kill command
                wstring clientId = iter->second.Context.ToClientId();
                wstring command = FabricTestCommands::KillCodePackageCommand + L" cp=" + codePackageId;
                FABRICSESSION.ExecuteCommandAtClient(command, clientId);
            }
            else
            {
                TestSession::WriteWarning(TraceSource, "CodePackageId {0} already marked as to be killed", codePackageId, Id);
                return false;
            }
        }
        else
        {
            TestSession::WriteWarning(TraceSource, "CodePackageId {0} not found on node {1}", codePackageId, Id);
            return false;
        }
    }

    return true;
}

void FabricTestNode::VerifyPackage(
    ApplicationIdentifier const & appId,
    wstring const & serviceManifestName,
    wstring const & packageName,
    wstring const & packageVersion,
    bool const isShared,
    set<wstring> const & imageStoreFiles)
{
    RunLayoutSpecification const & runLayout = nodeWrapper_->GetHostingSubsystem().RunLayout;
    StoreLayoutSpecification storeLayout;

    // The layout structre of code/config/data package folder are the same. Hence using GetCodePackageFolder for all
    wstring packagePath = runLayout.GetCodePackageFolder(appId.ToString(), serviceManifestName, packageName, packageVersion, isShared);
    TestSession::FailTestIfNot(Directory::Exists(packagePath), "{0} is NotFound", packagePath);

    auto packageFiles = Directory::GetFiles(packagePath, L"*", false, true);
    auto storeCodePackageFilePath = storeLayout.GetCodePackageFolder(
        appId.ApplicationTypeName,
        serviceManifestName,
        packageName,
        packageVersion);

    for (auto packageFile : packageFiles)
    {
        wstring filePath = Path::Combine(storeCodePackageFilePath, packageFile);
        TestSession::FailTestIf(imageStoreFiles.find(filePath) == imageStoreFiles.end(), "{0} is not found in ImageStore", filePath);
    }

    if (isShared)
    {
        StoreLayoutSpecification const & sharedLayout = nodeWrapper_->GetHostingSubsystem().SharedLayout;

        wstring sharedPackagePath = sharedLayout.GetCodePackageFolder(appId.ApplicationTypeName, serviceManifestName, packageName, packageVersion);
        TestSession::FailTestIfNot(Directory::Exists(sharedPackagePath), "{0} is NotFound", sharedPackagePath);
    }
}

void FabricTestNode::GetResourceUsageFromDeployedPackages(double & cpuUsage, double & memoryUsage)
{
    cpuUsage = 0;
    memoryUsage = 0;

    vector<Application2SPtr> deployedApplications;
    auto error = nodeWrapper_->GetHostingSubsystem().ApplicationManagerObj->GetApplications(deployedApplications);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetResourceUsage(): GetApplications failed with {0}", error);

    for (auto const & deployedApp : deployedApplications)
    {
        auto appRolloutVersion = deployedApp->CurrentVersion.RolloutVersionValue;

        if (appRolloutVersion == RolloutVersion::Invalid) { continue; }

        auto versionedApp = deployedApp->GetVersionedApplication();
        if (!versionedApp) { continue; }

        vector<ServicePackage2SPtr> deployedServicePackages;
        error = versionedApp->GetAllServicePackageInstances(deployedServicePackages);
        TestSession::FailTestIfNot(error.IsSuccess(), "GetResourceUsage(): GetAllServicePackageInstances failed with {0}", error);

        for (auto const & deployedServicePackage : deployedServicePackages)
        {
            if (nullptr != deployedServicePackage)
            {
                auto versionedServicePackage = deployedServicePackage->GetVersionedServicePackage();
                if (nullptr != versionedServicePackage)
                {
                    auto rgSettings = versionedServicePackage->PackageDescription.ResourceGovernanceDescription;
                    cpuUsage += rgSettings.CpuCores;
                    memoryUsage += rgSettings.MemoryInMB;
                }
            }
        }
    }
}

void FabricTestNode::VerifyDeployedServicePackages(vector<ServicePackage2SPtr> deployedServicePackages, set<wstring> const & imageStoreFiles)
{
    RunLayoutSpecification const & runLayout = nodeWrapper_->GetHostingSubsystem().RunLayout;
    StoreLayoutSpecification storeLayout;

    for (auto const & deployedServicePackage : deployedServicePackages)
    {
        auto servicePackageRolloutVersion = deployedServicePackage->CurrentVersionInstance.Version.RolloutVersionValue;
        if (servicePackageRolloutVersion == RolloutVersion::Invalid) { continue; }

        auto servicePackageFilePath = runLayout.GetServicePackageFile(deployedServicePackage->Id.ApplicationId.ToString(), deployedServicePackage->ServicePackageName, servicePackageRolloutVersion.ToString());
        TestSession::FailTestIfNot(File::Exists(servicePackageFilePath), "{0} is NotFound", servicePackageFilePath);

        auto storeServicePackageFilePath = storeLayout.GetServicePackageFile(
            deployedServicePackage->Id.ApplicationId.ApplicationTypeName,
            deployedServicePackage->Id.ApplicationId.ToString(),
            deployedServicePackage->ServicePackageName,
            servicePackageRolloutVersion.ToString());
        TestSession::FailTestIf(imageStoreFiles.find(storeServicePackageFilePath) == imageStoreFiles.end(), "{0} is not found in ImageStore", storeServicePackageFilePath);

        ServicePackageDescription packageDesc;
        auto error = Parser::ParseServicePackage(servicePackageFilePath, packageDesc);
        TestSession::FailTestIfNot(error.IsSuccess(), "ParseServicePackage failed with {0}. File: {1}", error, servicePackageFilePath);

        auto serviceManifestFilePath = runLayout.GetServiceManifestFile(deployedServicePackage->Id.ApplicationId.ToString(), deployedServicePackage->ServicePackageName, packageDesc.ManifestVersion);
        TestSession::FailTestIfNot(File::Exists(serviceManifestFilePath), "{0} is NotFound", serviceManifestFilePath);

        auto storeServiceManifestFilePath = storeLayout.GetServiceManifestFile(
            deployedServicePackage->Id.ApplicationId.ApplicationTypeName,
            deployedServicePackage->ServicePackageName,
            packageDesc.ManifestVersion);
        TestSession::FailTestIf(imageStoreFiles.find(storeServicePackageFilePath) == imageStoreFiles.end(), "{0} is not found in ImageStore", storeServicePackageFilePath);

        for (auto const & digestedCodePackage : packageDesc.DigestedCodePackages)
        {
            VerifyPackage(
                deployedServicePackage->Id.ApplicationId,
                packageDesc.ManifestName,
                digestedCodePackage.CodePackage.Name,
                digestedCodePackage.CodePackage.Version,
                digestedCodePackage.IsShared,
                imageStoreFiles);
        }

        for (auto const & digestedConfigPackage : packageDesc.DigestedConfigPackages)
        {
            VerifyPackage(
                deployedServicePackage->Id.ApplicationId,
                packageDesc.ManifestName,
                digestedConfigPackage.ConfigPackage.Name,
                digestedConfigPackage.ConfigPackage.Version,
                digestedConfigPackage.IsShared,
                imageStoreFiles);
        }

        for (auto const & digestedDataPackage : packageDesc.DigestedDataPackages)
        {
            VerifyPackage(
                deployedServicePackage->Id.ApplicationId,
                packageDesc.ManifestName,
                digestedDataPackage.DataPackage.Name,
                digestedDataPackage.DataPackage.Version,
                digestedDataPackage.IsShared,
                imageStoreFiles);
        }
    }
}

void FabricTestNode::VerifyApplicationInstanceAndSharedPackageFiles(set<wstring> const & imageStoreFiles)
{
    if (!this->IsActivated) { return; }

    vector<Application2SPtr> deployedApplications;
    auto error = nodeWrapper_->GetHostingSubsystem().ApplicationManagerObj->GetApplications(deployedApplications);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetApplications failed with {0}", error);

    RunLayoutSpecification const & runLayout = nodeWrapper_->GetHostingSubsystem().RunLayout;
    StoreLayoutSpecification storeLayout;

    for (auto const & deployedApp : deployedApplications)
    {
        auto appRolloutVersion = deployedApp->CurrentVersion.RolloutVersionValue;

        if (appRolloutVersion == RolloutVersion::Invalid) { continue; }

        auto appPackageFilePath = runLayout.GetApplicationPackageFile(deployedApp->Id.ToString(), appRolloutVersion.ToString());
        TestSession::FailTestIfNot(File::Exists(appPackageFilePath), "{0} is NotFound", appPackageFilePath);

        auto storeAppPackageFilePath = storeLayout.GetApplicationPackageFile(deployedApp->Id.ApplicationTypeName, deployedApp->Id.ToString(), appRolloutVersion.ToString());
        TestSession::FailTestIf(imageStoreFiles.find(storeAppPackageFilePath) == imageStoreFiles.end(), "{0} is not found in ImageStore", storeAppPackageFilePath);

        auto versionedApp = deployedApp->GetVersionedApplication();
        if (!versionedApp) { continue; }

        vector<ServicePackage2SPtr> deployedServicePackages;
        error = versionedApp->GetAllServicePackageInstances(deployedServicePackages);
        TestSession::FailTestIfNot(error.IsSuccess(), "GetAllServicePackageInstances failed with {0}", error);

        VerifyDeployedServicePackages(deployedServicePackages, imageStoreFiles);
    }
}

void FabricTestNode::VerifyImageCacheFiles(set<wstring> const & imageStoreFiles)
{
    if (!this->IsActivated) { return; }

    wstring imageCacheFolder = nodeWrapper_->GetHostingSubsystem().ImageCacheFolder;

    // Check if ImageCache is disabled
    if (imageCacheFolder.empty()) { return; }

    set<wstring> imageCacheFiles;
    vector<wstring> files = Directory::GetFiles(imageCacheFolder, L"*.*", true, false);
    for (auto iter = files.begin(); iter != files.end(); ++iter)
    {
        StringUtility::Replace<wstring>(*iter, imageCacheFolder, L"");
#if !defined(PLATFORM_UNIX)
        StringUtility::TrimLeading<wstring>(*iter, L"\\");
#else
        StringUtility::TrimLeading<wstring>(*iter, L"/");
#endif
        imageCacheFiles.insert(*iter);
    }

    for (auto const & imageStoreFile : imageStoreFiles)
    {
        // ApplicationManifest and ApplicationInstance files are not downloaded to the node
        if (StringUtility::Contains<wstring>(imageStoreFile, L"ApplicationManifest") || StringUtility::Contains<wstring>(imageStoreFile, L"ApplicationInstance"))
        {
            continue;
        }

        auto iter = imageCacheFiles.find(imageStoreFile);
        if (iter == imageCacheFiles.end())
        {
            TestSession::FailTest("File '{0}' is found in the ImageStore but not in the ImageCache", imageStoreFile);
        }

        imageCacheFiles.erase(iter);
    }

    for (auto const & imageCacheFile : imageCacheFiles)
    {
        std::wstring fileExtension = Path::GetExtension(imageCacheFile);
        if (fileExtension == L".ReaderLock" ||
            fileExtension == L".WriterLock" ||
            fileExtension == L".CacheLock" ||
            fileExtension == L".new") {
            continue;
        }

        TestSession::FailTest("File '{0}' is found in the ImageCache but not in the ImageStore", imageCacheFile);
    }
}

ReliabilityTestApi::ReconfigurationAgentComponentTestApi::ReconfigurationAgentProxyTestHelperUPtr FabricTestNode::GetProxyForStatefulHost()
{
    return RuntimeManager->GetProxyForStatefulHost();
}
