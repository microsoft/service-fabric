// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <psapi.h>

using namespace std;
using namespace FabricTest;
using namespace Reliability;
using namespace Federation;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;

GlobalWString RandomTestApplicationHelper::AppNamePrefix = make_global<wstring>(L"fabric:/testapp");
GlobalWString RandomTestApplicationHelper::AppTypeName = make_global<wstring>(L"FabTRandomApp");

wstring RandomTestApplicationHelper::GetPackageRgPolicies(map<wstring, int>& policies, wstring separator)
{
    wstring rgPolicies = L"";
    size_t i = 0;
    for (auto itRg = policies.begin(); itRg != policies.end(); itRg++)
    {
        rgPolicies = wformatString(L"{0}{1}{2}{3}{4}",
            rgPolicies,
            itRg->first,
            separator,
            itRg->second,
            (i < (policies.size() - 1) ? separator : L""));
        i++;
    }
    return rgPolicies;
}

 int RandomTestApplicationHelper::ServicePackage::GetServicePackageMemoryPolicy()
 {
    return RgPolicies.find(L"MemoryInMB") != RgPolicies.end() ? (RgPolicies.find(L"MemoryInMB"))->second : 0;
 }

void RandomTestApplicationHelper::UploadApplicationTypes()
{
    if(config_->ApplicationCount > 0)
    {
        // Enable CM
        CreateInitialApplicationType();
        if(config_->UpgradeInterval > 0)
        {
            CreateUpgradeApplicationTypes();
        }

        for (auto iter = applicationTypeVersions_.begin(); iter != applicationTypeVersions_.end(); iter++)
        {
            ApplicationType & type = *iter;
            FABRICSESSION.AddCommand(L"#");
            FABRICSESSION.AddCommand(wformatString("#{0}", type.Operation));
            FABRICSESSION.AddCommand(L"#");
            FABRICSESSION.AddCommand(wformatString("app.add {0} {1} {0}", type.Version, type.TypeName));
            FABRICSESSION.AddCommand(wformatString("app.clear {0}", type.Version));
            for (auto spIter = type.ServicePackages.begin(); spIter != type.ServicePackages.end(); spIter++)
            {
                ServicePackage & sp = *spIter;
                FABRICSESSION.AddCommand(wformatString("# {0}", sp.Name));
                FABRICSESSION.AddCommand(wformatString("app.servicepack {0} {1} version={2} configversion={2} skipupload={3} {4}",
                    type.Version,
                    sp.Name,
                    sp.Version,
                    sp.SkipUpload,
                    sp.RgPolicies.size() > 0 ? wformatString(L"resources={0}", GetPackageRgPolicies(sp.RgPolicies)) : L""));

                for (auto stIter = sp.Types.begin(); stIter != sp.Types.end(); stIter++)
                {
                    ServiceType & st = *stIter;
                    FABRICSESSION.AddCommand(wformatString("app.servicetypes {0} {1} {2} {3} {4}", 
                        type.Version, 
                        sp.Name, 
                        st.TypeName, 
                        st.IsStateful ? L"stateful" : L"stateless",
                        st.IsPersisted ? L"persist": L""));
                }

                for (auto cpIter = sp.CodePackages.begin(); cpIter != sp.CodePackages.end(); cpIter++)
                {
                    CodePackage & cp = *cpIter;
                    wstring supportedTypes;
                    for (auto stIter = cp.Types.begin();;)
                    {
                        if(stIter != cp.Types.end())
                        {
                            wstring & st = *stIter;
                            supportedTypes.append(st);
                            stIter++;
                        }

                        if(stIter != cp.Types.end())
                        {
                            supportedTypes.append(L",");
                        }
                        else
                        {
                            break;
                        }
                    }

                    FABRICSESSION.AddCommand(wformatString("app.codepack {0} {1} {2} version={3} {4}{5} skipupload={6} {7}",
                        type.Version, 
                        sp.Name, 
                        cp.Name, 
                        cp.Version,
                        supportedTypes.empty() ? L"" :L"types=",
                        supportedTypes, 
                        cp.SkipUpload | sp.SkipUpload,
                        cp.RgPolicies.size() > 0 ? wformatString(L"rgpolicies={0}", GetPackageRgPolicies(cp.RgPolicies,L";")) : L""));
                }

                for (auto reqIter = sp.DefaultServices.begin(); reqIter != sp.DefaultServices.end(); reqIter++)
                {
                    DefaultService & rs = *reqIter;

                    auto cmd = wformatString("app.reqservices {0} {1} {2} {3} partition={4} {5}={6}",
                        type.Version,
                        rs.ServiceName,
                        rs.ServiceType,
                        rs.IsStateful ? L"stateful" : L"stateless",
                        rs.PartitionCount,
                        rs.IsStateful ? L"replica" : L"instance",
                        rs.ReplicaCount);

                    if (rs.ActivationMode == ServicePackageActivationMode::ExclusiveProcess)
                    {
                        cmd = wformatString("{0} servicePackageActivationMode=ExclusiveProcess", cmd);
                    }

                    FABRICSESSION.AddCommand(cmd);
                }
            }

            FABRICSESSION.AddCommand(wformatString("app.upload {0}", type.Version));
        }
    }
    else
    {
        TestSession::FailTestIf(config_->UpgradeInterval > 0, "Upgrade enabled even though application count is 0");
    }
}

void RandomTestApplicationHelper::ProvisionApplicationTypes()
{
    for(auto iter = applicationTypeVersions_.begin(); iter != applicationTypeVersions_.end(); iter++)
    {
        FABRICSESSION.AddCommand(FabricTestCommands::ProvisionApplicationTypeCommand + FabricTestDispatcher::ParamDelimiter + (*iter).Version);
    }
}

void RandomTestApplicationHelper::CreateInitialApplicationType()
{
    // Start with 2 service packages always unless max is 1
    int servicePackages = config_->AppMaxServicePackage >= 2 ? 2 : 1;
    ApplicationType appType;
    appType.TypeName = *AppTypeName;
    appType.Version = L"1";
    for (int i = 0; i < servicePackages; i++)
    {
        wstring output;
        AddServicePackage(appType, output);
    }

        applicationTypeVersions_.push_back(appType);
}

void RandomTestApplicationHelper::CreateUpgradeApplicationTypes()
{
    // Start from 1 ince a version has alredy been created
    for (int i = 1; i < config_->ApplicationVersionsCount; i++)
    {
        ApplicationType app = applicationTypeVersions_[i - 1];
        app.Operation.clear();
        app.TypeName = L"FabTRandomApp";
        app.Version = wformatString("{0}", (i + 1));
        bool done = false;
        int upgradeChanges = 0;

        // Also need to figure out a better way to specify enabled/disabled upgrade operations in 
        // config rather than having to change this variable. 
        int upgradeOperationTypes = 8;

        // Mark some service packages to skip upload.
        // For changed or desired service/code packages, mark them to be uploaded when changed.
        for (auto it = app.ServicePackages.begin(); it != app.ServicePackages.end(); ++it)
        {
            it->SkipUpload = (random_.NextDouble() < 0.8);
        }

        while(!done)
        {
            double value = random_.NextDouble();
            int operation = -1;
            if(value < 0.30)
            {
                // 30 % chance of UpgradeCodePackage
                operation = 0;
            }
            else if (value < 0.50)
            {
                // 20 % chance of UpgradeConfigPackage
                operation = 1;
            }
            else
            {
                //50% chance of any other operation
                operation = random_.NextDouble() < 0.30 ? 0 : random_.Next(2, upgradeOperationTypes);
            }
            wstring output;
            switch(operation)
            {
            case 0:
                if(UpgradeCodePackage(app, output))
                {
                    upgradeChanges++;
                    app.Operation.append(L" UpgradeCodePackage ");
                    app.Operation.append(output);
                }
                break;
            case 1:
                if(UpgradeConfigPackage(app, output))
                {
                    upgradeChanges++;
                    app.Operation.append(L" UpgradeConfigPackage ");
                    app.Operation.append(output);
                    // Set done = true so that no other chnges happen and the upgrade is a true notification only
                    done = true;
                }
                break;
            case 2:
                if(AddCodePackage(app, output))
                {
                    upgradeChanges++;
                    app.Operation.append(L" AddCodePackage ");
                    app.Operation.append(output);
                }
                break;
            case 3:
                if(RemoveCodePackage(app, output))
                {
                    upgradeChanges++;
                    app.Operation.append(L" RemoveCodePackage ");
                    app.Operation.append(output);
                }
                break;
            case 4:
                if(AddServicePackage(app, output))
                {
                    upgradeChanges++;
                    app.Operation.append(L" AddServicePackage ");
                    app.Operation.append(output);
                }
                break;
            case 5:
                if(AddServiceType(app, output))
                {
                    upgradeChanges++;
                    app.Operation.append(L" AddServiceType ");
                    app.Operation.append(output);
                }
                break;
            case 6:
                if(RemoveServicePackage(app, output))
                {
                    upgradeChanges++;
                    app.Operation.append(L" RemoveServicePackage ");
                    app.Operation.append(output);
                }
                break;
            case 7:
                if(RemoveServiceType(app, output))
                {
                    upgradeChanges++;
                    app.Operation.append(L" RemoveServiceType ");
                    app.Operation.append(output);
                }
                break;
            default:
                TestSession::FailTest("Invalid Operation {0}", operation);
            }

            if(upgradeChanges == 2)
            {
                done = true;
            }
        }

        applicationTypeVersions_.push_back(app);
    }
}

void RandomTestApplicationHelper::GetAllServiceNames(size_t applicationVersionIndex, vector<wstring> & serviceNames)
{
    TestSession::FailTestIfNot(applicationVersionIndex < applicationTypeVersions_.size(), "Incorrect applicationVersionIndex {0}", applicationVersionIndex);
    ApplicationType & appType = applicationTypeVersions_[applicationVersionIndex];
    for (auto spIter = appType.ServicePackages.begin(); spIter != appType.ServicePackages.end(); spIter++)
    {
        ServicePackage & servicePack = *spIter;
        for (auto reqSIter = servicePack.DefaultServices.begin(); reqSIter != servicePack.DefaultServices.end(); reqSIter++)
        {
            DefaultService & reqService = *reqSIter;
            serviceNames.push_back(reqService.ServiceName);
        }
    }
}

void RandomTestApplicationHelper::GetAllStatefulServiceNames(size_t applicationVersionIndex, vector<wstring> & serviceNames)
{
    TestSession::FailTestIfNot(applicationVersionIndex < applicationTypeVersions_.size(), "Incorrect applicationVersionIndex {0}", applicationVersionIndex);
    ApplicationType & appType = applicationTypeVersions_[applicationVersionIndex];
    for (auto spIter = appType.ServicePackages.begin(); spIter != appType.ServicePackages.end(); spIter++)
    {
        ServicePackage & servicePack = *spIter;
        for (auto reqSIter = servicePack.DefaultServices.begin(); reqSIter != servicePack.DefaultServices.end(); reqSIter++)
        {
            DefaultService & reqService = *reqSIter;
            if(reqService.IsStateful)
            {
                serviceNames.push_back(reqService.ServiceName);
            }
        }
    }
}

bool RandomTestApplicationHelper::IsStatefulService(size_t applicationVersionIndex, wstring const& serviceName)
{
    TestSession::FailTestIfNot(applicationVersionIndex < applicationTypeVersions_.size(), "Incorrect applicationVersionIndex {0}", applicationVersionIndex);
    ApplicationType & appType = applicationTypeVersions_[applicationVersionIndex];
    for (auto spIter = appType.ServicePackages.begin(); spIter != appType.ServicePackages.end(); spIter++)
    {
        ServicePackage & servicePack = *spIter;
        for (auto reqSIter = servicePack.DefaultServices.begin(); reqSIter != servicePack.DefaultServices.end(); reqSIter++)
        {
            DefaultService & reqService = *reqSIter;
            if(serviceName == reqService.ServiceName)
            {
                return reqService.IsStateful;
            }
        }
    }

    TestSession::FailTest("Service {0} not found", serviceName);
}

wstring RandomTestApplicationHelper::GetVersionString(size_t applicationVersionIndex)
{
    TestSession::FailTestIfNot(applicationVersionIndex < applicationTypeVersions_.size(), "Incorrect applicationVersionIndex {0}", applicationVersionIndex);
    return applicationTypeVersions_[applicationVersionIndex].Version;
}

void RandomTestApplicationHelper::GenerateServicePackageRgPolicies(ServicePackage& servicePackage, bool forUpgrade)
{
    // Generate service package CPU resource requirement
    // with configured probability, if there are nodes with CPU resource
    if (random_.NextDouble() < config_->ServicePackageCpuLimitRatio && 
        packageRGCapacities_.find(L"CPU") != packageRGCapacities_.end())
    {
        int cpuResource = random_.Next(config_->MinPackageCpuLimit, packageRGCapacities_.find(L"CPU")->second + 1);
        servicePackage.RgPolicies.insert(make_pair(L"CPU", cpuResource));
    }

    // Generate service package Memory resource requirement
    // with configured probability, if there are nodes with Memory resource
    if (random_.NextDouble() < config_->ServicePackageMemoryLimitRatio &&
        packageRGCapacities_.find(L"Memory") != packageRGCapacities_.end())
    {
        int memoryResource = random_.Next(config_->MinPackageMemoryLimit, packageRGCapacities_.find(L"Memory")->second + 1);
        servicePackage.RgPolicies.insert(make_pair(L"MemoryInMB", memoryResource));
    }

    // Memory resource specification per code package (all or none)
    // Valid only if there are nodes with memory resource,
    // and it is not generation of upgrade application commands
    if (!forUpgrade && packageRGCapacities_.find(L"Memory") != packageRGCapacities_.end())
    {
        servicePackage.HasMemoryPolicyPerCp = random_.NextDouble() < config_->CodePackageMemoryLimitRatio ? true : false;
    }
    else
    {
        servicePackage.HasMemoryPolicyPerCp = false;
    }
}

bool RandomTestApplicationHelper::AddServicePackage(ApplicationType & appType, wstring & output)
{
    if(appType.ServicePackages.size() == static_cast<size_t>(config_->AppMaxServicePackage))
    {
        return false;
    }

    ServicePackage servicePack;
    servicePack.Name = wformatString("SP.{0}{1}", DateTime::Now().Ticks, random_.Next());
    servicePack.Version = L"1";
    servicePack.SkipUpload = false;

    // Randomly generate required service package resources
    GenerateServicePackageRgPolicies(servicePack, false);

    int codePackages = random_.Next(1, config_->AppMaxCodePackage + 1);
    for (int i = 0; i < codePackages; i++)
    {
        wstring temp;
        TestSession::FailTestIfNot(AddCodePackage(servicePack, temp), "AddCodePackage returned false");
    }

    int serviceTypes = random_.Next(1, config_->AppMaxServiceTypes + 1);
    serviceTypes = serviceTypes < codePackages ? (codePackages < config_->AppMaxServiceTypes ? codePackages : config_->AppMaxServiceTypes) : serviceTypes;
    for (int i = 0; i < serviceTypes; i++)
    {
        wstring temp;
        TestSession::FailTestIfNot(AddServiceType(servicePack, temp), "AddServiceType returned false");
    }

    appType.ServicePackages.push_back(servicePack);
    output = servicePack.Name;
    return true;
}

void RandomTestApplicationHelper::GenerateCodePackageRgPolicies(CodePackage& codePackage, bool hasMemoryPolicy, int servicePackMemoryPolicy)
{
    // Generate service package CPU resource requirement
    // with 50% probability, if there are nodes with CPU metric
    if (random_.NextDouble() < config_->CodePackageCpuLimitRatio &&
        packageRGCapacities_.find(L"CPU") != packageRGCapacities_.end())
    {
        int cpuResource = random_.Next(config_->NodeCpuCapacityMin, config_->NodeCpuCapacityMax + 1);
        codePackage.RgPolicies.insert(make_pair(L"CpuShares", cpuResource));
    }

    // If code package requires memory resource (all or none)
    if (hasMemoryPolicy)
    {
        // If there is no SP memory policy it will be set to 0,
        // hence max CP capacity is equal to the one per replica (worst case: all exclusive)
        int maxCpMemory = servicePackMemoryPolicy == 0 ? packageRGCapacities_.find(L"Memory")->second :
            static_cast<int>(double(servicePackMemoryPolicy) / config_->AppMaxCodePackage) ;
        int cpMemoryPolicy = random_.Next(config_->MinPackageMemoryLimit, maxCpMemory + 1);
        codePackage.RgPolicies.insert(make_pair(L"MemoryInMB", cpMemoryPolicy));
    }
}

bool RandomTestApplicationHelper::AddCodePackage(ApplicationType & appType, wstring & output)
{
    int spIndex = random_.Next(0, static_cast<int>(appType.ServicePackages.size()));
    bool result = AddCodePackage(appType.ServicePackages[spIndex], output);
    if(result)
    {
        int version = Int32_Parse(appType.ServicePackages[spIndex].Version);
        appType.ServicePackages[spIndex].Version = wformatString("{0}", ++version);
        appType.ServicePackages[spIndex].SkipUpload = false;
    }

    return result;
}

bool RandomTestApplicationHelper::AddCodePackage(ServicePackage & servicePack, wstring & output)
{
    if(servicePack.CodePackages.size() == static_cast<size_t>(config_->AppMaxCodePackage))
    {
        return false;
    }

    CodePackage codePackage;
    codePackage.Name = wformatString("CP.{0}{1}", DateTime::Now().Ticks, random_.Next());
    codePackage.Version = L"1";
    codePackage.SkipUpload = false;

    // Randomly generate code package RG policies
    GenerateCodePackageRgPolicies(codePackage, servicePack.HasMemoryPolicyPerCp, servicePack.GetServicePackageMemoryPolicy());

    for (auto cpIter = servicePack.CodePackages.begin(); cpIter != servicePack.CodePackages.end(); cpIter++)
    {
        CodePackage & cp = *cpIter;
        if(cp.Types.size() > 1)
        {
            codePackage.Types.push_back(cp.Types[0]);
            cp.Types.erase(cp.Types.begin());
            int cpVersion = Int32_Parse(cp.Version);
            cp.Version = wformatString("{0}", ++cpVersion);
            cp.SkipUpload = false;
            break;
        }
    }

    servicePack.CodePackages.push_back(codePackage);
    servicePack.SkipUpload = false;
    output = servicePack.Name + L"," + codePackage.Name; 
    return true;
}

bool RandomTestApplicationHelper::AddServiceType(ApplicationType & appType, wstring & output)
{
    int spIndex = random_.Next(0, static_cast<int>(appType.ServicePackages.size()));
    bool result = AddServiceType(appType.ServicePackages[spIndex], output);
    if(result)
    {
        int version = Int32_Parse(appType.ServicePackages[spIndex].Version);
        appType.ServicePackages[spIndex].Version = wformatString("{0}", ++version);
        appType.ServicePackages[spIndex].SkipUpload = false;
    }

    return result;
}

bool RandomTestApplicationHelper::AddServiceType(ServicePackage & servicePack, wstring & output)
{
    if(servicePack.Types.size() == static_cast<size_t>(config_->AppMaxServiceTypes))
    {
        return false;
    }

    ServiceType serviceType;
    serviceType.TypeName = wformatString("ST.{0}{1}", DateTime::Now().Ticks, random_.Next());
    serviceType.IsStateful = random_.NextDouble() > config_->AppStatelessServiceRatio ? true : false;
    serviceType.IsPersisted = serviceType.IsStateful && random_.NextDouble() < config_->AppPersistedServiceRatio ? true : false;

    int cpIndex = random_.Next(0, static_cast<int>(servicePack.CodePackages.size()));
    servicePack.CodePackages[cpIndex].Types.push_back(serviceType.TypeName);
    int cpVersion = Int32_Parse(servicePack.CodePackages[cpIndex].Version);
    servicePack.CodePackages[cpIndex].Version = wformatString("{0}", ++cpVersion);
    servicePack.CodePackages[cpIndex].SkipUpload = false;

    servicePack.Types.push_back(serviceType);

    DefaultService service;
    service.ServiceName = wformatString("Service{0}{1}", DateTime::Now().Ticks, random_.Next());
    service.ServiceType = serviceType.TypeName;
    service.IsStateful = serviceType.IsStateful;
    service.PartitionCount = random_.Next(1, config_->AppMaxPartitionCount + 1);
    service.ReplicaCount = random_.Next(1, config_->AppMaxReplicaCount + 1);
    service.ActivationMode = this->GetRandomServicePackageActivationMode();

    servicePack.DefaultServices.push_back(service);
    output = servicePack.Name + L"," + serviceType.TypeName; 
    return true;
}

bool RandomTestApplicationHelper::RemoveServicePackage(ApplicationType & appType, wstring & output)
{
    if(appType.ServicePackages.size() == 1)
    {
        return false;
    }

    int spIndex = random_.Next(0, static_cast<int>(appType.ServicePackages.size()));
    auto spIter = appType.ServicePackages.begin();
    advance(spIter, spIndex);
    output = (*spIter).Name;
    appType.ServicePackages.erase(spIter);
    return true;
}

bool RandomTestApplicationHelper::RemoveCodePackage(ApplicationType & appType, wstring & output)
{
    int spIndex = random_.Next(0, static_cast<int>(appType.ServicePackages.size()));
    auto spIter = appType.ServicePackages.begin();
    advance(spIter, spIndex);
    ServicePackage & servicePack = *spIter;

    if(servicePack.CodePackages.size() == 1)
    {
        return false;
    }

    int cpIndex = random_.Next(0, static_cast<int>(servicePack.CodePackages.size()));
    auto cpIter = servicePack.CodePackages.begin();
    advance(cpIter, cpIndex);
    CodePackage & codePackage = *cpIter;
    vector<wstring> serviceTypes = codePackage.Types;
    servicePack.CodePackages.erase(cpIter);
    int i = 0;
    for (auto typeIter = serviceTypes.begin(); typeIter != serviceTypes.end(); typeIter++)
    {
        cpIndex = (i % static_cast<int>(servicePack.CodePackages.size()));
        servicePack.CodePackages[cpIndex].Types.push_back(*typeIter);
        int cpVersion = Int32_Parse(servicePack.CodePackages[cpIndex].Version);
        servicePack.CodePackages[cpIndex].Version = wformatString("{0}", ++cpVersion);
        servicePack.CodePackages[cpIndex].SkipUpload = false;
        i++;
    }

    int version = Int32_Parse(appType.ServicePackages[spIndex].Version);
    servicePack.Version = wformatString("{0}", ++version);
    servicePack.SkipUpload = false;

    // Force service restart.
    output = servicePack.Name + L"," + codePackage.Name; 
    return true;
}

bool RandomTestApplicationHelper::RemoveServiceType(ApplicationType & appType, wstring & output)
{
    int spIndex = random_.Next(0, static_cast<int>(appType.ServicePackages.size()));
    auto spIter = appType.ServicePackages.begin();
    advance(spIter, spIndex);
    ServicePackage & servicePack = *spIter;

    if(servicePack.Types.size() == 1)
    {
        return false;
    }

    int typeIndex = random_.Next(0, static_cast<int>(servicePack.Types.size()));
    auto typeIter = servicePack.Types.begin();
    advance(typeIter, typeIndex);
    ServiceType & type = *typeIter;
    wstring typeName = type.TypeName;
    servicePack.Types.erase(typeIter);
    for (auto cpIter = servicePack.CodePackages.begin(); cpIter != servicePack.CodePackages.end(); cpIter++)
    {
        CodePackage & codePackage = *cpIter;
        auto iter = find(codePackage.Types.begin(), codePackage.Types.end(), typeName);
        if(iter != codePackage.Types.end())
        {
            codePackage.Types.erase(iter);
            int cpVersion = Int32_Parse(codePackage.Version);
            codePackage.Version = wformatString("{0}", ++cpVersion);
            codePackage.SkipUpload = false;
            break;
        }
    }

    for (auto reqIter = servicePack.DefaultServices.begin(); reqIter != servicePack.DefaultServices.end(); reqIter++)
    {
        DefaultService & reqService = *reqIter;
        if(reqService.ServiceType == typeName)
        {
            servicePack.DefaultServices.erase(reqIter);
            break;
        }
    }

    int version = Int32_Parse(appType.ServicePackages[spIndex].Version);
    servicePack.Version = wformatString("{0}", ++version);
    servicePack.SkipUpload = false;

    output = servicePack.Name + L"," + typeName; 
    // Force service restart.

    return true;
}

bool RandomTestApplicationHelper::UpgradeConfigPackage(ApplicationType & appType, wstring & output)
{
    int spIndex = random_.Next(0, static_cast<int>(appType.ServicePackages.size()));
    auto spIter = appType.ServicePackages.begin();
    advance(spIter, spIndex);
    ServicePackage & servicePack = *spIter;
    
    int spVersion = Int32_Parse(servicePack.Version);
    servicePack.Version = wformatString("{0}", ++spVersion);
    servicePack.SkipUpload = false;

    // Randomly generate required service package resources
    GenerateServicePackageRgPolicies(servicePack, true);

    output = servicePack.Name;
    return true;
}

bool RandomTestApplicationHelper::UpgradeCodePackage(ApplicationType & appType, wstring & output)
{
    int spIndex = random_.Next(0, static_cast<int>(appType.ServicePackages.size()));
    auto spIter = appType.ServicePackages.begin();
    advance(spIter, spIndex);
    ServicePackage & servicePack = *spIter;
    
    int cpIndex = random_.Next(0, static_cast<int>(servicePack.CodePackages.size()));
    auto cpIter = servicePack.CodePackages.begin();
    advance(cpIter, cpIndex);
    CodePackage & codePackage = *cpIter;
    
    int cpVersion = Int32_Parse(codePackage.Version);
    codePackage.Version = wformatString("{0}", ++cpVersion);
    codePackage.SkipUpload = false;

    // Randomly generate code package RG policies
    GenerateCodePackageRgPolicies(codePackage, servicePack.HasMemoryPolicyPerCp, servicePack.GetServicePackageMemoryPolicy());

    int spVersion = Int32_Parse(servicePack.Version);
    servicePack.Version = wformatString("{0}", ++spVersion);
    servicePack.SkipUpload = false;

    output = servicePack.Name + L"," + codePackage.Name;
    return true;
}

ServicePackageActivationMode::Enum RandomTestApplicationHelper::GetRandomServicePackageActivationMode()
{
    if (random_.Next() % 2 == 0)
    {
        return ServicePackageActivationMode::SharedProcess;
    }

    return ServicePackageActivationMode::ExclusiveProcess;
}
