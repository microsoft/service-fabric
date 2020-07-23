// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const Trace_LocalResourceManager("LocalResourceManager");

LocalResourceManager::LocalResourceManager(
    Common::ComponentRoot const & root,
    __in FabricNodeConfigSPtr const & fabricNodeConfig,
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    fabricNodeConfig_(fabricNodeConfig),
    hosting_(hosting),
    numAvailableCores_(0),
    numUsedCoresCorrected_(0),
    availableMemoryInMB_(0),
    memoryMBInUse_(0),
    healthWarning_(0),
    systemMemoryInMB_(0),
    systemCpuCores_(0),
    images_(),
    getImagesQueryErrors_(0)
{
}

Common::ErrorCode LocalResourceManager::RegisterServicePackage(VersionedServicePackage const & package)
{
    AcquireExclusiveLock lockMe(lock_);

    auto error = RegisterServicePackageInternal(package.ServicePackageObj.Id,
        package.PackageDescription.ResourceGovernanceDescription,
        package.ServicePackageObj.ApplicationName);

    WriteInfo(
        Trace_LocalResourceManager,
        Root.TraceId,
        "RegisterServicePackage: Id={0}, InstanceId={1}, IsGoverned={2}.",
        package.ServicePackageObj.Id,
        package.ServicePackageObj.InstanceId,
        package.PackageDescription.ResourceGovernanceDescription.IsGoverned);

    return error;
}

void LocalResourceManager::UnregisterServicePackage(VersionedServicePackage const & package)
{
    AcquireExclusiveLock lockMe(lock_);

    hosting_.HealthManagerObj->UnregisterSource(
        package.ServicePackageObj.Id,
        *ServiceModel::Constants::ResourceGovernanceReportProperty);

    UnregisterServicePackageInternal(package.ServicePackageObj.Id,
        package.PackageDescription.ResourceGovernanceDescription);
}

Common::ErrorCode LocalResourceManager::AdjustCPUPoliciesForCodePackage(
    CodePackage const & package,
    ResourceGovernancePolicyDescription & rg,
    bool isContainer,
    bool isContainerGroup)
{
    AcquireExclusiveLock lockMe(lock_);

    ServicePackageInstanceIdentifier const& spID = package.CodePackageInstanceId.ServicePackageInstanceId;
    auto const& registeredSpIterator = registeredSPs_.find(spID);

    if (registeredSpIterator == registeredSPs_.end())
    {
        // Service package is not registered, so nothing to change.
        WriteInfo(
            Trace_LocalResourceManager,
            Root.TraceId,
            "Code Package {0} ignored because Service Package {1} is not registered. ",
            package.CodePackageInstanceId.CodePackageName,
            spID);
        return ErrorCodeValue::Success;
    }

    double numCoresAllocated = registeredSpIterator->second.CpuCores;

    //no cores allocated here so nothing to adjust
    if (numCoresAllocated < 0.0001)
    {
        return ErrorCodeValue::Success;
    }

    bool isDefined = false;
    double fractionOfSp = GetCpuFraction(package, rg, isDefined);

    //if this is a container group we should not split shares in case they are not defined
    //we will have a cgroup parent that will govern
    if (isContainer && (!isContainerGroup || isDefined))
    {
        double nanoCpus = numCoresAllocated * Constants::DockerNanoCpuMultiplier;
        //adjust nano cpus accroding to shares of other 
        rg.NanoCpus = static_cast<uint64>(fractionOfSp * nanoCpus);
    }
    else
    {
#if defined(PLATFORM_UNIX)
        //If cpu shares are not defined properly do not do anything on CP level - SP cgroup limits will govern CPU usage
        if (isDefined)
        {
            //If the quota is equal to period it basically means we get 1 core
            //So here we multiply with the number of cores but adjust with the shares for this CP
            rg.CpuQuota = static_cast<uint>(fractionOfSp * numCoresAllocated * Constants::CgroupsCpuPeriod);
        }
#else
        double shares = fractionOfSp * Constants::JobObjectCpuCyclesNumber;
        // On all supported systems we will use shares on ALL cores in the system
        shares = shares * (numCoresAllocated / max(numAvailableCores_, systemCpuCores_));
        rg.CpuShares = static_cast<uint>(shares);
#endif
    }

    WriteInfo(
        Trace_LocalResourceManager,
        Root.TraceId, 
        "CP Rg policy after adjusting is {0}, isContainer:{1} isContainerGroup:{2} for CP name {3}", rg, isContainer, isContainerGroup, package.CodePackageInstanceId.CodePackageName);

    return ErrorCodeValue::Success;
}

double LocalResourceManager::GetResourceUsage(std::wstring const & resourceName)
{
    if (resourceName == *ServiceModel::Constants::SystemMetricNameCpuCores)
    {
        return static_cast<double>(numUsedCoresCorrected_) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
    }
    else if (resourceName == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
    {
        return static_cast<double>(memoryMBInUse_);
    }

    Common::Assert::TestAssert("LocalResourceManager::GetResourceUsage({0}): Unknown resource.", resourceName);
    return 0;
}

uint64 LocalResourceManager::GetResourceNodeCapacity(std::wstring const & resourceName) const
{
    if (resourceName == *ServiceModel::Constants::SystemMetricNameCpuCores)
    {
        return numAvailableCores_;
    }
    else if (resourceName == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
    {
        return availableMemoryInMB_;
    }

    Common::Assert::TestAssert("LocalResourceManager::GetResourceNodeCapacity({0}): Unknown resource.", resourceName);
    return 0;
}

Common::ErrorCode LocalResourceManager::OnOpen()
{
    // Set up capacities correctly, based on what user provides in the cluster manifest.
    if (!GetCapacityFromConfiguration(*ServiceModel::Constants::SystemMetricNameCpuCores, numAvailableCores_))
    {
        // if user didn't specify node capacities for CPUCores in the manifest
        // LRM should see infinity as available cores
        numAvailableCores_ = UINT64_MAX;
        // save this state into a flag in order to detect this situation when registering SP
        // and trying to affinitize cores for processes
        WriteInfo(
            Trace_LocalResourceManager,
            Root.TraceId,
            "Not specified node capacity for {0} metric in the manifest.Falling back to infinite capacity.", *ServiceModel::Constants::SystemMetricNameCpuCores);
    }
    if (!GetCapacityFromConfiguration(*ServiceModel::Constants::SystemMetricNameMemoryInMB, availableMemoryInMB_))
    {
        // if user didn't specify node capacities for Memory in the manifest
        // LRM should see infinity as available memory
        availableMemoryInMB_ = UINT64_MAX;
        WriteInfo(
            Trace_LocalResourceManager,
            Root.TraceId,
            "Not specified node capacity for {0} metric in the manifest.Falling back to infinite capacity.", *ServiceModel::Constants::SystemMetricNameMemoryInMB);
    }

    if (!HostingConfig::GetConfig().LocalResourceManagerTestMode)
    {
        hosting_.HealthManagerObj->RegisterSource(*ServiceModel::Constants::ResourceGovernanceReportProperty);
        // if we're not in test mode
        // check and correct Node capacities for RG metrics(CPU and Memory)
        // read real capacities for both Windows and Linux and set them if AutoDetectAvailableResources is turned on
        auto error = CheckAndCorrectAvailableResources();
        if (!error.IsSuccess())
        {
            return error;
        }
    }

#if !defined(PLATFORM_UNIX)
    WriteInfo(
        Trace_LocalResourceManager,
        Root.TraceId,
        "LocalResourceManager OS information: IsWindows10OrGreater() == {0}.",
        IsWindows10OrGreater());
#endif

    numUsedCoresCorrected_ = 0;
    memoryMBInUse_ = 0;

    WriteInfo(
        Trace_LocalResourceManager,
        Root.TraceId,
        "LocalResourceManager opened with {0} cores and {1} MB of memory available.",
        numAvailableCores_,
        availableMemoryInMB_);

    // Set up timer to periodically invoke GetNodeAvailableContainerImages
    auto componentRoot = Root.CreateComponentRoot();
    getImagesTimer_ = Common::Timer::Create(
        "LocalResourceManager.GetNodeAvailableContainerImages",
        [this, componentRoot](Common::TimerSPtr const&) mutable { this->GetNodeAvailableContainerImages(); }, true);
    getImagesTimer_->Change(HostingConfig::GetConfig().NodeAvailableContainerImagesRefreshInterval);

    return ErrorCodeValue::Success;
}

Common::ErrorCode Hosting2::LocalResourceManager::OnClose()
{
    // Unregister node from reporting health warnings if it was registered
    if (!HostingConfig::GetConfig().LocalResourceManagerTestMode)
    {
        hosting_.HealthManagerObj->UnregisterSource(*ServiceModel::Constants::ResourceGovernanceReportProperty);
    }

    if (getImagesTimer_ != nullptr)
    {
        getImagesTimer_->Cancel();
    }

    return ErrorCodeValue::Success;
}

void Hosting2::LocalResourceManager::OnAbort()
{
    if (getImagesTimer_ != nullptr)
    {
        getImagesTimer_->Cancel();
    }
}

Common::ErrorCode LocalResourceManager::CheckAndCorrectAvailableResources()
{
    bool isAutoDetection = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().AutoDetectAvailableResources;
    double cpuPercent = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().CpuPercentageNodeCapacity;
    double memoryPercent = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MemoryPercentageNodeCapacity;

    // Read memory and emit a warning if user specified more memory then node actually has
    Common::ErrorCode errorMemory = Environment::GetAvailableMemoryInBytes(systemMemoryInMB_);
    if (!errorMemory.IsSuccess())
    {
        return errorMemory;
    }
    systemMemoryInMB_ /= (1024 * 1024);
    if (systemMemoryInMB_ < availableMemoryInMB_)
    {
        // Available memory is smaller than what user provided.
        if (availableMemoryInMB_ == UINT64_MAX)
        {
            // if user didn't specify node capacities, just write info
            WriteInfo(
                Trace_LocalResourceManager,
                Root.TraceId,
                "User didn't specify capacities for memory. Fallen back to infinite capacity, but available is {0}.",
                systemMemoryInMB_);
        }
        else
        {
            // if user specified bad capacities, write warning for debugging
            WriteWarning(
                Trace_LocalResourceManager,
                Root.TraceId,
                "Available memory is smaller than what user provided: Manifest requested {0} available {1}.",
                availableMemoryInMB_,
                systemMemoryInMB_);
            // remember that it is needed to emit a health warning for the memory metric
            healthWarning_ |= MemoryWarning;
        }

        // if auto-detection is enabled assign available memory that will Hosting see to real node capacity
        if (isAutoDetection)
        {
            // encounter the percentage that is specified in the configuration
            availableMemoryInMB_ = static_cast<uint64>(systemMemoryInMB_ * memoryPercent);
        }
    }

    // If user specified more CPU capacity then the node really has
    // do not use affinity mask at all
    // save this state into a flag in order to detect this situation when registering SP
    // or trying to affinitize cores for processes
    systemCpuCores_ = Environment::GetNumberOfProcessors();
    if (systemCpuCores_ < numAvailableCores_)
    {
        // Available number of cores is smaller than what user provided.
        if (numAvailableCores_ == UINT64_MAX)
        {
            WriteInfo(
                Trace_LocalResourceManager,
                Root.TraceId,
                "User didn't specify capacities for CpuCores. Fallen back to infinite capacity, but available is {0}.",
                systemCpuCores_);
        }
        else
        {
            WriteWarning(
                Trace_LocalResourceManager,
                Root.TraceId,
                "Available number of cores is smaller than what user provided: Manifest requested {0} available {1}.",
                numAvailableCores_,
                systemCpuCores_);
            // remember that health warning needs to be shown for cpu metric
            healthWarning_ |= CpuWarning;
        }

        // if auto-detection is enabled assign available cores that will Hosting see to real number of cores
        if (isAutoDetection)
        {
            // in case when node has only 1 core, use 100% of capacity
            numAvailableCores_ = (systemCpuCores_ == 1) ? 1: static_cast<uint64>(systemCpuCores_ * cpuPercent);
        }
    }
    return ErrorCodeValue::Success;
}

std::wstring LocalResourceManager::GetExtraDescriptionHealthWarning(
    bool isCapacityMismatch, 
    bool reportMemoryNotDefined, 
    bool reportCpuNotDefined,
    std::wstring const & applicationName)
{
    std::wstring extraDescription;
    if (isCapacityMismatch)
    {
        if (healthWarning_ & CpuWarning)
        {
            extraDescription = L"ServiceFabric:/_CpuCores";
        }
        if (healthWarning_ & MemoryWarning)
        {
            if (!extraDescription.empty()) extraDescription.append(L" and ");
            extraDescription.append(L"ServiceFabric:/_MemoryInMB");
        }

        extraDescription.append(wformatString(L". Real node capacities are: {0} cores and {1} memory in MB.", systemCpuCores_, systemMemoryInMB_));
    }
    else
    {
        if (reportCpuNotDefined)
        {
            extraDescription = L"ServiceFabric:/_CpuCores";
        }
        if (reportMemoryNotDefined)
        {
            if (!extraDescription.empty()) extraDescription.append(L" and ");
            extraDescription.append(L"ServiceFabric:/_MemoryInMB");
        }
        extraDescription.append(wformatString(
            L". But application '{0}' on node '{1}' is requesting resource governance.",
            applicationName,
            fabricNodeConfig_->InstanceName));
    }
    return extraDescription;
}

void LocalResourceManager::ReportResourceCapacityMismatch()
{
    // emit health warning on node when bad capacities are specified
    std::wstring extraDescription = GetExtraDescriptionHealthWarning(true);
    // clearing healthWarning_ as we want to report health warning for bad capacities only once
    // when first SP registers on this node
    healthWarning_ = 0;
    hosting_.HealthManagerObj->ReportHealth(
        *ServiceModel::Constants::ResourceGovernanceReportProperty,
        SystemHealthReportCode::Hosting_AvailableResourceCapacityMismatch,
        extraDescription,
        SequenceNumber::GetNext());
}

void LocalResourceManager::ReportResourceCapacityNotDefined(
    ServicePackageInstanceIdentifier const & packageId,
    bool reportMemoryNotDefined,
    bool reportCpuNotDefined,
    std::wstring const & applicationName)
{
    std::wstring extraDescription = GetExtraDescriptionHealthWarning(false, reportMemoryNotDefined, reportCpuNotDefined, applicationName);
    hosting_.HealthManagerObj->ReportHealth(
        packageId,
        *ServiceModel::Constants::ResourceGovernanceReportProperty,
        SystemHealthReportCode::Hosting_AvailableResourceCapacityNotDefined,
        extraDescription,
        SequenceNumber::GetNext());
}

Common::ErrorCode LocalResourceManager::RegisterServicePackageInternal(
    ServicePackageInstanceIdentifier const & packageId,
    ServiceModel::ServicePackageResourceGovernanceDescription const & rgSettings,
    std::wstring const & applicationName)
{

    auto memoryRequested = rgSettings.MemoryInMB;
    // multiply with CpuCorrectionFactor to avoid double arithmetic issues later
    auto coresRequested = static_cast<uint64> (rgSettings.CpuCores * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor);

    // whether we need to report that Memory or CPU aren't define in the manifest
    bool reportMemoryNotDefined = availableMemoryInMB_ == UINT64_MAX;
    bool reportCpuNotDefined = numAvailableCores_ == UINT64_MAX;

    // before emitting health warning check if user is using RG at all and if we're not in test mode
    if (!HostingConfig::GetConfig().LocalResourceManagerTestMode && rgSettings.IsGoverned)
    {
        if (healthWarning_ > 0)
        {
            // Report it on node that capacities are bad, only once - when first SP registers on this node
            ReportResourceCapacityMismatch();
        }
        if (reportMemoryNotDefined || reportCpuNotDefined)
        {
            hosting_.HealthManagerObj->RegisterSource(
                packageId,
                applicationName,
                *ServiceModel::Constants::ResourceGovernanceReportProperty);
            // Report it on service package level that capacities are not specified and the customer is using RG
            // report for each service package that registers on node with bad capacities and uses RG
            ReportResourceCapacityNotDefined(packageId, reportMemoryNotDefined, reportCpuNotDefined, applicationName);
        }
    }

    auto const& registeredSPIt = registeredSPs_.find(packageId);
    if (registeredSPIt != registeredSPs_.end())
    {
        if (registeredSPIt->second != rgSettings)
        {
            // Different RG settings for the same SP should be possible only during upgrade, not during registration.
            WriteError(
                Trace_LocalResourceManager,
                Root.TraceId,
                "Changing RG settings for already registered SP should not be done using RegisterServicePackage. Id = {0} CurrentSettings = {1} NewSettings = {2}.",
                packageId,
                registeredSPIt->second,
                rgSettings);
            return ErrorCodeValue::ServicePackageAlreadyRegisteredWithLRM;
        }
        else
        {
            // This should not be happening, but functionally we will be OK.
            WriteInfo(
                Trace_LocalResourceManager,
                Root.TraceId,
                "Skipping registration for Service Package {0} as it is already registered.",
                packageId);
            return ErrorCodeValue::Success;
        }
    }

    // we need to multiply with ResourceGovernanceCpuCorrectionFactor to avoid issues with rounding in double arithmetic
    // when user didn't specify capacity for Cpu do not multiply
    uint64 numAvailableCpuCorrected = (numAvailableCores_ == UINT64_MAX) ? numAvailableCores_ : static_cast<uint64> (numAvailableCores_ * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor);
    if (coresRequested > numAvailableCpuCorrected - numUsedCoresCorrected_)
    {
        double cpuCoresLeft = static_cast<double>(numAvailableCpuCorrected - numUsedCoresCorrected_) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
        WriteWarning(
            Trace_LocalResourceManager,
            Root.TraceId,
            "Unable to register service package {0} - requested {1} cores, available {2} cores.  After CPU correction [Totally available CpuCores on node are {3}, CpuCoresInUse {4} and requested CpuCores {5}]. Calculated requested cores {6}",
            packageId,
            rgSettings.CpuCores,
            cpuCoresLeft,
            numAvailableCpuCorrected,
            numUsedCoresCorrected_,
            coresRequested,
            static_cast<double>(coresRequested) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor);
        return ErrorCodeValue::NotEnoughCPUForServicePackage;
    }

    if (memoryRequested > availableMemoryInMB_ - memoryMBInUse_)
    {
        WriteWarning(
            Trace_LocalResourceManager,
            Root.TraceId,
            "Unable to register service package {0} - requested {1} MB, available {2} MB.",
            packageId,
            memoryRequested,
            availableMemoryInMB_ - memoryMBInUse_);
        return ErrorCodeValue::NotEnoughMemoryForServicePackage;
    }

    numUsedCoresCorrected_ += coresRequested;
    memoryMBInUse_ += memoryRequested;


    registeredSPs_.insert(make_pair(packageId, rgSettings));
    WriteInfo(
        Trace_LocalResourceManager,
        Root.TraceId,
        "Registering service package {0} with {1} cores and {2} MB memory. After CPU correction [Totally available CpuCores on node are {3}, CpuCoresInUse {4} and requested CpuCores {5}]. Calculated requested cores {6}",
        packageId,
        rgSettings.CpuCores,
        memoryRequested,
        numAvailableCpuCorrected,
        numUsedCoresCorrected_,
        coresRequested,
        static_cast<double>(coresRequested) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor);

    return ErrorCodeValue::Success;
}

void LocalResourceManager::UnregisterServicePackageInternal(ServicePackageInstanceIdentifier const & packageId,
    ServiceModel::ServicePackageResourceGovernanceDescription const& rgSettings)
{
    auto registeredSPIt = registeredSPs_.find(packageId);

    if (registeredSPIt == registeredSPs_.end())
    {
        return;
    }

    if (registeredSPIt->second != rgSettings)
    {
        WriteError(
            Trace_LocalResourceManager,
            Root.TraceId,
            "UnregisterServicePackageInternal(): Provided RG settings {0} do not match registered RG settings {1}.",
            rgSettings,
            registeredSPIt->second);
        Common::Assert::TestAssert("Provided RG settings {0} do not match registered RG settings {1}.",
            rgSettings,
            registeredSPIt->second);
    }

    // Increase totals for available resources.
    memoryMBInUse_ -= registeredSPIt->second.MemoryInMB;
    numUsedCoresCorrected_ -= static_cast<uint64> (registeredSPIt->second.CpuCores * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor);

    registeredSPs_.erase(packageId);

    WriteInfo(
        Trace_LocalResourceManager,
        Root.TraceId,
        "Unregistering service package {0}.",
        packageId);
}

bool LocalResourceManager::GetCapacityFromConfiguration(std::wstring const & metric, uint64 & capacity)
{
    auto & capacities = fabricNodeConfig_->NodeCapacities;
    for (auto itCapacity = capacities.begin(); itCapacity != capacities.end(); ++itCapacity)
    {
        if (StringUtility::AreEqualPrefixPartCaseInsensitive(itCapacity->first, metric, L':'))
        {
            capacity = itCapacity->second;
            return true;
        }
    }
    return false;
}

double LocalResourceManager::GetCpuFraction(CodePackage const & package, ResourceGovernancePolicyDescription const & rg, bool & isDefined) const
{
    auto & sp = package.VersionedServicePackageObj;
    auto & digestedCodePackages = sp.PackageDescription.DigestedCodePackages;
    uint totalRate = 0;
    uint CPcount = static_cast<uint>(digestedCodePackages.size());
    bool foundZero = false;

    for (auto dCP : digestedCodePackages)
    {
        if (0 == dCP.ResourceGovernancePolicy.CpuShares)
        {
            foundZero = true;
            break;
        }
        totalRate += dCP.ResourceGovernancePolicy.CpuShares;
    }
    if (foundZero)
    {
        // If some of the code packages has zero set for CPU rate, then we will not recalculate for others.
        totalRate = 0;
    }

    double fraction = 0.0;

    //this means we have shares properly defined
    if (totalRate > 0)
    {
        fraction = (double)rg.CpuShares / totalRate;
        isDefined = true;
    }
    else
    {
        fraction = 1.0 / CPcount;
        isDefined = false;
    }

    return fraction;
}

void LocalResourceManager::GetNodeAvailableContainerImages()
{
    if (hosting_.State.Value > FabricComponentState::Opened)
    {
        // no need to notify if the Hosting Subsystem is closing down
        return;
    }

    // Read from PLB if preferred container placement is enabled
    bool preferNodesForContainerPlacement = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PreferNodesForContainerPlacement;

    // Send available images only if it is enabled and LRM is not in test mode
    if (preferNodesForContainerPlacement &&
        !HostingConfig::GetConfig().LocalResourceManagerTestMode &&
        !HostingConfig::GetConfig().DisableContainers)
    {
        // Periodically get available images from docker and send them to the FM
        auto operation = hosting_.FabricActivatorClientObj->BeginGetImages(
            HostingConfig::GetConfig().NodeAvailableContainerImagesTimeout,
            [this](AsyncOperationSPtr const & operation) 
        {
            this->OnGetNodeAvailableContainerImagesCompleted(operation, false);
        },
            Root.CreateAsyncOperationRoot());
        OnGetNodeAvailableContainerImagesCompleted(operation, true);
    }
}

void LocalResourceManager::OnGetNodeAvailableContainerImagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = hosting_.FabricActivatorClientObj->EndGetImages(operation, images_);

    int64 nodeAvailableContainerImagesRefreshInterval = HostingConfig::GetConfig().NodeAvailableContainerImagesRefreshInterval.TotalSeconds();
    if (error.IsSuccess())
    {
        getImagesQueryErrors_ = 0;
        this->SendAvailableContainerImagesToFM();
    }
    else
    {
        getImagesQueryErrors_++;
        // wait a bit more due if there are errors
        nodeAvailableContainerImagesRefreshInterval += (nodeAvailableContainerImagesRefreshInterval * getImagesQueryErrors_) / 2;
        WriteWarning(
            Trace_LocalResourceManager,
            Root.TraceId,
            "Failed to get images with error: {0}. Query failed {1} times sequentialy, next function for querying images will be scheduled in {2} seconds.",
            error,
            getImagesQueryErrors_,
            nodeAvailableContainerImagesRefreshInterval);
    }

    TimeSpan getImagesRetryInterval = TimeSpan::FromSeconds(static_cast<double>(nodeAvailableContainerImagesRefreshInterval));
    getImagesTimer_->Change(getImagesRetryInterval);
}

void LocalResourceManager::SendAvailableContainerImagesToFM() const
{
    WriteInfo(
        Trace_LocalResourceManager,
        Root.TraceId,
        "Message is sent to the FM from node: {0} with images: {1}",
        hosting_.NodeId,
        images_);

    SendAvailableContainerImagesEventArgs eventArgs(move(images_), hosting_.NodeId);
    hosting_.EventDispatcherObj->EnqueueAvaialableImagesEvent(move(eventArgs));
}