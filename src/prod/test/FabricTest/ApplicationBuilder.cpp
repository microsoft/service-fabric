// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const TraceSource = "ApplicationBuilder";
wstring const ApplicationBuilder::DefaultStatelessServicePackage = L"Stateless";
wstring const ApplicationBuilder::DefaultStatefulServicePackage = L"Stateful";

wstring const ApplicationBuilder::ApplicationBuilderCreateApplicationType = L"app.add";
wstring const ApplicationBuilder::ApplicationBuilderSetCodePackage = L"app.codepack";
wstring const ApplicationBuilder::ApplicationBuilderSetServicePackage = L"app.servicepack";
wstring const ApplicationBuilder::ApplicationBuilderSetServiceTypes = L"app.servicetypes";
wstring const ApplicationBuilder::ApplicationBuilderSetServiceTemplates = L"app.servicetemplate";
wstring const ApplicationBuilder::ApplicationBuilderSetDefaultService = L"app.reqservices";
wstring const ApplicationBuilder::ApplicationBuilderSetUser = L"app.user";
wstring const ApplicationBuilder::ApplicationBuilderSetGroup = L"app.group";
wstring const ApplicationBuilder::ApplicationBuilderRunAs = L"app.runas";
wstring const ApplicationBuilder::ApplicationBuilderSharedPackage = L"app.sharedpackage";
wstring const ApplicationBuilder::ApplicationBuilderHealthPolicy = L"app.healthpolicy";
wstring const ApplicationBuilder::ApplicationBuilderNetwork = L"app.network";
wstring const ApplicationBuilder::ApplicationBuilderEndpoint = L"app.endpoint";
wstring const ApplicationBuilder::ApplicationBuilderUpload = L"app.upload";
wstring const ApplicationBuilder::ApplicationBuilderDelete = L"app.delete";
wstring const ApplicationBuilder::ApplicationBuilderClear = L"app.clear";
wstring const ApplicationBuilder::ApplicationBuilderSetParameters = L"app.parameters";

wstring const ApplicationBuilder::ApplicationPackageFolderInImageStore = L"incoming";

std::map<std::wstring, ApplicationBuilder> ApplicationBuilder::applicationTypes_;
Common::ExclusiveLock ApplicationBuilder::applicationTypesLock_; 

bool ApplicationBuilder::IsApplicationBuilderCommand(std::wstring const& command)
{
    wstring string(L"app.");
    return StringUtility::StartsWith(command, string) ? true : false;
}

bool ApplicationBuilder::ExecuteCommand(
    wstring const& command, 
    StringCollection const& params,
    wstring const& clientCredentialsType,
    ComPointer<IFabricNativeImageStoreClient> const & imageStoreClientCPtr)
{
    if(params.size() < 1)
    {
        FABRICSESSION.PrintHelp(command);
        return false;
    }

    AcquireExclusiveLock grab(applicationTypesLock_);
    wstring const& appFriendlyName = params[0];
    auto iter = applicationTypes_.find(appFriendlyName);
    if(iter == applicationTypes_.end())
    {
        wstring toCreateFrom;
        if(command == ApplicationBuilder::ApplicationBuilderCreateApplicationType)
        {
            wstring appTypeName;
            wstring appTypeVersion;
            if(params.size() == 1)
            {
                appTypeName = params[0];
                appTypeVersion = L"1.0";
            }
            else if(params.size() == 3)
            {
                appTypeName = params[1];
                appTypeVersion = params[2];
            }
            else if(params.size() == 4)
            {
                appTypeName = params[1];
                appTypeVersion = params[2];
                toCreateFrom = params[3];
            }
            else
            {
                FABRICSESSION.PrintHelp(command);
                return false;
            }

            if(toCreateFrom.empty())
            {
                applicationTypes_.insert(make_pair(
                    appFriendlyName, 
                    ApplicationBuilder(appTypeName, appTypeVersion, clientCredentialsType)));
            }
            else
            {
                auto iterToCreateFrom = applicationTypes_.find(toCreateFrom);
                if(iterToCreateFrom == applicationTypes_.end())
                {
                    FABRICSESSION.PrintHelp(command);
                    return false;
                }

                ApplicationBuilder application = iterToCreateFrom->second;
                application.applicationTypeName_ = appTypeName;
                application.applicationTypeVersion_ = appTypeVersion;
                applicationTypes_.insert(make_pair(appFriendlyName, move(application)));
            }

            iter = applicationTypes_.find(appTypeName);

            return true;
        }
        else
        {
            return false;
        }
    }

    ApplicationBuilder & applicationBuilder = iter->second;
    if (command == ApplicationBuilder::ApplicationBuilderSetCodePackage)
    {
        return applicationBuilder.SetCodePackage(params, clientCredentialsType);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderUpload)
    {
        return applicationBuilder.UploadApplication(appFriendlyName, params, imageStoreClientCPtr);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderDelete)
    {
        return applicationBuilder.DeleteApplication(appFriendlyName);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderSetServicePackage)
    {
        return applicationBuilder.SetServicePackage(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderSetServiceTypes)
    {
        return applicationBuilder.SetServicetypes(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderSetServiceTemplates)
    {
        return applicationBuilder.SetServiceTemplates(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderSetDefaultService)
    {
        return applicationBuilder.SetDefaultService(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderSetUser)
    {
        return applicationBuilder.SetUser(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderSetGroup)
    {
        return applicationBuilder.SetGroup(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderRunAs)
    {
        return applicationBuilder.SetRunAs(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderHealthPolicy)
    {
        return applicationBuilder.SetHealthPolicy(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderClear)
    {
        return applicationBuilder.Clear();
    }
    else if(command == ApplicationBuilder::ApplicationBuilderSetParameters)
    {
        return applicationBuilder.SetParameters(params);
    }
    else if(command == ApplicationBuilder::ApplicationBuilderSharedPackage)
    {
        return applicationBuilder.SetSharedPackage(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderNetwork)
    {
        return applicationBuilder.SetNetwork(params);
    }
    else if (command == ApplicationBuilder::ApplicationBuilderEndpoint)
    {
        return applicationBuilder.SetEndpoint(params);
    }
    else
    {
        return false;
    }
}

void ApplicationBuilder::GetDefaultServices(wstring const& appType, wstring const& version, StringCollection & defaultServices)
{   
    AcquireExclusiveLock grab(applicationTypesLock_);
    for (auto iter = applicationTypes_.begin() ; iter != applicationTypes_.end(); ++iter)
    {
        auto & appbuilder = iter->second;

        if (appbuilder.applicationTypeName_ == appType && appbuilder.applicationTypeVersion_ == version)
        {
            appbuilder.GetDefaultServices(defaultServices);

            break;
        }
    }    
}

wstring ApplicationBuilder::GetApplicationBuilderName(wstring const& appType, wstring const& version)
{
    AcquireExclusiveLock grab(applicationTypesLock_);
    for (auto iter = applicationTypes_.begin() ; iter != applicationTypes_.end(); ++iter)
    {
        auto & appbuilder = iter->second;

        if (appbuilder.applicationTypeName_ == appType && appbuilder.applicationTypeVersion_ == version)
        {
            return iter->first;
        }
    } 

    TestSession::FailTest("AppType {0} and version {1} not found", appType, version);
}

bool ApplicationBuilder::TryGetDefaultServices(
    wstring const& appBuilderName,
    StringCollection & defaultServices)
{
    AcquireExclusiveLock grab(applicationTypesLock_);
    auto iter = applicationTypes_.find(appBuilderName);
    TestSession::FailTestIf(iter == applicationTypes_.end(), "AppBuilder {0} does not exist", appBuilderName);
    auto & appBuilder = iter->second;
    appBuilder.GetDefaultServices(defaultServices);
    return true;
}

bool ApplicationBuilder::TryGetServiceManifestDescription(
    wstring const& appBuilderName, 
    wstring const& servicePackageName, 
    ServiceModel::ServiceManifestDescription & serviceManifest)
{   
    AcquireExclusiveLock grab(applicationTypesLock_);
    auto iter = applicationTypes_.find(appBuilderName);
    TestSession::FailTestIf(iter == applicationTypes_.end(), "AppBuilder {0} does not exist", appBuilderName);
    auto const&  appbuilder = iter->second;
    for (auto iter1 = appbuilder.serviceManifestImports_.begin() ; iter1 != appbuilder.serviceManifestImports_.end(); ++iter1)
    {
        auto const& manifest = iter1->second;
        if(manifest.Name == servicePackageName)
        {
            serviceManifest = manifest;
            return true;
        }
    } 

    return false;
}

bool ApplicationBuilder::TryGetSupportedServiceTypes(
    std::wstring const& appBuilderName,
    ServiceModel::CodePackageIdentifier const& codePackageId, 
    std::vector<wstring> & serviceTypes)
{   
    AcquireExclusiveLock grab(applicationTypesLock_);
    auto iter = applicationTypes_.find(appBuilderName);
    TestSession::FailTestIf(iter == applicationTypes_.end(), "AppBuilder {0} does not exist", appBuilderName);
    auto const&  appbuilder = iter->second;
    for (auto iter1 = appbuilder.serviceManifestImports_.begin() ; iter1 != appbuilder.serviceManifestImports_.end(); ++iter1)
    {
        auto const& manifest = iter1->second;
        if(manifest.Name == codePackageId.ServicePackageId.ServicePackageName)
        {
            TestSession::FailTestIfNot(manifest.ConfigPackages.size() == 1, "There should only be one config package");
            auto configSettingsIter = appbuilder.configSettings_.find(manifest.Name + manifest.ConfigPackages[0].Name);
            TestSession::FailTestIf(configSettingsIter == appbuilder.configSettings_.end(), "Config settings not found");
            auto configSectionIter = configSettingsIter->second.Sections.find(FabricTestConstants::SupportedServiceTypesSection);
            TestSession::FailTestIf(configSectionIter == configSettingsIter->second.Sections.end(), "Config section not found");

            auto configParamIter = configSectionIter->second.Parameters.find(codePackageId.CodePackageName);
            TestSession::FailTestIf(configParamIter == configSectionIter->second.Parameters.end(), "Parameter {0} not found", codePackageId.CodePackageName);
            wstring types = configParamIter->second.Value;
            StringUtility::Split<wstring>(types, serviceTypes, L",");
            return true;
        }
    } 

    return false;
}

ApplicationBuilder::ApplicationBuilder(
    wstring const& applicationTypeName, 
    wstring const& applicationVersion,
    wstring const& clientCredentialsType)
{
    // Set default application
    applicationTypeName_ = applicationTypeName;

    applicationTypeVersion_ = applicationVersion;

    applicationParameters_.insert(make_pair(L"ServiceInstanceCount", L"1"));
    applicationParameters_.insert(make_pair(L"StatefulReplicaCount", L"3"));
    applicationParameters_.insert(make_pair(L"StatefulPartitionCount", L"3"));

    // Add stateless service templates
    StatelessService calcService;
    calcService.TypeName = L"CalculatorService";
    calcService.InstanceCount = L"1";
    calcService.Scheme = PartitionScheme::Singleton;
    statelessServiceTemplates_.push_back(calcService);

    StatelessService sciCalcService;
    sciCalcService.TypeName = L"ScientificCalculatorService";
    sciCalcService.InstanceCount = L"1";
    sciCalcService.Scheme = PartitionScheme::Singleton;
    statelessServiceTemplates_.push_back(sciCalcService);

    // Add stateful service templates
    StatefulService statefulService;
    statefulService.TypeName = L"StatefulService";
    statefulService.TargetReplicaSetSize = L"3";
    statefulService.Scheme = PartitionScheme::UniformInt64;
    statefulService.MinReplicaSetSize = L"1";
    statefulService.PartitionCount = L"3";
    statefulService.HighKey = wformatString(FabricTestSessionConfig::GetConfig().ServiceDescriptorHighRange);
    statefulService.LowKey = wformatString(FabricTestSessionConfig::GetConfig().ServiceDescriptorLowRange);
    statefulServiceTemplates_.push_back(statefulService);

    StatefulService persistedService;
    persistedService.TypeName = L"PersistedStatefulService";
    persistedService.TargetReplicaSetSize = L"3";
    persistedService.Scheme = PartitionScheme::UniformInt64;
    persistedService.MinReplicaSetSize = L"1";
    persistedService.PartitionCount = L"3";
    persistedService.HighKey = wformatString(FabricTestSessionConfig::GetConfig().ServiceDescriptorHighRange);
    persistedService.LowKey = wformatString(FabricTestSessionConfig::GetConfig().ServiceDescriptorLowRange);
    statefulServiceTemplates_.push_back(persistedService);

    // Add required stateless services
    calcService.InstanceCount = L"[ServiceInstanceCount]";
    sciCalcService.InstanceCount = L"[ServiceInstanceCount]";
    calcService.ServicePackageActivationMode = L"SharedProcess";
    sciCalcService.ServicePackageActivationMode = L"SharedProcess";
    requiredStatelessServices_.insert(make_pair(L"CalcService", calcService));
    requiredStatelessServices_.insert(make_pair(L"SciCalcService", sciCalcService));

    // Add required stateful services
    statefulService.TargetReplicaSetSize = L"[StatefulReplicaCount]";
    persistedService.TargetReplicaSetSize = L"[StatefulReplicaCount]";
    statefulService.PartitionCount = L"[StatefulPartitionCount]";
    persistedService.PartitionCount = L"[StatefulPartitionCount]";
    statefulService.ServicePackageActivationMode = L"SharedProcess";
    persistedService.ServicePackageActivationMode = L"SharedProcess";
    requiredStatefulService_.insert(make_pair(L"StatefulService", statefulService));
    requiredStatefulService_.insert(make_pair(L"PersistedService", persistedService));

    // Add stateless service manifest
    ServiceManifestDescription statelessServiceManifestDescription;
    statelessServiceManifestDescription.Name = DefaultStatelessServicePackage;
    statelessServiceManifestDescription.Version = applicationTypeVersion_;

    // Add ServiceTypeDescriptions
    ServiceTypeDescription calculatorServiceTypeDescription;
    calculatorServiceTypeDescription.ServiceTypeName = L"CalculatorService";
    calculatorServiceTypeDescription.IsStateful = false;

    ServiceTypeDescription sciCalculatorServiceTypeDescription;
    sciCalculatorServiceTypeDescription.ServiceTypeName = L"ScientificCalculatorService";
    sciCalculatorServiceTypeDescription.IsStateful = false;

    statelessServiceManifestDescription.ServiceTypeNames.push_back(calculatorServiceTypeDescription);
    statelessServiceManifestDescription.ServiceTypeNames.push_back(sciCalculatorServiceTypeDescription);

    // Add CodePackageDescriptions
    CodePackageDescription statelessCodePackage;
    statelessCodePackage.Name = L"Code";
    statelessCodePackage.Version = applicationTypeVersion_;
    statelessCodePackage.EntryPoint.ExeEntryPoint.Program = L"FabricTestHost.exe";
    statelessCodePackage.EntryPoint.ExeEntryPoint.Arguments = wformatString(
        "version={0} testdir={1} serverport={2} useetw={3} security={4}", 
        applicationTypeVersion_, 
        FabricTestDispatcher::TestDataDirectory,
        FederationTestCommon::AddressHelper::ServerListenPort,
        FabricTestSessionConfig::GetConfig().UseEtw,
        clientCredentialsType);
    statelessCodePackage.EntryPoint.EntryPointType = EntryPointType::Exe;    
    statelessCodePackage.IsShared = false;
    statelessServiceManifestDescription.CodePackages.push_back(statelessCodePackage);

    // Add ConfigPackageDescription
    ConfigPackageDescription statelessConfigPackage;
    statelessConfigPackage.Name = L"Config";
    statelessConfigPackage.Version = applicationTypeVersion_;

    ConfigParameter statelessConfigParameter;
    statelessConfigParameter.Name = L"Code";
    statelessConfigParameter.Value = L"CalculatorService,ScientificCalculatorService";
    statelessConfigParameter.MustOverride = false;

    ConfigSection statelessConfigSection;
    statelessConfigSection.Name = FabricTestConstants::SupportedServiceTypesSection;
    statelessConfigSection.Parameters.insert(make_pair(statelessConfigParameter.Name, statelessConfigParameter));

    ConfigSettings statelessConfigSettings;
    statelessConfigSettings.Sections.insert(make_pair(statelessConfigSection.Name, statelessConfigSection));

    configSettings_.insert(make_pair(statelessServiceManifestDescription.Name + statelessConfigPackage.Name, statelessConfigSettings));
    statelessServiceManifestDescription.ConfigPackages.push_back(statelessConfigPackage);

    // Add DataPackageDescriptions
    DataPackageDescription statelessDataPackage;
    statelessDataPackage.Name = L"Data";
    statelessDataPackage.Version = applicationTypeVersion_;

    statelessServiceManifestDescription.DataPackages.push_back(statelessDataPackage);

    serviceManifestImports_.insert(make_pair(DefaultStatelessServicePackage, statelessServiceManifestDescription));

    // Add stateful service manifest
    ServiceManifestDescription statefulServiceManifestDescription;
    statefulServiceManifestDescription.Name = DefaultStatefulServicePackage;
    statefulServiceManifestDescription.Version = applicationTypeVersion_;

    // Add ServiceTypeDescriptions
    ServiceTypeDescription statefulServiceTypeDescription;
    statefulServiceTypeDescription.ServiceTypeName = L"StatefulService";
    statefulServiceTypeDescription.IsStateful = true;
    statefulServiceTypeDescription.HasPersistedState = false;

    ServiceTypeDescription persistedServiceTypeDescription;
    persistedServiceTypeDescription.ServiceTypeName = L"PersistedStatefulService";
    persistedServiceTypeDescription.IsStateful = true;
    persistedServiceTypeDescription.HasPersistedState = true;

    statefulServiceManifestDescription.ServiceTypeNames.push_back(statefulServiceTypeDescription);
    statefulServiceManifestDescription.ServiceTypeNames.push_back(persistedServiceTypeDescription);

    // Add CodePackageDescriptions
    CodePackageDescription statefulCodePackage;
    statefulCodePackage.Name = L"Code";
    statefulCodePackage.Version = applicationTypeVersion_;
    statefulCodePackage.EntryPoint.ExeEntryPoint.Program = L"FabricTestHost.exe" ;
    statefulCodePackage.EntryPoint.ExeEntryPoint.Arguments = wformatString(
        "version={0} testdir={1} serverport={2} useetw={3} security={4}", 
        applicationTypeVersion_, 
        FabricTestDispatcher::TestDataDirectory,
        FederationTestCommon::AddressHelper::ServerListenPort,
        FabricTestSessionConfig::GetConfig().UseEtw,
        clientCredentialsType);
    statefulCodePackage.EntryPoint.EntryPointType = EntryPointType::Exe;    
    statefulCodePackage.IsShared = false;
    statefulServiceManifestDescription.CodePackages.push_back(statefulCodePackage);

    // Add ConfigPackageDescription
    ConfigPackageDescription statefulConfigPackage;
    statefulConfigPackage.Name = L"Config";
    statefulConfigPackage.Version = applicationTypeVersion_;

    ConfigParameter statefulConfigParameter;
    statefulConfigParameter.Name = L"Code";
    statefulConfigParameter.Value = L"StatefulService,PersistedStatefulService";
    statefulConfigParameter.MustOverride = false;

    ConfigSection statefulConfigSection;
    statefulConfigSection.Name = FabricTestConstants::SupportedServiceTypesSection;
    statefulConfigSection.Parameters.insert(make_pair(statefulConfigParameter.Name, statefulConfigParameter));

    ConfigSettings statefulConfigSettings;
    statefulConfigSettings.Sections.insert(make_pair(statefulConfigSection.Name, statefulConfigSection));

    configSettings_.insert(make_pair(statefulServiceManifestDescription.Name + statefulConfigPackage.Name, statefulConfigSettings));
    statefulServiceManifestDescription.ConfigPackages.push_back(statefulConfigPackage);

    // Add DataPackageDescriptions
    DataPackageDescription statefulDataPackage;
    statefulDataPackage.Name = L"Data";
    statefulDataPackage.Version = applicationTypeVersion_;

    statefulServiceManifestDescription.DataPackages.push_back(statefulDataPackage);

    serviceManifestImports_.insert(make_pair(DefaultStatefulServicePackage, statefulServiceManifestDescription));

    // Set users and groups for test
    StringCollection params;
    params.push_back(applicationTypeName);
    params.push_back(L"FabricUser");
    params.push_back(L"groups=fabricgroup");
    SetUser(params);

    params.clear();
    params.push_back(applicationTypeName);
    params.push_back(L"StatefulU");
    params.push_back(L"groups=fabricgroup");
    SetUser(params);

    params.clear();
    params.push_back(applicationTypeName);
    params.push_back(L"StatelessU");
    params.push_back(L"groups=fabricgroup");
    params.push_back(L"systemgroups=users");
    SetUser(params);

    params.clear();
    params.push_back(applicationTypeName);
    params.push_back(L"FabricGroup");
    params.push_back(L"systemgroups=users");
    SetGroup(params);

    params.clear();
    params.push_back(applicationTypeName);
    params.push_back(L"Stateless");
    params.push_back(L"policies=Code:StatelessU");
    SetRunAs(params);

    params.clear();
    params.push_back(applicationTypeName);
    params.push_back(L"Stateful");
    params.push_back(L"policies=Code:StatefulU");
    SetRunAs(params);

    params.clear();
    params.push_back(applicationTypeName);
    params.push_back(L"default=FabricUser");
    SetRunAs(params);
}

void ApplicationBuilder::GetDefaultServices(StringCollection & requiresServices)
{
    for (auto iter = requiredStatefulService_.begin(); iter != requiredStatefulService_.end(); iter++)
    {
        requiresServices.push_back(iter->first);
    }

    for (auto iter = requiredStatelessServices_.begin(); iter != requiredStatelessServices_.end(); iter++)
    {
        requiresServices.push_back(iter->first);
    }
}

bool ApplicationBuilder::Clear()
{
    applicationParameters_.clear();
    statelessServiceTemplates_.clear();
    statefulServiceTemplates_.clear();
    requiredStatelessServices_.clear();
    requiredStatefulService_.clear();
    serviceManifestImports_.clear();
    skipUploadCodePackages_.clear();
    skipUploadServiceManifests_.clear();
    configSettings_.clear();
    userMap_.clear();
    groupMap_.clear();
    runAsPolicies_.clear();
    rgPolicies_.clear();
    spRGPolicies_.clear();
    containerHosts_.clear();
    defaultRunAsPolicy_.clear();
    networkPolicies_.clear();
    return true;
}

bool ApplicationBuilder::SetUser(StringCollection const& params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetUser);
        return false;
    }

    wstring const& userName = params[1];

    if(userName == L"clearall")
    {
        userMap_.clear();
        return true;
    }

    CommandLineParser parser(params, 2);

    User user;
    parser.TryGetString(L"type", user.AccountType);
    parser.TryGetString(L"name", user.AccontName);
    parser.TryGetString(L"password", user.AccountPassword);
    user.IsPasswordEncrypted = parser.GetBool(L"encrypted");

    parser.GetVector(L"systemgroups", user.SystemGroups);
    parser.GetVector(L"groups", user.Groups);

    userMap_.insert(make_pair(userName, user));
    return true;
}

bool ApplicationBuilder::SetGroup(StringCollection const& params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetGroup);
        return false;
    }

    wstring const& groupName = params[1];

    if(groupName == L"clearall")
    {
        groupMap_.clear();
        return true;
    }

    CommandLineParser parser(params, 2);

    Group group;
    parser.GetVector(L"domaingroups", group.DomainGroup);
    parser.GetVector(L"systemgroups", group.SystemGroups);
    parser.GetVector(L"domainusers", group.DomainUsers);

    groupMap_.insert(make_pair(groupName, group));
    return true;
}

bool ApplicationBuilder::SetSharedPackage(StringCollection const & params)
{
     if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSharedPackage);
        return false;
    }

    if(params[1] == L"clearall")
    {
        sharedPackagePolicies_.clear();
        return true;
    }

    CommandLineParser parser(params, 1);
    Common::StringCollection sharedPackagePolicies;
    parser.GetVector(L"SharedPackagePolicies", sharedPackagePolicies);

    if(sharedPackagePolicies.size() > 0)
    {
        wstring const& serviceManifestName = params[1];
        sharedPackagePolicies_.insert(make_pair(serviceManifestName, sharedPackagePolicies));

         auto serviceManifestIter = serviceManifestImports_.find(serviceManifestName);
         TestSession::FailTestIf(serviceManifestIter == serviceManifestImports_.end(), "serviceManifestName {0} not found", serviceManifestName);
        for (auto iter = sharedPackagePolicies.begin(); iter != sharedPackagePolicies.end(); iter++)
        {
            ConfigParameter configParameter;
            configParameter.Name = *iter;
            configParameter.Value = L"true";
            configParameter.MustOverride = false;

            TestSession::FailTestIfNot(serviceManifestIter->second.ConfigPackages.size() == 1, "There should only be one config package");
            auto configSettingsIter = configSettings_.find(serviceManifestName + serviceManifestIter->second.ConfigPackages[0].Name);
            TestSession::FailTestIf(configSettingsIter == configSettings_.end(), "Config settings not found");
            auto configSectionIter = configSettingsIter->second.Sections.find(FabricTestConstants::PackageSharingSection);
            if(configSectionIter == configSettingsIter->second.Sections.end())
            {
                ConfigSection configSection;
                configSection.Name = FabricTestConstants::PackageSharingSection;
                configSettingsIter->second.Sections.insert(make_pair(configSection.Name, configSection));
                configSectionIter = configSettingsIter->second.Sections.find(FabricTestConstants::PackageSharingSection);
            }

            auto configParamIter = configSectionIter->second.Parameters.find(configParameter.Name);
            if(configParamIter == configSectionIter->second.Parameters.end())
            {
                configSectionIter->second.Parameters.insert(make_pair(configParameter.Name, configParameter));
            }
            else
            {
                configSectionIter->second.Parameters.erase(configParamIter);
                configSectionIter->second.Parameters.insert(make_pair(configParameter.Name, configParameter));
            }
        }
    }

    return true;
}

bool ApplicationBuilder::SetRunAs(StringCollection const& params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderRunAs);
        return false;
    }

    if(params[1] == L"clearall")
    {
        runAsPolicies_.clear();
        defaultRunAsPolicy_.clear();
        return true;
    }

    CommandLineParser parser(params, 1);
    wstring defaulRunAsPolicy;
    map<wstring, wstring> runAsPolicies;
    parser.GetMap(L"policies", runAsPolicies);

    if(parser.TryGetString(L"default", defaulRunAsPolicy))
    {
        defaultRunAsPolicy_ = defaulRunAsPolicy;
    }

    if(runAsPolicies.size() > 0)
    {
        wstring const& serviceManifestName = params[1];
        runAsPolicies_.insert(make_pair(serviceManifestName, runAsPolicies));

        auto serviceManifestIter = serviceManifestImports_.find(serviceManifestName);
        TestSession::FailTestIf(serviceManifestIter == serviceManifestImports_.end(), "serviceManifestName {0} not found", serviceManifestName);
        for (auto iter = runAsPolicies.begin(); iter != runAsPolicies.end(); iter++)
        {
            ConfigParameter configParameter;
            configParameter.Name = iter->first;
            configParameter.Value = iter->second;
            configParameter.MustOverride = false;

            TestSession::FailTestIfNot(serviceManifestIter->second.ConfigPackages.size() == 1, "There should only be one config package");
            auto configSettingsIter = configSettings_.find(serviceManifestName + serviceManifestIter->second.ConfigPackages[0].Name);
            TestSession::FailTestIf(configSettingsIter == configSettings_.end(), "Config settings not found");
            auto configSectionIter = configSettingsIter->second.Sections.find(FabricTestConstants::CodePackageRunAsSection);
            if(configSectionIter == configSettingsIter->second.Sections.end())
            {
                ConfigSection configSection;
                configSection.Name = FabricTestConstants::CodePackageRunAsSection;
                configSettingsIter->second.Sections.insert(make_pair(configSection.Name, configSection));
                configSectionIter = configSettingsIter->second.Sections.find(FabricTestConstants::CodePackageRunAsSection);
            }

            auto configParamIter = configSectionIter->second.Parameters.find(configParameter.Name);
            if(configParamIter == configSectionIter->second.Parameters.end())
            {
                configSectionIter->second.Parameters.insert(make_pair(configParameter.Name, configParameter));
            }
            else
            {
                configSectionIter->second.Parameters.erase(configParamIter);
                configSectionIter->second.Parameters.insert(make_pair(configParameter.Name, configParameter));
            }
        }
    }

    return true;
}

bool ApplicationBuilder::SetHealthPolicy(StringCollection const& params)
{
    CommandLineParser parser(params, 0);

    wstring json;
    if(parser.TryGetString(L"jsonpolicy", json))
    {
        healthPolicy_ = make_shared<ApplicationHealthPolicy>();
        auto error = ApplicationHealthPolicy::FromString(json, *healthPolicy_);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to parse application health policy {0}", error);
    }
    else
    {
        return false;
    }

    return true;
}

bool ApplicationBuilder::SetNetwork(StringCollection const& params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderNetwork);
        return false;
    }

    if (params[1] == L"clearall")
    {
        networkPolicies_.clear();
        return true;
    }

    CommandLineParser parser(params, 1);

    wstring json;
    if (parser.TryGetString(L"jsondescription", json))
    {
        NetworkPoliciesDescription networkPoliciesDescription;
        auto error = NetworkPoliciesDescription::FromString(json, networkPoliciesDescription);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to parse network policies {0}", error);

        wstring const& serviceManifestName = params[1];
        networkPolicies_.insert(make_pair(serviceManifestName, networkPoliciesDescription));
    }
    else
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderNetwork);
        return false;
    }

    return true;
}

bool ApplicationBuilder::SetEndpoint(StringCollection const& params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderEndpoint);
        return false;
    }

    if (params[1] == L"clearall")
    {
        for (auto iter1 = serviceManifestImports_.begin(); iter1 != serviceManifestImports_.end(); iter1++)
        {
            iter1->second.Resources.Endpoints.clear();
        }

        return true;
    }

    wstring const& serviceManifestName = params[1];
    auto iter2 = serviceManifestImports_.find(serviceManifestName);
    TestSession::FailTestIf(iter2 == serviceManifestImports_.end(), "serviceManifestName {0} not found", serviceManifestName);

    CommandLineParser parser(params, 1);

    wstring name;
    if (!parser.TryGetString(L"name", name))
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderEndpoint);
        return false;
    }

    wstring protocolTypeStr;
    wstring endpointTypeStr;
    ProtocolType::Enum protocolType;
    EndpointType::Enum endpointType;
    parser.TryGetString(L"protocol", protocolTypeStr, L"tcp");
    parser.TryGetString(L"type", endpointTypeStr, L"input");
    protocolType = ProtocolType::GetProtocolType(protocolTypeStr);
    endpointType = EndpointType::GetEndpointType(endpointTypeStr);

    EndpointDescription endpointDescription;
    endpointDescription.Name = name;
    endpointDescription.Protocol = protocolType;
    endpointDescription.Type = endpointType;

    iter2->second.Resources.Endpoints.push_back(endpointDescription);

    return true;
}

bool ApplicationBuilder::SetParameters(StringCollection const& params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetParameters);
        return false;
    }

    wstring const & parametersArgument = params[1];
    StringCollection pairs;
    StringUtility::Split<wstring>(parametersArgument, pairs, L",");
    for (auto iter = pairs.begin(); iter != pairs.end(); iter++)
    {
        StringCollection pair;
        wstring pairString = (*iter);
        StringUtility::Split<wstring>(pairString, pair, L":");
        if (pair.size() != 2)
        {
            FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetParameters);
            return false;
        }

        applicationParameters_.insert(make_pair(pair[0], pair[1]));
    }

    return true;
}

bool ApplicationBuilder::SetServiceTemplates(StringCollection const& params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetServicePackage);
        return false;
    }

    wstring const& serviceTypeName = params[1];

    if(serviceTypeName == L"clearall")
    {
        statelessServiceTemplates_.clear();
        statefulServiceTemplates_.clear();
        return true;
    }

    CommandLineParser parser(params, 2);
    bool isStateful = parser.GetBool(L"stateful");
    PartitionScheme scheme;

    std::wstring partitionCount;
    parser.TryGetString(L"partition", partitionCount, L"1");
    std::wstring lowKey;
    parser.TryGetString(L"lowkey", lowKey, wformatString(FabricTestSessionConfig::GetConfig().ServiceDescriptorLowRange));
    std::wstring highKey;
    parser.TryGetString(L"highkey", highKey, wformatString(FabricTestSessionConfig::GetConfig().ServiceDescriptorHighRange));
    vector<wstring> names;

    if (parser.GetBool(L"singlepartition"))
    {
        scheme = PartitionScheme::Singleton;
    }
    else if (parser.GetBool(L"namedpartition"))
    {
        scheme = PartitionScheme::Named;
        std::wstring partitionNames;
        parser.TryGetString(L"partitionnames", partitionNames, L"");
        StringUtility::Split<wstring>(partitionNames, names, L";");
        StringWriter writer(partitionCount);
        writer << names.size();
    }
    else //if (parser.GetBool(L"uniformpartition"))
    {
        // Default value
        scheme = PartitionScheme::UniformInt64;
    }

    vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies;
    wstring scalingPolicy;
    parser.TryGetString(L"scalingPolicy", scalingPolicy, L"");
    if (!scalingPolicy.empty())
    {
        if (!TestFabricClient::GetServiceScalingPolicy(scalingPolicy, scalingPolicies))
        {
            return false;
        }
    }

    if(isStateful)
    {
        for (auto iter = statefulServiceTemplates_.begin(); iter != statefulServiceTemplates_.end(); iter++)
        {
            if(iter->TypeName == serviceTypeName)
            {
                statefulServiceTemplates_.erase(iter);
                break;
            }
        }

        std::wstring targetReplicaSetSize;
        parser.TryGetString(L"replica", targetReplicaSetSize, L"1");

        std::wstring minReplicaSetSize;
        parser.TryGetString(L"minreplicasetsize", minReplicaSetSize, L"1");

        std::wstring replicaRestartWaitDurationInSeconds;
        parser.TryGetString(L"replicarestartwaitduration", replicaRestartWaitDurationInSeconds, L"");

        std::wstring quorumLossWaitDurationInSeconds;
        parser.TryGetString(L"quorumlosswaitduration", quorumLossWaitDurationInSeconds, L"");

        std::wstring standByReplicaKeepDurationInSeconds;
        parser.TryGetString(L"standbyreplicakeepduration", standByReplicaKeepDurationInSeconds, L"");

        StatefulService statefulService;
        statefulService.TypeName = serviceTypeName;
        statefulService.TargetReplicaSetSize = targetReplicaSetSize;
        statefulService.MinReplicaSetSize = minReplicaSetSize;
        statefulService.Scheme = scheme;
        statefulService.PartitionCount = partitionCount;
        statefulService.LowKey = lowKey;
        statefulService.HighKey = highKey;
        statefulService.PartitionNames = names;
        statefulService.ReplicaRestartWaitDurationInSeconds = replicaRestartWaitDurationInSeconds;
        statefulService.QuorumLossWaitDurationInSeconds = quorumLossWaitDurationInSeconds;
        statefulService.StandByReplicaKeepDurationInSeconds = standByReplicaKeepDurationInSeconds;
        statefulService.ScalingPolicies = scalingPolicies;

        statefulServiceTemplates_.push_back(statefulService);
    }
    else
    {
        for (auto iter = statelessServiceTemplates_.begin(); iter != statelessServiceTemplates_.end(); iter++)
        {
            if(iter->TypeName == serviceTypeName)
            {
                statelessServiceTemplates_.erase(iter);
                break;
            }
        }

        std::wstring instanceCount;
        parser.TryGetString(L"instance", instanceCount, L"1");

        StatelessService statelessService;
        statelessService.TypeName = serviceTypeName;
        statelessService.InstanceCount = instanceCount;
        statelessService.Scheme = scheme;
        statelessService.PartitionCount = partitionCount;
        statelessService.LowKey = lowKey;
        statelessService.HighKey = highKey;
        statelessService.PartitionNames = names;
        statelessService.ScalingPolicies = scalingPolicies;

        statelessServiceTemplates_.push_back(statelessService);
    }

    return true;
}

bool ApplicationBuilder::SetDefaultService(StringCollection const& params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetServicePackage);
        return false;
    }

    wstring const& serviceName = params[1];

    if(serviceName == L"clearall")
    {
        requiredStatelessServices_.clear();
        requiredStatefulService_.clear();
        return true;
    }

    if(params.size() < 3)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetServicePackage);
        return false;
    }

    wstring const& serviceTypeName = params[2];

    CommandLineParser parser(params, 3);
    bool isStateful = parser.GetBool(L"stateful");
    PartitionScheme scheme;

    std::wstring partitionCount;
    parser.TryGetString(L"partition", partitionCount, L"1");
    std::wstring lowKey;
    parser.TryGetString(L"lowkey", lowKey, wformatString(FabricTestSessionConfig::GetConfig().ServiceDescriptorLowRange));
    std::wstring highKey;
    parser.TryGetString(L"highkey", highKey, wformatString(FabricTestSessionConfig::GetConfig().ServiceDescriptorHighRange));
    std::wstring serviceDnsName;
    parser.TryGetString(L"serviceDnsName", serviceDnsName, L"");
    vector<wstring> names;

    wstring servicePackageActivationMode;
    parser.TryGetString(L"servicePackageActivationMode", servicePackageActivationMode, L"SharedProcess");

    if (parser.GetBool(L"singlepartition"))
    {
        scheme = PartitionScheme::Singleton;
    }
    else if (parser.GetBool(L"namedpartition"))
    {
        scheme = PartitionScheme::Named;
        std::wstring partitionNames;
        parser.TryGetString(L"partitionnames", partitionNames, L"");
        StringUtility::Split<wstring>(partitionNames, names, L";");
        StringWriter writer(partitionCount);
        writer << names.size();
    }
    else //if (parser.GetBool(L"uniformpartition"))
    {
        // Default value
        scheme = PartitionScheme::UniformInt64;
    }

    vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies;
    wstring scalingPolicy;
    parser.TryGetString(L"scalingPolicy", scalingPolicy, L"");
    if (!scalingPolicy.empty())
    {
        if (!TestFabricClient::GetServiceScalingPolicy(scalingPolicy, scalingPolicies))
        {
            return false;
        }
    }

    if(isStateful)
    {
        for (auto iter = requiredStatefulService_.begin(); iter != requiredStatefulService_.end(); iter++)
        {
            if(iter->first == serviceName)
            {
                requiredStatefulService_.erase(iter);
                break;
            }
        }

        std::wstring targetReplicaSetSize;
        parser.TryGetString(L"replica", targetReplicaSetSize, L"1");

        std::wstring minReplicaSetSize;
        parser.TryGetString(L"minreplicasetsize", minReplicaSetSize, L"1");

        std::wstring placementConstraints;
        parser.TryGetString(L"constraint", placementConstraints, FabricTestDispatcher::DefaultPlacementConstraints);

        vector<ServiceModel::ServicePlacementPolicyDescription> placementPolicies;
        StringCollection policiesStrings;
        parser.GetVector(L"placementPolicies", policiesStrings);
        if (!policiesStrings.empty() && !TestFabricClient::GetPlacementPolicies(policiesStrings, placementPolicies))
        {
            return false;
        }

        vector<Reliability::ServiceLoadMetricDescription> reliabilityMetrics;
        StringCollection metricStrings;
        parser.GetVector(L"metrics", metricStrings);
        if (!metricStrings.empty() && !TestFabricClient::GetServiceMetrics(metricStrings, reliabilityMetrics, isStateful))
        {
            return false;
        }

        vector<Reliability::ServiceCorrelationDescription> serviceCorrelations;
        StringCollection correlationStrings;
        parser.GetVector(L"servicecorrelations", correlationStrings);
        if (!correlationStrings.empty() && !TestFabricClient::GetCorrelations(correlationStrings, serviceCorrelations))
        {
            return false;
        }

        std::wstring defaultMoveCost;
        parser.TryGetString(L"defaultmovecost", defaultMoveCost, L"");

        std::wstring replicaRestartWaitDurationInSeconds;
        parser.TryGetString(L"replicarestartwaitduration", replicaRestartWaitDurationInSeconds, L"");

        std::wstring quorumLossWaitDurationInSeconds;
        parser.TryGetString(L"quorumlosswaitduration", quorumLossWaitDurationInSeconds, L"");

        std::wstring standByReplicaKeepDurationInSeconds;
        parser.TryGetString(L"standbyreplicakeepduration", standByReplicaKeepDurationInSeconds, L"");

        StatefulService statefulService;
        statefulService.TypeName = serviceTypeName;
        statefulService.TargetReplicaSetSize = targetReplicaSetSize;
        statefulService.MinReplicaSetSize = minReplicaSetSize;
        statefulService.Scheme = scheme;
        statefulService.PartitionCount = partitionCount;
        statefulService.LowKey = lowKey;
        statefulService.HighKey = highKey;
        statefulService.PartitionNames = move(names);
        statefulService.ReplicaRestartWaitDurationInSeconds = replicaRestartWaitDurationInSeconds;
        statefulService.QuorumLossWaitDurationInSeconds = quorumLossWaitDurationInSeconds;
        statefulService.StandByReplicaKeepDurationInSeconds = standByReplicaKeepDurationInSeconds;
        statefulService.PlacementConstraints = placementConstraints;
        statefulService.Metrics = reliabilityMetrics;
        statefulService.PlacementPolicies = placementPolicies;
        statefulService.ServiceCorrelations = serviceCorrelations;
        statefulService.DefaultMoveCost = defaultMoveCost;
        statefulService.ServicePackageActivationMode = servicePackageActivationMode;
        statefulService.ScalingPolicies = scalingPolicies;
        statefulService.ServiceDnsName = serviceDnsName;

        requiredStatefulService_.insert(make_pair(serviceName, statefulService));
    }
    else
    {
        for (auto iter = requiredStatelessServices_.begin(); iter != requiredStatelessServices_.end(); iter++)
        {
            if(iter->first == serviceName)
            {
                requiredStatelessServices_.erase(iter);
                break;
            }
        }

        std::wstring instanceCount;
        parser.TryGetString(L"instance", instanceCount, L"1");

        std::wstring placementConstraints;
        parser.TryGetString(L"constraint", placementConstraints, FabricTestDispatcher::DefaultPlacementConstraints);

        vector<ServiceModel::ServicePlacementPolicyDescription> placementPolicies;
        StringCollection policiesStrings;
        parser.GetVector(L"placementPolicies", policiesStrings);
        if (!policiesStrings.empty() && !TestFabricClient::GetPlacementPolicies(policiesStrings, placementPolicies))
        {
            return false;
        }

        vector<Reliability::ServiceLoadMetricDescription> reliabilityMetrics;
        StringCollection metricStrings;
        parser.GetVector(L"metrics", metricStrings);
        if (!metricStrings.empty() && !TestFabricClient::GetServiceMetrics(metricStrings, reliabilityMetrics, isStateful))
        {
            return false;
        }

        vector<Reliability::ServiceCorrelationDescription> serviceCorrelations;
        StringCollection correlationStrings;
        parser.GetVector(L"servicecorrelations", correlationStrings);
        if (!correlationStrings.empty() && !TestFabricClient::GetCorrelations(correlationStrings, serviceCorrelations))
        {
            return false;
        }

        std::wstring defaultMoveCost;
        parser.TryGetString(L"defaultmovecost", defaultMoveCost, L"");

        StatelessService statelessService;
        statelessService.TypeName = serviceTypeName;
        statelessService.InstanceCount = instanceCount;
        statelessService.Scheme = scheme;
        statelessService.PartitionCount = partitionCount;
        statelessService.LowKey = lowKey;
        statelessService.HighKey = highKey;
        statelessService.PartitionNames = move(names);
        statelessService.PlacementConstraints = placementConstraints;
        statelessService.Metrics = reliabilityMetrics;
        statelessService.PlacementPolicies = placementPolicies;
        statelessService.ServiceCorrelations = serviceCorrelations;
        statelessService.DefaultMoveCost = defaultMoveCost;
        statelessService.ScalingPolicies = scalingPolicies;
        statelessService.ServiceDnsName = serviceDnsName;

        statelessService.ServicePackageActivationMode = servicePackageActivationMode;

        requiredStatelessServices_.insert(make_pair(serviceName, statelessService));
    }

    return true;
}

bool ApplicationBuilder::SetServicePackage(StringCollection const& params)
{
    if(params.size() < 2)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetServicePackage);
        return false;
    }

    wstring const& serviceManifestName = params[1];

    if(serviceManifestName == L"clearall")
    {
        serviceManifestImports_.clear();
        skipUploadCodePackages_.clear();
        skipUploadServiceManifests_.clear();
        return true;
    }
    else
    {
        auto iter = serviceManifestImports_.find(serviceManifestName);
        if (iter != serviceManifestImports_.end())
        {
            serviceManifestImports_.erase(iter);
        }

        auto iter2 = skipUploadServiceManifests_.find(serviceManifestName);
        if (iter2 != skipUploadServiceManifests_.end())
        {
            skipUploadServiceManifests_.erase(iter2);
        }

        auto iter3 = skipUploadCodePackages_.find(serviceManifestName);
        if (iter3 != skipUploadCodePackages_.end())
        {
            skipUploadCodePackages_.erase(iter3);
        }
    }

    CommandLineParser parser(params, 2);
    wstring serviceManifestVersion;
    parser.TryGetString(L"version", serviceManifestVersion, applicationTypeVersion_);
    bool skipUploadServiceManifest = parser.GetBool(L"skipupload");

    wstring configPackageName;
    wstring configPackageVersion;
    parser.TryGetString(L"configname", configPackageName, L"Config");
    parser.TryGetString(L"configversion", configPackageVersion, serviceManifestVersion);
    
    wstring dataPackageName;
    wstring dataPackageVersion;
    parser.TryGetString(L"dataname", dataPackageName, L"Data");
    parser.TryGetString(L"dataversion", dataPackageVersion, serviceManifestVersion);

    ServiceManifestDescription serviceManifestDescription;
    serviceManifestDescription.Name = serviceManifestName;
    serviceManifestDescription.Version = serviceManifestVersion;

    wstring resources;
    Common::StringCollection metricCollection;

    std::map<std::wstring, std::wstring> spRGmap;

    if (parser.GetVector(L"resources", metricCollection))
    {
        if ((metricCollection.size() % 2) != 0)
        {
            TestSession::WriteError(
                TraceSource,
                "Invalid specification of resource governance metrics.");
            return false;
        }

        size_t metricCount = metricCollection.size() / 2;
        for (size_t metricIndex = 0; metricIndex < metricCount; ++metricIndex)
        {
            if (metricCollection[metricIndex * 2] == L"CPU")
            {
                spRGmap.insert(make_pair(L"CpuCores", metricCollection[metricIndex * 2 + 1]));
            }
            else if (metricCollection[metricIndex * 2] == L"MemoryInMB")
            {
                spRGmap.insert(make_pair(L"MemoryInMB", metricCollection[metricIndex * 2 + 1]));
            }

        }
    }

    spRGPolicies_.insert(make_pair(serviceManifestName, move(spRGmap)));

    // Add ConfigPackageDescription
    ConfigPackageDescription configPackage;
    configPackage.Name = configPackageName;
    configPackage.Version = configPackageVersion;

    ConfigSection configSection;
    configSection.Name = FabricTestConstants::SupportedServiceTypesSection;

    ConfigSettings configSettings;
    configSettings.Sections.insert(make_pair(configSection.Name, configSection));

    configSettings_.insert(make_pair(serviceManifestDescription.Name + configPackage.Name, configSettings));
    serviceManifestDescription.ConfigPackages.push_back(configPackage);

    // Add DataPackageDescriptions
    bool skipDataPackage = parser.GetBool(L"skipData");
    if (!skipDataPackage)
    {
        DataPackageDescription dataPackage;
        dataPackage.Name = dataPackageName;
        dataPackage.Version = dataPackageVersion;

        serviceManifestDescription.DataPackages.push_back(dataPackage);
    }

    serviceManifestImports_.insert(make_pair(serviceManifestDescription.Name, move(serviceManifestDescription)));

    if (skipUploadServiceManifest)
    {
        skipUploadServiceManifests_.insert(serviceManifestName);
    }

    return true;
}

bool ApplicationBuilder::SetServicetypes(StringCollection const& params)
{
    if(params.size() < 3)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetServiceTypes);
        return false;
    }

    wstring const& serviceManifestName = params[1];
    wstring const& serviceTypeName = params[2];

    auto iter = serviceManifestImports_.find(serviceManifestName);
    if(iter == serviceManifestImports_.end())
    {
        TestSession::WriteError(TraceSource, "serviceManifestName {0} does not exist.", serviceManifestName);
        return false;
    }

    ServiceManifestDescription & serviceManifestDescription = iter->second;
    if(serviceTypeName == L"clearall")
    {
        serviceManifestDescription.CodePackages.clear();
        return true;
    }
    else
    {
        for (auto serviceTypesIter = serviceManifestDescription.ServiceTypeNames.begin(); serviceTypesIter != serviceManifestDescription.ServiceTypeNames.end(); serviceTypesIter++)
        {
            if(serviceTypesIter->ServiceTypeName == serviceTypeName)
            {
                serviceManifestDescription.ServiceTypeNames.erase(serviceTypesIter);
                break;
            }
        }
    }

    CommandLineParser parser(params, 3);

    bool isStateful = parser.GetBool(L"stateful");
    bool hasPersistedState = parser.GetBool(L"persist");
    bool isImplicitType = parser.GetBool(L"implicit");

    wstring placementConstraints;
    parser.TryGetString(L"constraint", placementConstraints, FabricTestDispatcher::DefaultPlacementConstraints);

    vector<Reliability::ServiceLoadMetricDescription> reliabilityMetrics;
    StringCollection metricStrings;
    parser.GetVector(L"metrics", metricStrings);
    if(!metricStrings.empty() && !TestFabricClient::GetServiceMetrics(metricStrings, reliabilityMetrics, isStateful))
    {
        return false;
    }

    ServiceTypeDescription serviceTypeDescription;
    serviceTypeDescription.ServiceTypeName = serviceTypeName;
    serviceTypeDescription.IsStateful = isStateful;
    serviceTypeDescription.HasPersistedState = hasPersistedState;
    serviceTypeDescription.PlacementConstraints = placementConstraints;

    serviceTypeDescription.UseImplicitHost = isImplicitType;

    vector<ServiceModel::ServiceLoadMetricDescription> serviceModelMetrics;
    for (auto metricsIter = reliabilityMetrics.begin(); metricsIter != reliabilityMetrics.end(); ++metricsIter)
    {
        ServiceModel::ServiceLoadMetricDescription metric;

        metric.PrimaryDefaultLoad = metricsIter->PrimaryDefaultLoad;
        metric.SecondaryDefaultLoad = metricsIter->SecondaryDefaultLoad;
        metric.Name = metricsIter->Name;
        metric.Weight = WeightType::FromPublicApi(metricsIter->Weight);

        serviceModelMetrics.push_back(move(metric));
    }

    serviceTypeDescription.LoadMetrics.swap(serviceModelMetrics);

    serviceManifestDescription.ServiceTypeNames.push_back(serviceTypeDescription);

    return true;
}

bool ApplicationBuilder::SetCodePackage(
    StringCollection const& params,
    wstring const & clientCredentialsType)
{
    if(params.size() < 3)
    {
        FABRICSESSION.PrintHelp(ApplicationBuilder::ApplicationBuilderSetCodePackage);
        return false;
    }

    wstring const& serviceManifestName = params[1];
    wstring const& codePackageName = params[2];

    auto iter = serviceManifestImports_.find(serviceManifestName);
    if(iter == serviceManifestImports_.end())
    {
        TestSession::WriteError(TraceSource, "serviceManifestName {0} does not exist.", serviceManifestName);
        return false;
    }

    ServiceManifestDescription & serviceManifestDescription = iter->second;
    if(codePackageName == L"clearall")
    {
        serviceManifestDescription.CodePackages.clear();
        rgPolicies_.clear();
        return true;
    }
    else
    {
        for (auto codePackageIter = serviceManifestDescription.CodePackages.begin(); codePackageIter != serviceManifestDescription.CodePackages.end(); codePackageIter++)
        {
            if(codePackageIter->Name == codePackageName)
            {
                serviceManifestDescription.CodePackages.erase(codePackageIter);
                break;
            }
        }

        auto skipIter = skipUploadCodePackages_.find(serviceManifestName);
        if (skipIter != skipUploadCodePackages_.end())
        {
            auto iter2 = skipIter->second.find(codePackageName);
            if (iter2 != skipIter->second.end())
            {
                skipIter->second.erase(iter2);
            }
        }
    }

    CommandLineParser parser(params, 3);

    wstring version;
    parser.TryGetString(L"version", version, serviceManifestDescription.Version);    
    wstring entryPointType;
    parser.TryGetString(L"entrytype", entryPointType, L"Exe");    
    wstring isolationPolicyType;
    parser.TryGetString(L"isolationtype", isolationPolicyType, L"DedicatedProcess");    
    bool isShared = parser.GetBool(L"isshared");
    auto isGuestExe = parser.GetBool(L"isGuestExe");

    wstring supportedTypes;
    parser.TryGetString(L"types", supportedTypes);

    CodePackageDescription codePackageDescription;
    codePackageDescription.Name = codePackageName;
    codePackageDescription.Version = version;
    codePackageDescription.IsShared = isShared;
    bool skipUploadCodePackage = parser.GetBool(L"skipupload");

    CodePackageIsolationPolicyType::Enum isolationPolicy = CodePackageIsolationPolicyType::DedicatedProcess;
    if(!CodePackageIsolationPolicyType::TryParseFromString(isolationPolicyType, isolationPolicy))
    {
        TestSession::WriteError(TraceSource, "IsolationPolicyType {0} does not exist.", isolationPolicyType);
        return false;
    }

    DllHostHostedDllKind::Enum assemblyType = DllHostHostedDllKind::Unmanaged;
    if (entryPointType == L"Dll")
    { 
        entryPointType = L"DllHost";
        assemblyType = DllHostHostedDllKind::Unmanaged;
    }

    if (entryPointType == L".Net")
    { 
        entryPointType = L"DllHost";
        assemblyType = DllHostHostedDllKind::Managed;
    }

    if(!EntryPointType::TryParseFromString(entryPointType, codePackageDescription.EntryPoint.EntryPointType))
    {
        TestSession::WriteError(TraceSource, "entryPointType {0} does not exist.", entryPointType);
        return false;
    }

    if (codePackageDescription.EntryPoint.EntryPointType == EntryPointType::Exe)
    {
        wstring program = L"FabricTestHost.exe";
        wstring arguments = wformatString(
            "version={0} testdir={1} serverport={2} useetw={3} security={4} isGuestExe={5}", 
            version, 
            FabricTestDispatcher::TestDataDirectory,
            FederationTestCommon::AddressHelper::ServerListenPort,
            FabricTestSessionConfig::GetConfig().UseEtw,
            clientCredentialsType,
            isGuestExe);

        codePackageDescription.EntryPoint.ExeEntryPoint.Program = program;
        codePackageDescription.EntryPoint.ExeEntryPoint.Arguments = arguments;
    }
    else if (codePackageDescription.EntryPoint.EntryPointType == EntryPointType::ContainerHost)
    {
        wstring containerImage;
        wstring containerEntrypoint;
#if defined (PLATFORM_UNIX)
        parser.TryGetString(L"containerimage", containerImage, L"ubuntu");
        parser.TryGetString(L"containerentrypoint", containerEntrypoint, L"tail,-f,/dev/null");
#else
        parser.TryGetString(L"containerimage", containerImage, L"microsoft/nanoserver:sac2016");
        parser.TryGetString(L"containerentrypoint", containerEntrypoint, L"powershell.exe,Start-Sleep -s 10000");
#endif
        codePackageDescription.EntryPoint.ContainerEntryPoint.ImageName = containerImage;
        codePackageDescription.EntryPoint.ContainerEntryPoint.EntryPoint = containerEntrypoint;
        containerHosts_[serviceManifestName].insert(codePackageName);
    }
    else
    {
        wstring entryPoint;
        if(assemblyType == DllHostHostedDllKind::Unmanaged)
        {
            parser.TryGetString(L"entry", entryPoint, L"FabricTestHost.exe");
        }
        else
        {
            parser.TryGetString(L"entry", entryPoint, L"FabricTestHost");
        }

        ServiceModel::DllHostHostedDllDescription dll;
        dll.Kind = assemblyType;
        if (dll.Kind == DllHostHostedDllKind::Unmanaged)
        {
            dll.Unmanaged.DllName = entryPoint;
        }
        else 
        {
            dll.Managed.AssemblyName = entryPoint;
        }

        codePackageDescription.EntryPoint.DllHostEntryPoint.IsolationPolicyType = isolationPolicy;
        codePackageDescription.EntryPoint.DllHostEntryPoint.HostedDlls.push_back(dll);
    }

    // Parse RG policies for this code package.
    wstring rgPolicies;
    parser.TryGetString(L"rgpolicies", rgPolicies, L"");
    if (rgPolicies != L"")
    {
        auto rgPolicyIterator = rgPolicies_.find(serviceManifestName);
        if (rgPolicyIterator == rgPolicies_.end())
        {
            rgPolicyIterator = rgPolicies_.insert(make_pair(serviceManifestName, map<wstring, wstring>())).first;
        }
        rgPolicyIterator->second.insert(make_pair(codePackageName, move(rgPolicies)));
    }

    serviceManifestDescription.CodePackages.push_back(codePackageDescription);

    if (skipUploadCodePackage)
    {
        auto skipIter = skipUploadCodePackages_.find(serviceManifestName);
        if (skipIter == skipUploadCodePackages_.end())
        {
            skipIter = skipUploadCodePackages_.insert(make_pair(serviceManifestName, set<wstring>())).first;
        }

        skipIter->second.insert(codePackageName);
    }

    ConfigParameter configParameter;
    configParameter.Name = codePackageName;
    configParameter.Value = supportedTypes;
    configParameter.MustOverride = false;

    TestSession::FailTestIfNot(serviceManifestDescription.ConfigPackages.size() == 1, "There should only be one config package");
    auto configSettingsIter = configSettings_.find(serviceManifestDescription.Name + serviceManifestDescription.ConfigPackages[0].Name);
    TestSession::FailTestIf(configSettingsIter == configSettings_.end(), "Config settings not found");
    auto configSectionIter = configSettingsIter->second.Sections.find(FabricTestConstants::SupportedServiceTypesSection);
    TestSession::FailTestIf(configSectionIter == configSettingsIter->second.Sections.end(), "Config section not found");

    auto configParamIter = configSectionIter->second.Parameters.find(configParameter.Name);
    if(configParamIter == configSectionIter->second.Parameters.end())
    {
        configSectionIter->second.Parameters.insert(make_pair(configParameter.Name, configParameter));
    }
    else
    {
        configSectionIter->second.Parameters.erase(configParamIter);
        configSectionIter->second.Parameters.insert(make_pair(configParameter.Name, configParameter));
    }

    return true;
}

bool ApplicationBuilder::UploadApplication(
    std::wstring const& applicationFriendlyName,
    Common::StringCollection const& params,
    ComPointer<IFabricNativeImageStoreClient> const & imageStoreClientCPtr)
{
    wstring pathToIncoming = Path::Combine(Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestImageStoreDirectory), ApplicationPackageFolderInImageStore);
    if(!Directory::Exists(pathToIncoming))
    {
        Directory::Create(pathToIncoming);
    }

    wstring pathToApp = Path::Combine(pathToIncoming, applicationFriendlyName);
    wstring pathToAppManifest = Path::Combine(pathToApp, L"\\ApplicationManifest.xml");
    wstring applicationManifest;
    GetApplicationManifestString(applicationManifest);
    if(Directory::Exists(pathToApp))
    {
        TestSession::WriteError(TraceSource, "Cannot upload app type {0} twice.", applicationTypeName_);
        return false;
    }

    Directory::Create(pathToApp);
    Common::FileWriter appManifestFileWriter;
    auto error = appManifestFileWriter.TryOpen(pathToAppManifest);
    if (!error.IsSuccess())
    {
        TestSession::WriteError(TraceSource, "Failed to open app manifest {0}: {1}", pathToAppManifest, error);
        return false;
    }

    std::string applicationManifesResult;
    StringUtility::UnicodeToAnsi(applicationManifest, applicationManifesResult);
    appManifestFileWriter << applicationManifesResult;

    for (auto iter = serviceManifestImports_.begin(); iter != serviceManifestImports_.end(); iter++)
    {
        auto itSkippedServiceManifest = skipUploadServiceManifests_.find(iter->first);
        if (itSkippedServiceManifest != skipUploadServiceManifests_.end())
        {
            // Do not upload anything for this service manifest
            TestSession::WriteInfo(TraceSource, "Skip upload for service manifest {0}.", iter->first);
            continue;
        }

        wstring pathToServicePackage = Path::Combine(pathToApp, iter->second.Name);
        wstring pathToServiceManifest = Path::Combine(pathToServicePackage, L"\\ServiceManifest.xml");
        Directory::Create(pathToServicePackage);
        wstring serviceManifest;
        GetServiceManifestString(serviceManifest, iter->second);
        Common::FileWriter serviceManifestFileWriter;
        error = serviceManifestFileWriter.TryOpen(pathToServiceManifest);

        if (!error.IsSuccess())
        {
            TestSession::WriteError(TraceSource, "Failed to open service manifest {0}: {1}", pathToServiceManifest, error);
            return false;
        }

        std::string serviceManifesResult;
        StringUtility::UnicodeToAnsi(serviceManifest, serviceManifesResult);
        serviceManifestFileWriter << serviceManifesResult;

        auto iterSkippedServiceManifestCodePackages = skipUploadCodePackages_.find(iter->first);
        bool hasSkippedCodePackages = (iterSkippedServiceManifestCodePackages != skipUploadCodePackages_.end());

        for (auto codePackageIter = iter->second.CodePackages.begin(); codePackageIter != iter->second.CodePackages.end(); codePackageIter++)
        {
            if (hasSkippedCodePackages)
            {
                auto iterSkippedCodePackage = iterSkippedServiceManifestCodePackages->second.find(codePackageIter->Name);
                if (iterSkippedCodePackage != iterSkippedServiceManifestCodePackages->second.end())
                {
                    // Do not upload the code package
                    TestSession::WriteInfo(TraceSource, "Skip upload for service manifest {0}, code package {1}.", iter->first, codePackageIter->Name);
                    continue;
                }
            }

            wstring pathToCodePackage = Path::Combine(pathToServicePackage, codePackageIter->Name);
            Directory::Create(pathToCodePackage);
            File::Copy(Path::Combine(Environment::GetExecutablePath(), L"FabricTestHost.exe"), Path::Combine(pathToCodePackage, L"\\FabricTestHost.exe"));            
        }

        for (auto configPackageIter = iter->second.ConfigPackages.begin(); configPackageIter != iter->second.ConfigPackages.end(); configPackageIter++)
        {
            wstring pathToConfigPackage = Path::Combine(pathToServicePackage, configPackageIter->Name);
            Directory::Create(pathToConfigPackage);
            wstring key = iter->second.Name + configPackageIter->Name;
            auto configSettingsIter = configSettings_.find(key);

            wstring settingsXml;
            StringWriter settingsXmlWriter(settingsXml);
            settingsXmlWriter.WriteLine("<?xml version='1.0' encoding='UTF-8'?>");
            settingsXmlWriter.WriteLine("<Settings xsi:schemaLocation='http://schemas.microsoft.com/2011/01/fabric ServiceFabricServiceModel.xsd' xmlns='http://schemas.microsoft.com/2011/01/fabric' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'>");
            if(configSettingsIter != configSettings_.end())
            {
                for (auto configSectionIter = configSettingsIter->second.Sections.begin(); configSectionIter != configSettingsIter->second.Sections.end(); configSectionIter++)
                {
                    settingsXmlWriter.WriteLine("<Section Name='{0}'>", configSectionIter->first);
                    for (auto configParamIter = configSectionIter->second.Parameters.begin(); configParamIter != configSectionIter->second.Parameters.end(); configParamIter++)
                    {
                        settingsXmlWriter.WriteLine("<Parameter Name='{0}' Value='{1}' MustOverride='{2}'/>", configParamIter->second.Name, configParamIter->second.Value, configParamIter->second.MustOverride);
                    }
                    settingsXmlWriter.WriteLine("</Section>");
                }

                if (!defaultRunAsPolicy_.empty())
                {
                    settingsXmlWriter.WriteLine("<Section Name='{0}'>", FabricTestConstants::DefaultRunAsSection);
                    settingsXmlWriter.WriteLine("<Parameter Name='User' Value='{0}' MustOverride='false'/>", defaultRunAsPolicy_);
                    settingsXmlWriter.WriteLine("</Section>");
                }
            }
            settingsXmlWriter.WriteLine(L"</Settings>");

            auto settingsFilename = Path::Combine(pathToConfigPackage, L"\\Settings.xml");
            Common::FileWriter settingsFileWriter;
            error = settingsFileWriter.TryOpen(settingsFilename);
            if (!error.IsSuccess())
            {
                TestSession::WriteError(TraceSource, "Failed to open settings file {0}: {1}", settingsFilename, error);
                return false;
            }
            std::string settingsXmlResult;
            StringUtility::UnicodeToAnsi(settingsXml, settingsXmlResult);
            settingsFileWriter << settingsXmlResult;
        }

        for (auto dataPackageIter = iter->second.DataPackages.begin(); dataPackageIter != iter->second.DataPackages.end(); dataPackageIter++)
        {
            wstring pathToDataPackage = Path::Combine(pathToServicePackage, dataPackageIter->Name);
            Directory::Create(pathToDataPackage);
        }
    }

    appManifestFileWriter.Close();

#if !defined(PLATFORM_UNIX)
    CommandLineParser parser(params, 1);
    if (parser.GetBool(L"compress"))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Compressing application package at {0}",
            pathToApp);

        error = Management::ImageStore::ImageStoreUtility::ArchiveApplicationPackage(
            pathToApp, 
            Api::INativeImageStoreProgressEventHandlerPtr());
        TestSession::FailTestIfNot(error.IsSuccess(), "ArchiveApplicationPackage failed: error = {0}", error);
    }
#endif

    if (imageStoreClientCPtr)
    {

        wstring remoteDestination = Path::Combine(ApplicationPackageFolderInImageStore, applicationFriendlyName);

        auto hr = imageStoreClientCPtr->UploadContent(
            remoteDestination.c_str(),
            pathToApp.c_str(),
            this->GetComTimeout(ServiceModelConfig::GetConfig().MaxFileOperationTimeout),
            FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC);

        TestSession::WriteInfo(
            TraceSource,
            "Upload to native image store '{0}' -> '{1}': hr = {2} ({3})",
            pathToApp,
            remoteDestination,
            hr,
            ErrorCode::FromHResult(hr));

        TestSession::FailTestIfNot(SUCCEEDED(hr), "UploadContent failed: hr = {0}", hr);
    }

    return true;
}

bool ApplicationBuilder::DeleteApplication(std::wstring const& applicationFriendlyName)
{
    wstring pathToIncoming = Path::Combine(Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestImageStoreDirectory), L"incoming");   
    wstring pathToApp = Path::Combine(pathToIncoming, applicationFriendlyName);

    if(Directory::Exists(pathToApp))
    {
        ErrorCode error = Directory::Delete(pathToApp, true);
        if(!error.IsSuccess())
        {
            TestSession::WriteError(TraceSource, "Could not delete {0}.", pathToApp);
            return false;
        }
    }

    auto iter = applicationTypes_.find(applicationFriendlyName);
    if(iter != applicationTypes_.end())
    {
        applicationTypes_.erase(iter);
    }
    else
    {
        return false;
    }

    return true;
}

void ApplicationBuilder::GetServiceManifestString(wstring & serviceManifest, ServiceManifestDescription & serviceManifestDescription)
{
    StringWriter serviceManifestWriter(serviceManifest);
    serviceManifestWriter.WriteLine("<?xml version='1.0' encoding='UTF-8'?>");
    serviceManifestWriter.WriteLine(
        "<ServiceManifest Name='{0}' Version='{1}' xsi:schemaLocation='http://schemas.microsoft.com/2011/01/fabric ServiceFabricServiceModel.xsd' xmlns='http://schemas.microsoft.com/2011/01/fabric' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'>",
        serviceManifestDescription.Name,
        serviceManifestDescription.Version);

    serviceManifestWriter.WriteLine(L"<ServiceTypes>");
    for (auto iter = serviceManifestDescription.ServiceTypeNames.begin(); iter != serviceManifestDescription.ServiceTypeNames.end(); iter++)
    {
        if(iter->IsStateful)
        {
            serviceManifestWriter.Write("<StatefulServiceType ServiceTypeName='{0}' HasPersistedState='{1}'",
                iter->ServiceTypeName, iter->HasPersistedState);
            if (iter->UseImplicitHost)
            {
                serviceManifestWriter.WriteLine(L" UseImplicitHost='true'>");
            }
            else
            {
                serviceManifestWriter.WriteLine(L">");
            }
        }
        else
        {
            serviceManifestWriter.Write("<StatelessServiceType ServiceTypeName='{0}'", 
                iter->ServiceTypeName);
            if (iter->UseImplicitHost)
            {
                serviceManifestWriter.WriteLine(L" UseImplicitHost='true'>");
            }
            else
            {
                serviceManifestWriter.WriteLine(L">");
            }
        }

        if(!iter->LoadMetrics.empty())
        {
            serviceManifestWriter.WriteLine("<LoadMetrics>");

            for (auto metric = iter->LoadMetrics.begin(); metric != iter->LoadMetrics.end(); ++metric)
            {
                serviceManifestWriter.WriteLine("<LoadMetric Name='{0}' PrimaryDefaultLoad='{1}' SecondaryDefaultLoad='{2}' Weight='{3}' />",
                    metric->Name,
                    metric->PrimaryDefaultLoad,
                    metric->SecondaryDefaultLoad,
                    metric->Weight);
            } 

            serviceManifestWriter.WriteLine("</LoadMetrics>");
        }

        if(!iter->PlacementConstraints.empty())
        {
            auto xml = iter->PlacementConstraints;
            StringUtility::Replace<wstring>(xml, L"&", L"&amp;");
            StringUtility::Replace<wstring>(xml, L"<", L"&lt;");
            StringUtility::Replace<wstring>(xml, L">", L"&gt;");
            StringUtility::Replace<wstring>(xml, L"\"", L"&quot;");
            StringUtility::Replace<wstring>(xml, L"'", L"&apos;");

            serviceManifestWriter.WriteLine("<PlacementConstraints>{0}</PlacementConstraints>", xml);
        }

        if(iter->IsStateful)
        {
            serviceManifestWriter.WriteLine("</StatefulServiceType>");
        }
        else
        {
            serviceManifestWriter.WriteLine("</StatelessServiceType>");
        }

        // Do not support load metrics, placement constraints and extensions as of now
    }
    serviceManifestWriter.WriteLine(L"</ServiceTypes>");

    for (auto iter = serviceManifestDescription.CodePackages.begin(); iter != serviceManifestDescription.CodePackages.end(); iter++)
    {
        serviceManifestWriter.WriteLine("<CodePackage Name='{0}' Version='{1}' IsShared='{2}'>",
            iter->Name,
            iter->Version,
            iter->IsShared);

        serviceManifestWriter.WriteLine("<EntryPoint>");
        if (iter->EntryPoint.EntryPointType == ServiceModel::EntryPointType::Exe)
        {
            serviceManifestWriter.WriteLine("<ExeHost>");
            serviceManifestWriter.WriteLine("<Program>{0}</Program>", iter->EntryPoint.ExeEntryPoint.Program);
            if (!iter->EntryPoint.ExeEntryPoint.Arguments.empty())
            {
                serviceManifestWriter.WriteLine("<Arguments>{0}</Arguments>", iter->EntryPoint.ExeEntryPoint.Arguments);
            }
            serviceManifestWriter.WriteLine("</ExeHost>");
        }
        else if (iter->EntryPoint.EntryPointType == ServiceModel::EntryPointType::ContainerHost)
        {
            serviceManifestWriter.WriteLine("<ContainerHost>");
            serviceManifestWriter.WriteLine("<ImageName>{0}</ImageName>", iter->EntryPoint.ContainerEntryPoint.ImageName);
            if (!iter->EntryPoint.ContainerEntryPoint.EntryPoint.empty())
            {
                serviceManifestWriter.WriteLine("<EntryPoint>{0}</EntryPoint>", iter->EntryPoint.ContainerEntryPoint.EntryPoint);
            }
            serviceManifestWriter.WriteLine("</ContainerHost>");
        }
        else if (iter->EntryPoint.EntryPointType == ServiceModel::EntryPointType::DllHost)
        {
            serviceManifestWriter.WriteLine("<DllHost IsolationPolicy='{0}'>", iter->EntryPoint.DllHostEntryPoint.IsolationPolicyType);

            for (auto it2 = begin(iter->EntryPoint.DllHostEntryPoint.HostedDlls); it2 != end(iter->EntryPoint.DllHostEntryPoint.HostedDlls); ++it2)
            {
                if (it2->Kind == DllHostHostedDllKind::Unmanaged)
                {
                    serviceManifestWriter.WriteLine("<UnmanagedDll>{0}</UnmanagedDll>", it2->Unmanaged.DllName);
                }
                else if (it2->Kind == DllHostHostedDllKind::Managed)
                {
                    serviceManifestWriter.WriteLine("<ManagedAssembly>{0}</ManagedAssembly>", it2->Managed.AssemblyName);
                }
            }
            serviceManifestWriter.WriteLine("</DllHost>");
        }
        serviceManifestWriter.WriteLine("</EntryPoint>");

        serviceManifestWriter.WriteLine("</CodePackage>");
    }

    for (auto iter = serviceManifestDescription.ConfigPackages.begin(); iter != serviceManifestDescription.ConfigPackages.end(); iter++)
    {
        serviceManifestWriter.WriteLine("<ConfigPackage Name='{0}' Version='{1}'></ConfigPackage>",
            iter->Name,
            iter->Version);
    }

    for (auto iter = serviceManifestDescription.DataPackages.begin(); iter != serviceManifestDescription.DataPackages.end(); iter++)
    {
        serviceManifestWriter.WriteLine("<DataPackage Name='{0}' Version='{1}'></DataPackage>",
            iter->Name,
            iter->Version);
    }

    if (serviceManifestDescription.Resources.Endpoints.size() > 0)
    {
        serviceManifestWriter.WriteLine(L"<Resources>");
        serviceManifestWriter.WriteLine(L"<Endpoints>");
        for (auto iter = serviceManifestDescription.Resources.Endpoints.begin(); iter != serviceManifestDescription.Resources.Endpoints.end(); iter++)
        {
            serviceManifestWriter.WriteLine("<Endpoint Name='{0}' Protocol='{1}' Type='{2}' />", iter->Name, ProtocolType::EnumToString(iter->Protocol), EndpointType::EnumToString(iter->Type));
        }
        serviceManifestWriter.WriteLine(L"</Endpoints>");
        serviceManifestWriter.WriteLine(L"</Resources>");
    }

    serviceManifestWriter.WriteLine(L"</ServiceManifest>");
}

void ApplicationBuilder::GetApplicationManifestString(wstring & applicationManifest)
{
    StringWriter appManifestWriter(applicationManifest);
    appManifestWriter.WriteLine("<?xml version='1.0' encoding='UTF-8'?>");
    appManifestWriter.WriteLine(
        "<ApplicationManifest ApplicationTypeName='{0}' ApplicationTypeVersion='{1}' xsi:schemaLocation='http://schemas.microsoft.com/2011/01/fabric ServiceFabricServiceModel.xsd' xmlns='http://schemas.microsoft.com/2011/01/fabric' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'>",
        applicationTypeName_,
        applicationTypeVersion_);

    appManifestWriter.WriteLine(L"<Parameters>");
    for (auto iter = applicationParameters_.begin(); iter != applicationParameters_.end(); iter++)
    {
        appManifestWriter.WriteLine("<Parameter Name='{0}' DefaultValue='{1}'></Parameter>",
            iter->first,
            iter->second);
    }
    appManifestWriter.WriteLine(L"</Parameters>");

    for (auto iter = serviceManifestImports_.begin(); iter != serviceManifestImports_.end(); iter++)
    {
        appManifestWriter.WriteLine(L"<ServiceManifestImport>");
        appManifestWriter.WriteLine("<ServiceManifestRef ServiceManifestName='{0}' ServiceManifestVersion='{1}'/>",
            iter->second.Name,
            iter->second.Version);

        auto runAsPoliciesIter = runAsPolicies_.find(iter->second.Name);
        auto packageSharingIter = sharedPackagePolicies_.find(iter->second.Name);
        auto rgPoliciesIter = rgPolicies_.find(iter->second.Name);
        auto spRGPoliciesIter = spRGPolicies_.find(iter->second.Name);
        auto containerHostIter = containerHosts_.find(iter->second.Name);
        auto networkPoliciesIter = networkPolicies_.find(iter->second.Name);
        auto repositoryAccountName = "sftestcontainerreg";
        auto repositoryAccountPassword = "";

        if(runAsPoliciesIter != runAsPolicies_.end() ||
            packageSharingIter != sharedPackagePolicies_.end() ||
            rgPoliciesIter != rgPolicies_.end() ||
            spRGPoliciesIter != spRGPolicies_.end() ||
            containerHostIter != containerHosts_.end() ||
            networkPoliciesIter != networkPolicies_.end())
        {
            appManifestWriter.WriteLine(L"<Policies>");

            if (spRGPoliciesIter != spRGPolicies_.end())
            {

                if (spRGPoliciesIter->second.size() > 0)
                {
                    appManifestWriter.Write("<ServicePackageResourceGovernancePolicy");
                    for (auto rgPair : spRGPoliciesIter->second)
                    {
                        appManifestWriter.Write(" {0}=\"{1}\"", rgPair.first, rgPair.second);
                    }
                    appManifestWriter.Write(" />");
                    appManifestWriter.WriteLine();
                }
            }

            if (rgPoliciesIter != rgPolicies_.end())
            {
                for (auto cpRGPair : rgPoliciesIter->second)
                {
                    wstring rgPolicyString = L"";

                    StringCollection rgCollection;
                    StringUtility::Split<wstring>(cpRGPair.second, rgCollection, L";");

                    TestSession::FailTestIfNot(rgCollection.size() > 0, "Invalid RG policies provided for code packege {0}.", cpRGPair.first);
                    TestSession::FailTestIfNot(rgCollection.size() %2 == 0, "Invalid RG policies provided for code packege {0}.", cpRGPair.first);

                    auto numPolicies = rgCollection.size() / 2;

                    for (auto index = 0; index < numPolicies; ++index)
                    {
                        rgPolicyString += L" " + rgCollection[2 * index] + L"=\"" + rgCollection[2 * index + 1] + L"\"";
                    }

                    appManifestWriter.WriteLine("<ResourceGovernancePolicy CodePackageRef=\"{0}\" {1} />",
                        cpRGPair.first,
                        rgPolicyString);
                }
            }
            if(runAsPoliciesIter != runAsPolicies_.end())
            {
                for (auto policies = runAsPoliciesIter->second.begin(); policies != runAsPoliciesIter->second.end(); policies++)
                {
                    appManifestWriter.WriteLine("<RunAsPolicy CodePackageRef='{0}' UserRef='{1}'/>",
                        policies->first,
                        policies->second);
                }
            }
            if(packageSharingIter != sharedPackagePolicies_.end())
            {
                for(auto policies = packageSharingIter->second.begin(); policies != packageSharingIter->second.end(); policies++)
                {
                    appManifestWriter.WriteLine("<PackageSharingPolicy PackageRef='{0}' />", *policies);
                }
            }
            if (containerHostIter != containerHosts_.end())
            {
                for (auto itContainerHost = containerHostIter->second.begin(); itContainerHost != containerHostIter->second.end(); ++itContainerHost)
                {
                    appManifestWriter.WriteLine("<ContainerHostPolicies CodePackageRef='{0}'>", *itContainerHost);
                    appManifestWriter.WriteLine("<RepositoryCredentials AccountName='{0}' Password='{1}' />",
                        repositoryAccountName, repositoryAccountPassword);
                    appManifestWriter.WriteLine("</ContainerHostPolicies>");
                }
            }
            if (networkPoliciesIter != networkPolicies_.end())
            {
                appManifestWriter.WriteLine(L"<NetworkPolicies>");
                for (auto containerNetworkPoliciesIter = networkPoliciesIter->second.ContainerNetworkPolicies.begin(); containerNetworkPoliciesIter != networkPoliciesIter->second.ContainerNetworkPolicies.end(); containerNetworkPoliciesIter++)
                {
                    appManifestWriter.WriteLine("<ContainerNetworkPolicy NetworkRef='{0}'>", containerNetworkPoliciesIter->NetworkRef);
                    for (auto endpointBindingsIter = containerNetworkPoliciesIter->EndpointBindings.begin(); endpointBindingsIter != containerNetworkPoliciesIter->EndpointBindings.end(); endpointBindingsIter++)
                    {
                        appManifestWriter.WriteLine("<EndpointBinding EndpointRef='{0}' />", endpointBindingsIter->EndpointRef);
                    }

                    appManifestWriter.WriteLine("</ContainerNetworkPolicy>");
                }
                appManifestWriter.WriteLine(L"</NetworkPolicies>");
            }
            appManifestWriter.WriteLine(L"</Policies>");
        }

        // No config overrides support yet
        appManifestWriter.WriteLine(L"</ServiceManifestImport>");
    }

    if(statelessServiceTemplates_.size() > 0 || statefulServiceTemplates_.size() > 0)
    {
        appManifestWriter.WriteLine(L"<ServiceTemplates>");
        for (auto iter = statelessServiceTemplates_.begin(); iter != statelessServiceTemplates_.end(); iter++)
        {
            WriteStatelessServiceType(appManifestWriter, *iter);
        }

        for (auto iter = statefulServiceTemplates_.begin(); iter != statefulServiceTemplates_.end(); iter++)
        {
            WriteStatefulServiceType(appManifestWriter, *iter);
        }
        appManifestWriter.WriteLine(L"</ServiceTemplates>");
    }

    if(requiredStatelessServices_.size() > 0 || requiredStatefulService_.size() > 0)
    {
        appManifestWriter.WriteLine(L"<DefaultServices>");
        for (auto iter = requiredStatelessServices_.begin(); iter != requiredStatelessServices_.end(); iter++)
        {
            if (!iter->second.ServiceDnsName.empty())
            {
                appManifestWriter.WriteLine("<Service Name='{0}' ServicePackageActivationMode='{1}' ServiceDnsName='{2}'>", iter->first, iter->second.ServicePackageActivationMode, iter->second.ServiceDnsName);
            }
            else
            {
                appManifestWriter.WriteLine("<Service Name='{0}' ServicePackageActivationMode='{1}'>", iter->first, iter->second.ServicePackageActivationMode, iter->second.ServiceDnsName);
            }

            WriteStatelessServiceType(appManifestWriter, iter->second);
            appManifestWriter.WriteLine("</Service>");
        }

        for (auto iter = requiredStatefulService_.begin(); iter != requiredStatefulService_.end(); iter++)
        {
            if (!iter->second.ServiceDnsName.empty())
            {
                appManifestWriter.WriteLine("<Service Name='{0}' ServicePackageActivationMode='{1}' ServiceDnsName='{2}'>", iter->first, iter->second.ServicePackageActivationMode, iter->second.ServiceDnsName);
            }
            else
            {
                appManifestWriter.WriteLine("<Service Name='{0}' ServicePackageActivationMode='{1}'>", iter->first, iter->second.ServicePackageActivationMode, iter->second.ServiceDnsName);
            }

            WriteStatefulServiceType(appManifestWriter, iter->second);
            appManifestWriter.WriteLine("</Service>");
        }
        appManifestWriter.WriteLine(L"</DefaultServices>");
    }

    if(userMap_.size() > 0 || groupMap_.size() > 0)
    {
        appManifestWriter.WriteLine(L"<Principals>");
        if(groupMap_.size() > 0)
        {
            appManifestWriter.WriteLine(L"<Groups>");
            for (auto iter = groupMap_.begin(); iter != groupMap_.end(); iter++)
            {
                appManifestWriter.WriteLine("<Group Name='{0}'>", iter->first);
                if(iter->second.DomainGroup.size() > 0 || iter->second.SystemGroups.size() > 0 || iter->second.DomainUsers.size() > 0)
                {
                    appManifestWriter.WriteLine(L"<Membership>");
                    for (auto iter1 = iter->second.DomainGroup.begin(); iter1 != iter->second.DomainGroup.end(); iter1++)
                    {
                        appManifestWriter.WriteLine("<DomainGroup Name='{0}'/>", *iter1);
                    }

                    for (auto iter1 = iter->second.SystemGroups.begin(); iter1 != iter->second.SystemGroups.end(); iter1++)
                    {
                        appManifestWriter.WriteLine("<SystemGroup Name='{0}'/>", *iter1);
                    }

                    for (auto iter1 = iter->second.DomainUsers.begin(); iter1 != iter->second.DomainUsers.end(); iter1++)
                    {
                        appManifestWriter.WriteLine("<DomainUser Name='{0}'/>", *iter1);
                    }
                    appManifestWriter.WriteLine(L"</Membership>");
                }
                appManifestWriter.WriteLine(L"</Group>");
            }
            appManifestWriter.WriteLine(L"</Groups>");
        }

        if(userMap_.size() > 0)
        {
            appManifestWriter.WriteLine(L"<Users>");
            for (auto iter = userMap_.begin(); iter != userMap_.end(); iter++)
            {
                wstring userStartElement= wformatString("<User Name='{0}'", iter->first);
                if(!iter->second.AccountType.empty())
                {
                    wstring accountType = wformatString(" AccountType='{0}'", iter->second.AccountType);
                    userStartElement.append(accountType);
                }

                if(!iter->second.AccontName.empty())
                {
                    wstring accountName = wformatString(" AccountName='{0}'", iter->second.AccontName);
                    userStartElement.append(accountName);
                }

                if(!iter->second.AccountPassword.empty())
                {
                    wstring accountPassword = wformatString(" Password='{0}' PasswordEncrypted='{1}'", iter->second.AccountPassword, iter->second.IsPasswordEncrypted);
                    userStartElement.append(accountPassword);
                }
                                
                userStartElement.append(L">");                

                appManifestWriter.WriteLine(userStartElement);

                if(iter->second.Groups.size() > 0 || iter->second.SystemGroups.size() > 0)
                {
                    appManifestWriter.WriteLine(L"<MemberOf>");
                    for (auto iter1 = iter->second.SystemGroups.begin(); iter1 != iter->second.SystemGroups.end(); iter1++)
                    {
                        appManifestWriter.WriteLine("<SystemGroup Name='{0}'/>", *iter1);
                    }

                    for (auto iter1 = iter->second.Groups.begin(); iter1 != iter->second.Groups.end(); iter1++)
                    {
                        appManifestWriter.WriteLine("<Group NameRef='{0}'/>", *iter1);
                    }
                    appManifestWriter.WriteLine(L"</MemberOf>");
                }
                appManifestWriter.WriteLine(L"</User>");
            }
            appManifestWriter.WriteLine(L"</Users>");
        }
        appManifestWriter.WriteLine(L"</Principals>");
    }

    if(!defaultRunAsPolicy_.empty() || healthPolicy_)
    {
        appManifestWriter.WriteLine(L"<Policies>");

        if (!defaultRunAsPolicy_.empty())
        {
            appManifestWriter.WriteLine("<DefaultRunAsPolicy UserRef='{0}'/>", defaultRunAsPolicy_);
        }

        WriteHealthPolicy(appManifestWriter);

        appManifestWriter.WriteLine(L"</Policies>");
    }

    appManifestWriter.WriteLine(L"</ApplicationManifest>");

    // Do not support Principals and Policies as of now
}

void ApplicationBuilder::WriteHealthPolicy(StringWriter & writer)
{
    if (!healthPolicy_) { return; }

    writer.WriteLine("<HealthPolicy ConsiderWarningAsError='{0}' MaxPercentUnhealthyDeployedApplications='{1}'>",
        healthPolicy_->ConsiderWarningAsError,
        healthPolicy_->MaxPercentUnhealthyDeployedApplications);

    writer.WriteLine("<DefaultServiceTypeHealthPolicy MaxPercentUnhealthyServices='{0}' MaxPercentUnhealthyPartitionsPerService='{1}' MaxPercentUnhealthyReplicasPerPartition='{2}' />",
        healthPolicy_->DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices,
        healthPolicy_->DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService,
        healthPolicy_->DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition);

    for (auto it=healthPolicy_->ServiceTypeHealthPolicies.begin(); it!=healthPolicy_->ServiceTypeHealthPolicies.end(); ++it)
    {
        writer.WriteLine("<ServiceTypeHealthPolicy MaxPercentUnhealthyServices='{0}' MaxPercentUnhealthyPartitionsPerService='{1}' MaxPercentUnhealthyReplicasPerPartition='{2}' ServiceTypeName='{3}' />",
            it->second.MaxPercentUnhealthyServices,
            it->second.MaxPercentUnhealthyPartitionsPerService,
            it->second.MaxPercentUnhealthyReplicasPerPartition,
            it->first);
    }

    writer.WriteLine(L"</HealthPolicy>");
}

void ApplicationBuilder::WriteStatelessServiceType(StringWriter & writer, StatelessService & statelessService)
{
    writer.Write("<StatelessService ServiceTypeName='{0}' InstanceCount='{1}'",
        statelessService.TypeName,
        statelessService.InstanceCount);

    if (!statelessService.DefaultMoveCost.empty())
    {
        writer.Write(" DefaultMoveCost='{0}'", statelessService.DefaultMoveCost);
    }

    writer.WriteLine(L">");

    switch (statelessService.Scheme)
    {
    case PartitionScheme::Singleton:
        writer.WriteLine("<SingletonPartition></SingletonPartition>");
        break;
    case PartitionScheme::UniformInt64:
        writer.WriteLine("<UniformInt64Partition PartitionCount='{0}' LowKey='{1}' HighKey='{2}'></UniformInt64Partition>",
            statelessService.PartitionCount,
            statelessService.LowKey,
            statelessService.HighKey);
        break;
    case PartitionScheme::Named:
        writer.Write("<NamedPartition>");
        for (wstring s : statelessService.PartitionNames)
        {
            writer.Write("<Partition Name=\"{0}\"/>", s);
        }
        writer.WriteLine("</NamedPartition>");
        break;
    default:
        Assert::CodingError("Unknown partition scheme");
    }

    if (!statelessService.Metrics.empty())
    {
        writer.WriteLine("<LoadMetrics>");

        for (auto metric = statelessService.Metrics.begin(); metric != statelessService.Metrics.end(); ++metric)
        {
            wstring weight;
            switch (metric->Weight)
            {
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO:
                weight = L"Zero";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW:
                weight = L"Low";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM:
                weight = L"Medium";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH:
                weight = L"High";
                break;
            default:
                Assert::CodingError("Unknown metric weight");
            }
            writer.WriteLine("<LoadMetric Name='{0}' PrimaryDefaultLoad='{1}' SecondaryDefaultLoad='{2}' Weight='{3}' />",
                metric->Name,
                metric->PrimaryDefaultLoad,
                metric->SecondaryDefaultLoad,
                weight);
        } 

        writer.WriteLine("</LoadMetrics>");
    }

    if (!statelessService.PlacementConstraints.empty())
    {
        auto xml = statelessService.PlacementConstraints;
        StringUtility::Replace<wstring>(xml, L"&", L"&amp;");
        StringUtility::Replace<wstring>(xml, L"<", L"&lt;");
        StringUtility::Replace<wstring>(xml, L">", L"&gt;");
        StringUtility::Replace<wstring>(xml, L"\"", L"&quot;");
        StringUtility::Replace<wstring>(xml, L"'", L"&apos;");

        writer.WriteLine("<PlacementConstraints>{0}</PlacementConstraints>", xml);
    }

    if (!statelessService.ServiceCorrelations.empty())
    {
        writer.WriteLine("<ServiceCorrelations>");

        for (auto correlation = statelessService.ServiceCorrelations.begin(); correlation != statelessService.ServiceCorrelations.end(); ++correlation)
        {
            wstring scheme;
            switch (correlation->Scheme)
            {
            case FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY:
                scheme = L"Affinity";
                break;
            case FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY:
                scheme = L"AlignedAffinity";
                break;
            case FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY:
                scheme = L"NonAlignedAffinity";
                break;
            default:
                Assert::CodingError("Unknown Service Correlation");
            }
            writer.WriteLine("<ServiceCorrelation ServiceName='{0}' Scheme='{1}' />",
                correlation->ServiceName,
                scheme);
        }

        writer.WriteLine("</ServiceCorrelations>");
    }

    if (!statelessService.PlacementPolicies.empty())
    {
        writer.WriteLine("<ServicePlacementPolicies>");

        for (auto placementPolicy = statelessService.PlacementPolicies.begin(); placementPolicy != statelessService.PlacementPolicies.end(); ++placementPolicy)
        {
            wstring type;
            switch (placementPolicy->Type)
            {
            case FABRIC_PLACEMENT_POLICY_INVALID:
                type = L"InvalidDomain";
                break;
            case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
                type = L"RequiredDomain";
                break;
            case FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
                type = L"PreferedPrimaryDomain";
                break;
            case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
                type = L"RequiredDomainDistribution";
                break;
            default:
                Assert::CodingError("Unknown PlacementPolicy type");
            }
            writer.WriteLine("<ServicePlacementPolicy Type='{0}' DomainName='{1}' />",
                type,
                placementPolicy->DomainName);
        }

        writer.WriteLine("</ServicePlacementPolicies>");
    }

    if (statelessService.ScalingPolicies.size() > 0)
    {
        writer.WriteLine("<ServiceScalingPolicies>");
        for (auto scalingPolicy : statelessService.ScalingPolicies)
        {
            writer.WriteLine("<ScalingPolicy>");
            if (scalingPolicy.Trigger != nullptr)
            {
                if (scalingPolicy.Trigger->Kind == Reliability::ScalingTriggerKind::AveragePartitionLoad)
                {
                    shared_ptr<Reliability::AveragePartitionLoadScalingTrigger> aplTrigger = static_pointer_cast<Reliability::AveragePartitionLoadScalingTrigger>(scalingPolicy.Trigger);
                    writer.WriteLine("<AveragePartitionLoadScalingTrigger MetricName=\"{0}\" LowerLoadThreshold=\"{1}\" UpperLoadThreshold=\"{2}\" ScaleIntervalInSeconds=\"{3}\" />",
                        aplTrigger->MetricName,
                        aplTrigger->LowerLoadThreshold,
                        aplTrigger->UpperLoadThreshold,
                        aplTrigger->ScaleIntervalInSeconds);
                }
                else if (scalingPolicy.Trigger->Kind == Reliability::ScalingTriggerKind::AverageServiceLoad)
                {
                    shared_ptr<Reliability::AverageServiceLoadScalingTrigger> aslTrigger = static_pointer_cast<Reliability::AverageServiceLoadScalingTrigger>(scalingPolicy.Trigger);
                    writer.WriteLine("<AverageServiceLoadScalingTrigger MetricName=\"{0}\" LowerLoadThreshold=\"{1}\" UpperLoadThreshold=\"{2}\" ScaleIntervalInSeconds=\"{3}\" />",
                        aslTrigger->MetricName,
                        aslTrigger->LowerLoadThreshold,
                        aslTrigger->UpperLoadThreshold,
                        aslTrigger->ScaleIntervalInSeconds);
                }
            }
            if (scalingPolicy.Mechanism != nullptr)
            {
                if (scalingPolicy.Mechanism->Kind == Reliability::ScalingMechanismKind::AddRemoveIncrementalNamedPartition)
                {
                    shared_ptr<Reliability::AddRemoveIncrementalNamedPartitionScalingMechanism> arMechanism = static_pointer_cast<Reliability::AddRemoveIncrementalNamedPartitionScalingMechanism>(scalingPolicy.Mechanism);
                    writer.WriteLine("<AddRemoveIncrementalNamedPartitionScalingMechanism MinPartitionCount=\"{0}\" MaxPartitionCount=\"{1}\" ScaleIncrement=\"{2}\" />",
                        arMechanism->MinimumPartitionCount,
                        arMechanism->MaximumPartitionCount,
                        arMechanism->ScaleIncrement);
                }
                else if (scalingPolicy.Mechanism->Kind == Reliability::ScalingMechanismKind::PartitionInstanceCount)
                {
                    shared_ptr<Reliability::InstanceCountScalingMechanism> icMechanism = static_pointer_cast<Reliability::InstanceCountScalingMechanism>(scalingPolicy.Mechanism);
                    writer.WriteLine("<InstanceCountScalingMechanism MinInstanceCount=\"{0}\" MaxInstanceCount=\"{1}\" ScaleIncrement=\"{2}\" />",
                        icMechanism->MinimumInstanceCount,
                        icMechanism->MaximumInstanceCount,
                        icMechanism->ScaleIncrement);
                }
            }
            writer.WriteLine("</ScalingPolicy>");
        }
        writer.WriteLine("</ServiceScalingPolicies>");
    }

    writer.WriteLine("</StatelessService>");
}

void ApplicationBuilder::WriteStatefulServiceType(Common::StringWriter & writer, StatefulService & statefulService)
{
    writer.Write("<StatefulService ServiceTypeName='{0}' TargetReplicaSetSize='{1}' MinReplicaSetSize='{2}'",
        statefulService.TypeName,
        statefulService.TargetReplicaSetSize,
        statefulService.MinReplicaSetSize);

    if (!statefulService.DefaultMoveCost.empty())
    {
        writer.Write(" DefaultMoveCost='{0}'", statefulService.DefaultMoveCost);
    }

    if (!statefulService.ReplicaRestartWaitDurationInSeconds.empty())
    {
        writer.Write(" ReplicaRestartWaitDurationSeconds='{0}'", statefulService.ReplicaRestartWaitDurationInSeconds);
    }

    if (!statefulService.QuorumLossWaitDurationInSeconds.empty())
    {
        writer.Write(" QuorumLossWaitDurationSeconds='{0}'", statefulService.QuorumLossWaitDurationInSeconds);
    }

    if (!statefulService.StandByReplicaKeepDurationInSeconds.empty())
    {
        writer.Write(" StandByReplicaKeepDurationSeconds='{0}'", statefulService.StandByReplicaKeepDurationInSeconds);
    }

    writer.WriteLine(L">");

    switch (statefulService.Scheme)
    {
    case PartitionScheme::Singleton:
        writer.WriteLine("<SingletonPartition></SingletonPartition>");
        break;
    case PartitionScheme::UniformInt64:
        writer.WriteLine("<UniformInt64Partition PartitionCount='{0}' LowKey='{1}' HighKey='{2}'></UniformInt64Partition>",
            statefulService.PartitionCount,
            statefulService.LowKey,
            statefulService.HighKey);
        break;
    case PartitionScheme::Named:
        writer.Write("<NamedPartition>");
        for (wstring s : statefulService.PartitionNames)
        {
            writer.Write("<Partition Name=\"{0}\"/>", s);
        }
        writer.WriteLine("</NamedPartition>");
        break;
    default:
        Assert::CodingError("Unknown partition scheme");
    }

    if (!statefulService.Metrics.empty())
    {
        writer.WriteLine("<LoadMetrics>");

        for (auto metric = statefulService.Metrics.begin(); metric != statefulService.Metrics.end(); ++metric)
        {
            wstring weight;
            switch (metric->Weight)
            {
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO:
                weight = L"Zero";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW:
                weight = L"Low";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM:
                weight = L"Medium";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH:
                weight = L"High";
                break;
            default:
                Assert::CodingError("Unknown metric weight");
            }
            writer.WriteLine("<LoadMetric Name='{0}' PrimaryDefaultLoad='{1}' SecondaryDefaultLoad='{2}' Weight='{3}' />",
                metric->Name,
                metric->PrimaryDefaultLoad,
                metric->SecondaryDefaultLoad,
                weight);
        } 

        writer.WriteLine("</LoadMetrics>");
    }

    if (!statefulService.PlacementConstraints.empty())
    {
        auto xml = statefulService.PlacementConstraints;
        StringUtility::Replace<wstring>(xml, L"&", L"&amp;");
        StringUtility::Replace<wstring>(xml, L"<", L"&lt;");
        StringUtility::Replace<wstring>(xml, L">", L"&gt;");
        StringUtility::Replace<wstring>(xml, L"\"", L"&quot;");
        StringUtility::Replace<wstring>(xml, L"'", L"&apos;");

        writer.WriteLine("<PlacementConstraints>{0}</PlacementConstraints>", xml);
    }

    if (!statefulService.ServiceCorrelations.empty())
    {
        writer.WriteLine("<ServiceCorrelations>");

        for (auto correlation = statefulService.ServiceCorrelations.begin(); correlation != statefulService.ServiceCorrelations.end(); ++correlation)
        {
            wstring scheme;
            switch (correlation->Scheme)
            {
            case FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY:
                scheme = L"Affinity";
                break;
            case FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY:
                scheme = L"AlignedAffinity";
                break;
            case FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY:
                scheme = L"NonAlignedAffinity";
                break;
            default:
                Assert::CodingError("Unknown Service Correlation");
            }
            writer.WriteLine("<ServiceCorrelation ServiceName='{0}' Scheme='{1}' />",
                correlation->ServiceName,
                scheme);
        }

        writer.WriteLine("</ServiceCorrelations>");
    }

    if (!statefulService.PlacementPolicies.empty())
    {
        writer.WriteLine("<ServicePlacementPolicies>");

        for (auto placementPolicy = statefulService.PlacementPolicies.begin(); placementPolicy != statefulService.PlacementPolicies.end(); ++placementPolicy)
        {
            wstring type;
            switch (placementPolicy->Type)
            {
            case FABRIC_PLACEMENT_POLICY_INVALID:
                type = L"InvalidDomain";
                break;
            case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
                type = L"RequiredDomain";
                break;
            case FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
                type = L"PreferedPrimaryDomain";
                break;
            case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
                type = L"RequiredDomainDistribution";
                break;
            default:
                Assert::CodingError("Unknown PlacementPolicy type");
            }
            writer.WriteLine("<ServicePlacementPolicy Type='{0}' DomainName='{1}' />",
                type,
                placementPolicy->DomainName);
        }

        writer.WriteLine("</ServicePlacementPolicies>");
    }

    if (statefulService.ScalingPolicies.size() > 0)
    {
        writer.WriteLine("<ServiceScalingPolicies>");
        for (auto scalingPolicy : statefulService.ScalingPolicies)
        {
            writer.WriteLine("<ScalingPolicy>");
            if (scalingPolicy.Trigger != nullptr)
            {
                if (scalingPolicy.Trigger->Kind == Reliability::ScalingTriggerKind::AveragePartitionLoad)
                {
                    shared_ptr<Reliability::AveragePartitionLoadScalingTrigger> aplTrigger = static_pointer_cast<Reliability::AveragePartitionLoadScalingTrigger>(scalingPolicy.Trigger);
                    writer.WriteLine("<AveragePartitionLoadScalingTrigger MetricName=\"{0}\" LowerLoadThreshold=\"{1}\" UpperLoadThreshold=\"{2}\" ScaleIntervalInSeconds=\"{3}\" />",
                        aplTrigger->MetricName,
                        aplTrigger->LowerLoadThreshold,
                        aplTrigger->UpperLoadThreshold,
                        aplTrigger->ScaleIntervalInSeconds);
                }
                else if (scalingPolicy.Trigger->Kind == Reliability::ScalingTriggerKind::AverageServiceLoad)
                {
                    shared_ptr<Reliability::AverageServiceLoadScalingTrigger> aslTrigger = static_pointer_cast<Reliability::AverageServiceLoadScalingTrigger>(scalingPolicy.Trigger);
                    writer.WriteLine("<AverageServiceLoadScalingTrigger MetricName=\"{0}\" LowerLoadThreshold=\"{1}\" UpperLoadThreshold=\"{2}\" ScaleIntervalInSeconds=\"{3}\" UseOnlyPrimaryLoad=\"{4}\" />",
                        aslTrigger->MetricName,
                        aslTrigger->LowerLoadThreshold,
                        aslTrigger->UpperLoadThreshold,
                        aslTrigger->ScaleIntervalInSeconds,
                        aslTrigger->UseOnlyPrimaryLoad);
                }
            }
            if (scalingPolicy.Mechanism != nullptr)
            {
                if (scalingPolicy.Mechanism->Kind == Reliability::ScalingMechanismKind::AddRemoveIncrementalNamedPartition)
                {
                    shared_ptr<Reliability::AddRemoveIncrementalNamedPartitionScalingMechanism> arMechanism = static_pointer_cast<Reliability::AddRemoveIncrementalNamedPartitionScalingMechanism>(scalingPolicy.Mechanism);
                    writer.WriteLine("<AddRemoveIncrementalNamedPartitionScalingMechanism MinPartitionCount=\"{0}\" MaxPartitionCount=\"{1}\" ScaleIncrement=\"{2}\" />",
                        arMechanism->MinimumPartitionCount,
                        arMechanism->MaximumPartitionCount,
                        arMechanism->ScaleIncrement);
                }
                else if (scalingPolicy.Mechanism->Kind == Reliability::ScalingMechanismKind::PartitionInstanceCount)
                {
                    shared_ptr<Reliability::InstanceCountScalingMechanism> icMechanism = static_pointer_cast<Reliability::InstanceCountScalingMechanism>(scalingPolicy.Mechanism);
                    writer.WriteLine("<InstanceCountScalingMechanism MinInstanceCount=\"{0}\" MaxInstanceCount=\"{1}\" ScaleIncrement=\"{2}\" />",
                        icMechanism->MinimumInstanceCount,
                        icMechanism->MaximumInstanceCount,
                        icMechanism->ScaleIncrement);
                }
            }
            writer.WriteLine("</ScalingPolicy>");
        }
        writer.WriteLine("</ServiceScalingPolicies>");
    }

    writer.WriteLine("</StatefulService>");
}

DWORD ApplicationBuilder::GetComTimeout(TimeSpan cTimeout)
{
    DWORD comTimeout;
    HRESULT hr = Int64ToDWord(cTimeout.TotalPositiveMilliseconds(), &comTimeout);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "Could not convert timeout from int64 to DWORD. hr = {0}", hr);

    return comTimeout;
}

