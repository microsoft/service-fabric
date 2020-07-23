// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceSource("DigestedApplicationDescription");

DigestedApplicationDescription::DigestedApplicationDescription(
    ServiceModel::ApplicationInstanceDescription const & application,
    std::vector<StoreDataServicePackage> const & servicePackages,
    CodePackageDescriptionMap const & codePackages,
    std::vector<StoreDataServiceTemplate> const & serviceTemplates,
    std::vector<Naming::PartitionedServiceDescriptor> const & defaultServices,
    ServiceModel::ServicePackageResourceGovernanceMap const & rgDescription,
    std::vector<wstring> const & networks)
    : Application(application)
    , ServicePackages(servicePackages)
    , CodePackages(codePackages)
    , ServiceTemplates(serviceTemplates)
    , DefaultServices(defaultServices)
    , ResourceGovernanceDescriptions(rgDescription)
    , CodePackageContainersImages()
    , Networks(networks)
{
    // Fix RG settings in case that user did not provide anything on SP level (but something on CP level exists).
    for (auto cpPair : CodePackages)
    {
        ServicePackageIdentifier spIdentifier(Application.ApplicationId, cpPair.first.Value);
        auto spRGIterator = ResourceGovernanceDescriptions.find(spIdentifier);
        if (spRGIterator != ResourceGovernanceDescriptions.end())
        {
            spRGIterator->second.SetMemoryInMB(cpPair.second);
        }
        else
        {
            ServicePackageResourceGovernanceDescription newRGSetting;
            newRGSetting.SetMemoryInMB(cpPair.second);
            if (newRGSetting.IsGoverned)
            {
                ResourceGovernanceDescriptions.insert(make_pair(spIdentifier, move(newRGSetting)));
            }
        }

        // Collect all container images
        std::vector<wstring> containerImages;
        for (auto const& cpDesc : cpPair.second)
        {
            if (cpDesc.CodePackage.EntryPoint.EntryPointType == EntryPointType::Enum::ContainerHost)
            {
                auto imageName = cpDesc.CodePackage.EntryPoint.ContainerEntryPoint.ImageName;
                if (!imageName.empty() && (std::find(containerImages.begin(), containerImages.end(), imageName) == containerImages.end()))
                {
                    containerImages.push_back(move(imageName));
                }
            }
        }

        CodePackageContainersImagesDescription newCPContainersImages;
        newCPContainersImages.SetContainersImages(move(containerImages));
        CodePackageContainersImages.insert(make_pair(spIdentifier, move(newCPContainersImages)));
    }
}

bool DigestedApplicationDescription::TryValidate(__out wstring & errorDetails)
{
    ErrorCode error;
    
    for (auto iter = this->DefaultServices.begin(); iter != this->DefaultServices.end(); ++iter)
    {
        error = iter->Validate();

        if (!error.IsSuccess())
        {

            Trace.WriteWarning(
                TraceSource, 
                "Validation failed for default service PSD: [{0}] : Details ='{1}'", 
                *iter,
                error.Message);

            errorDetails = error.TakeMessage();

            return false;
        }
    }

    for (auto iter = this->ServiceTemplates.begin(); iter != this->ServiceTemplates.end(); ++iter)
    {
        error = iter->PartitionedServiceDescriptor.ValidateSkipName();

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceSource, 
                "Validation failed for service template PSD: [{0}] : Details ='{1}'", 
                iter->PartitionedServiceDescriptor,
                error.Message);

            errorDetails = error.TakeMessage();

            return false;
        }
    }

    return true;
}

std::vector<StoreDataServicePackage> DigestedApplicationDescription::ComputeRemovedServiceTypes(
    DigestedApplicationDescription const & targetDescription)
{
    vector<StoreDataServicePackage> removedServiceTypes;

    for (auto iter = this->ServicePackages.begin(); iter != this->ServicePackages.end(); ++iter)
    {
        bool found = false;
        for (auto iter2 = targetDescription.ServicePackages.begin(); iter2 != targetDescription.ServicePackages.end(); ++iter2)
        {
            if (iter->TypeName == iter2->TypeName)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Trace.WriteNoise(
                TraceSource,
                "Adding Removed Service Type: {0}",
                *iter);
            removedServiceTypes.push_back(*iter);
        }
    }

    return removedServiceTypes;
}

vector<StoreDataServicePackage> DigestedApplicationDescription::ComputeAddedServiceTypes(
    DigestedApplicationDescription const & targetDescription)
{
    vector<StoreDataServicePackage> addedServiceTypes;

    for (auto iter = targetDescription.ServicePackages.begin(); iter != targetDescription.ServicePackages.end(); ++iter)
    {
        bool found = false;
        for (auto iter2 = this->ServicePackages.begin(); iter2 != this->ServicePackages.end(); ++iter2)
        {
            if (iter->TypeName == iter2->TypeName)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Trace.WriteNoise(
                TraceSource,
                "Adding New Service Type: {0}",
                *iter);
            addedServiceTypes.push_back(*iter);
        }
    }

    return addedServiceTypes;
}

ErrorCode DigestedApplicationDescription::ComputeAffectedServiceTypes(
    DigestedApplicationDescription const & targetDescription,
    __out vector<StoreDataServicePackage> & affectedTypesResult,
    __out vector<StoreDataServicePackage> & movedTypesResult,
    __out CodePackageNameMap & codePackagesResult,
    __out bool & shouldRestartPackagesResult)
{
    vector<StoreDataServicePackage> affectedTypes;
    vector<StoreDataServicePackage> movedTypes;
    CodePackageNameMap codePackages;

    bool shouldRestartPackages = false;

    // A service type needs to be upgraded (i.e. included in the request message to FM) if one of the following is true:
    //
    // 1) the type is present in both the current and target application
    // 2) the application package has been upgraded
    // 3) the type is being supported by a different package now (this will be disallowed in Silk)
    // 4) the current package supporting the type has been changed
    // 5) the resource governance settings for the service package are changed
    //
    // The service packages must be restarted (i.e. the upgrade cannot be Rolling_NotificationOnly) if one of the following is true:
    //
    // 1) the application package major version has changed
    // 2) the major version on the service package has changed
    // 3) at least one code package has changed
    // 4) resource governance settings for the service package are changed
    //
    for (auto iter = targetDescription.ServicePackages.begin(); iter != targetDescription.ServicePackages.end(); ++iter)
    {
        bool isAffected = false;
        bool isMoved = false;

        // We still need to go through each package to look for code package changes even if the application package
        // major version has not changed
        //
        for (auto iter2 = this->ServicePackages.begin(); iter2 != this->ServicePackages.end(); ++iter2)
        {
            if (iter->TypeName != iter2->TypeName)
            {
                continue;
            }

            // Check if RG has changed for this SP.
            ServicePackageIdentifier spIdentifier(iter->ApplicationId.Value, iter->PackageName.Value);
            auto const& originalRG = this->ResourceGovernanceDescriptions.find(spIdentifier);
            auto const& targetRG = targetDescription.ResourceGovernanceDescriptions.find(spIdentifier);

            bool cpuCoresUsed = false;
            if (targetRG != targetDescription.ResourceGovernanceDescriptions.end() && targetRG->second.CpuCores > 0)
            {
                cpuCoresUsed = true;
            }

            if (originalRG != this->ResourceGovernanceDescriptions.end()
                && targetRG == targetDescription.ResourceGovernanceDescriptions.end())
            {
                // Resource Governance policy was removed.
                isAffected = true;
                shouldRestartPackages = true;
                this->AddAllCodePackages(iter->PackageName, codePackages);
            }
            else if (originalRG == this->ResourceGovernanceDescriptions.end()
                && targetRG != targetDescription.ResourceGovernanceDescriptions.end())
            {
                // Resource Governance policy was added.
                isAffected = true;
                shouldRestartPackages = true;
                this->AddAllCodePackages(iter->PackageName, codePackages);
            }
            else if (originalRG != this->ResourceGovernanceDescriptions.end()
                && targetRG != targetDescription.ResourceGovernanceDescriptions.end()
                && *originalRG != *targetRG)
            {
                // Resource Governance policy has changed.
                isAffected = true;
                shouldRestartPackages = true;
                this->AddAllCodePackages(iter->PackageName, codePackages);
            }

            auto current = find_if(this->CodePackages.begin(), this->CodePackages.end(),
                [iter2](pair<ServiceModelPackageName, vector<DigestedCodePackageDescription>> const & item) -> bool
            {
                return (item.first == iter2->PackageName);
            });

            auto target = find_if(targetDescription.CodePackages.begin(), targetDescription.CodePackages.end(),
                [iter](pair<ServiceModelPackageName, vector<DigestedCodePackageDescription>> const & item) -> bool
            {
                return (item.first == iter->PackageName);
            });

            if (iter->PackageName == iter2->PackageName &&
                originalRG != this->ResourceGovernanceDescriptions.end() &&
                targetRG != targetDescription.ResourceGovernanceDescriptions.end())
            {
                if (originalRG->second.HasRgChange(targetRG->second, current->second, target->second))
                {
                    // Resource Governance policy for some CP has changed
                    isAffected = true;
                    shouldRestartPackages = true;
                    this->AddAllCodePackages(iter->PackageName, codePackages);
                }
            }

            if (this->Application.ApplicationPackageReference.RolloutVersionValue !=
                targetDescription.Application.ApplicationPackageReference.RolloutVersionValue)
            {
                isAffected = true;

                if (this->Application.ApplicationPackageReference.RolloutVersionValue.Major !=
                    targetDescription.Application.ApplicationPackageReference.RolloutVersionValue.Major)
                {
                    shouldRestartPackages = true;

                    // Don't break early - we still need to determine if the service type has been
                    // moved between packages
                }
            }

            ServicePackageVersion currentPackageVersion;
            ServicePackageVersion targetPackageVersion;

            ErrorCode error = ServicePackageVersion::FromString(iter2->PackageVersion.Value, currentPackageVersion);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceSource, "Cannot parse '{0}' as a service package version", iter2->PackageVersion);
                return error;
            }

            error = ServicePackageVersion::FromString(iter->PackageVersion.Value, targetPackageVersion);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceSource, "Cannot parse '{0}' as a service package version", iter->PackageVersion);
                return error;
            }

            if (iter->PackageName != iter2->PackageName)
            {
                isMoved = true;

                this->AddAllCodePackages(iter->PackageName, codePackages);
            }
            else if (currentPackageVersion.RolloutVersionValue.Major != targetPackageVersion.RolloutVersionValue.Major)
            {
                isAffected = true;
                shouldRestartPackages = true;

                this->AddAllCodePackages(iter->PackageName, codePackages);
            }
            else if (currentPackageVersion.RolloutVersionValue != targetPackageVersion.RolloutVersionValue)
            {
                isAffected = true;

                if (this->HasCodePackageChange(
                    iter->PackageName,
                    current->second,
                    target->second,
                    codePackages))
                {
                    shouldRestartPackages = true;
                }
            }
        } // for each service type in the current application

        if (isAffected)
        {
            Trace.WriteNoise(
                TraceSource,
                "Adding Affected Service Type: {0}",
                *iter);
            affectedTypes.push_back(*iter);
        }

        if (isMoved)
        {
            Trace.WriteNoise(
                TraceSource,
                "Adding Moved Service Type: {0}",
                *iter);
            movedTypes.push_back(*iter);
        }

    } // for each service type in the target application

    affectedTypesResult.swap(affectedTypes);
    movedTypesResult.swap(movedTypes);
    codePackagesResult.swap(codePackages);
    shouldRestartPackagesResult = shouldRestartPackages;

    return ErrorCodeValue::Success;
}

bool DigestedApplicationDescription::HasCodePackageChange(
    ServiceModelPackageName const & servicePackageName,
    std::vector<ServiceModel::DigestedCodePackageDescription> const & current, 
    std::vector<ServiceModel::DigestedCodePackageDescription> const & target,
    __inout CodePackageNameMap & codePackages)
{
    bool hasCodePackageChange = false;

    for (auto iter = current.begin(); iter != current.end(); ++iter)
    {
        bool isFound = false;

        for (auto iter2 = target.begin(); iter2 != target.end(); ++iter2)
        {
            if (iter->CodePackage.Name == iter2->CodePackage.Name)
            {
                if (iter->RolloutVersionValue != iter2->RolloutVersionValue)
                {
                    this->AddCodePackage(
                        codePackages, 
                        servicePackageName, 
                        iter->CodePackage.Name);

                    hasCodePackageChange = true;
                }

                isFound = true;

                break;
            }
        }

        if (!isFound)
        {
            this->AddCodePackage(
                codePackages, 
                servicePackageName, 
                iter->CodePackage.Name);
        }
    }

    return hasCodePackageChange;
}

void DigestedApplicationDescription::AddAllCodePackages(
    ServiceModelPackageName const & packageName,
    __inout CodePackageNameMap & codePackages)
{
    for (auto const & pair : this->CodePackages)
    {
        if (pair.first == packageName)
        {
            for (auto const & description : pair.second)
            {
                this->AddCodePackage(
                    codePackages,
                    packageName,
                    description.Name);
            }
        }
    }
}

void DigestedApplicationDescription::AddCodePackage(
    __inout CodePackageNameMap & codePackages,
    ServiceModelPackageName const & servicePackageName,
    wstring const & codePackageName)
{
    auto findServicePackageIter = codePackages.find(servicePackageName);
    if (findServicePackageIter == codePackages.end())
    {
        auto result = codePackages.insert(make_pair(servicePackageName, vector<wstring>()));

        findServicePackageIter = result.first;
    }

    auto & codePackageList = findServicePackageIter->second;

    auto findCodePackageIter = find(codePackageList.begin(), codePackageList.end(), codePackageName);
    if (findCodePackageIter == codePackageList.end())
    {
        codePackageList.push_back(codePackageName);
    }
}

DigestedApplicationDescription::CodePackageNameMap DigestedApplicationDescription::GetAllCodePackageNames()
{
    CodePackageNameMap codePackages;

    for (auto const & pair : this->CodePackages)
    {
        for (auto const & description : pair.second)
        {
            this->AddCodePackage(
                codePackages,
                pair.first,
                description.Name);
        }
    }

    return codePackages;
}

std::vector<StoreDataServiceTemplate> DigestedApplicationDescription::ComputeRemovedServiceTemplates(
    DigestedApplicationDescription const & targetDescription)
{
    vector<StoreDataServiceTemplate> removedTemplates;

    for (auto iter = this->ServiceTemplates.begin(); iter != this->ServiceTemplates.end(); ++iter)
    {
        bool isFound = false;
        for (auto iter2 = targetDescription.ServiceTemplates.begin(); iter2 != targetDescription.ServiceTemplates.end(); ++iter2)
        {
            if (iter->TypeName == iter2->TypeName)
            {
                isFound = true;
                break;
            }
        }

        if (!isFound)
        {
            Trace.WriteNoise(
                TraceSource,
                "Adding Removed Service Template: {0}",
                *iter);
            removedTemplates.push_back(*iter);
        }
    }

    return removedTemplates;
}

std::vector<StoreDataServiceTemplate> DigestedApplicationDescription::ComputeModifiedServiceTemplates(
    DigestedApplicationDescription const & targetDescription)
{
    vector<StoreDataServiceTemplate> modifiedTemplates;

    for (auto iter = this->ServiceTemplates.begin(); iter != this->ServiceTemplates.end(); ++iter)
    {
        for (auto iter2 = targetDescription.ServiceTemplates.begin(); iter2 != targetDescription.ServiceTemplates.end(); ++iter2)
        {
            if (iter->TypeName == iter2->TypeName &&
                iter->PartitionedServiceDescriptor != iter2->PartitionedServiceDescriptor)
            {
                Trace.WriteNoise(
                    TraceSource,
                    "Adding Modified Service Template: current = {0} new = {1}",
                    *iter,
                    *iter2);

                modifiedTemplates.push_back(*iter2);

                break;
            }
        }
    }

    return modifiedTemplates;
}

std::vector<StoreDataServiceTemplate> DigestedApplicationDescription::ComputeAddedServiceTemplates(
    DigestedApplicationDescription const & targetDescription)
{
    vector<StoreDataServiceTemplate> addedTemplates;

    for (auto iter = targetDescription.ServiceTemplates.begin(); iter != targetDescription.ServiceTemplates.end(); ++iter)
    {
        bool isFound = false;
        for (auto iter2 = this->ServiceTemplates.begin(); iter2 != this->ServiceTemplates.end(); ++iter2)
        {
            if (iter->TypeName == iter2->TypeName)
            {
                isFound = true;
                break;
            }
        }

        if (!isFound)
        {
            Trace.WriteNoise(
                TraceSource,
                "Adding New Service Template: {0}",
                *iter);
            addedTemplates.push_back(*iter);
        }
    }

    return addedTemplates;
}

vector<ServicePackageReference> DigestedApplicationDescription::ComputeAddedServicePackages(DigestedApplicationDescription const & targetDescription)
{
    vector<ServicePackageReference> addedPackages;

    for (auto const & otherPkg : targetDescription.Application.ServicePackageReferences)
    {
        bool exists = false;

        for (auto const & thisPkg : this->Application.ServicePackageReferences)
        {
            if (thisPkg.Name == otherPkg.Name && thisPkg.RolloutVersionValue == otherPkg.RolloutVersionValue)
            {
                exists = true;
                break;
            }
        }

        if (!exists)
        {
            addedPackages.push_back(otherPkg);
        }
    }

    return move(addedPackages);
}