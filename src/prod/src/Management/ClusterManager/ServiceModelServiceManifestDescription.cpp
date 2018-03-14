// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::ImageModel;
using namespace Management::ClusterManager;

ServiceModelServiceManifestDescription::ServiceModelServiceManifestDescription()
    : name_()
    , version_()
    , content_()
    , serviceTypeNames_()
    , serviceGroupTypeNames_()
    , codePackages_()
    , configPackages_()
    , dataPackages_()
{
}

ServiceModelServiceManifestDescription::ServiceModelServiceManifestDescription(std::wstring const & name, std::wstring const & version)
    : name_(name)
    , version_(version)
    , content_()
    , serviceTypeNames_()
    , serviceGroupTypeNames_()
    , codePackages_()
    , configPackages_()
    , dataPackages_()
{
}

void ServiceModelServiceManifestDescription::AddServiceModelType(ServiceModel::ServiceTypeDescription const & serviceType)
{
    serviceTypeNames_.push_back(serviceType);
}

void ServiceModelServiceManifestDescription::AddServiceGroupModelType(ServiceModel::ServiceGroupTypeDescription const & serviceGroupType)
{
    serviceGroupTypeNames_.push_back(serviceGroupType);
}

void ServiceModelServiceManifestDescription::AddCodePackage(std::wstring const & name, std::wstring const & version)
{
    codePackages_.AddPackage(name, version);
}

void ServiceModelServiceManifestDescription::AddConfigPackage(std::wstring const & name, std::wstring const & version)
{
    configPackages_.AddPackage(name, version);
}

void ServiceModelServiceManifestDescription::AddDataPackage(std::wstring const & name, std::wstring const & version)
{
    dataPackages_.AddPackage(name, version);
}

bool ServiceModelServiceManifestDescription::HasCodePackage(std::wstring const & name, std::wstring const & version)
{
    return codePackages_.HasPackage(name, version);
}

bool ServiceModelServiceManifestDescription::HasConfigPackage(std::wstring const & name, std::wstring const & version)
{
    return configPackages_.HasPackage(name, version);
}

bool ServiceModelServiceManifestDescription::HasDataPackage(std::wstring const & name, std::wstring const & version)
{
    return dataPackages_.HasPackage(name, version);
}

void ServiceModelServiceManifestDescription::TrimDuplicatePackages(ServiceModelServiceManifestDescription const & other)
{
    codePackages_.TrimDuplicatePackages(other.codePackages_);
    configPackages_.TrimDuplicatePackages(other.configPackages_);
    dataPackages_.TrimDuplicatePackages(other.dataPackages_);
}

vector<wstring> ServiceModelServiceManifestDescription::GetFilenames(
    StoreLayoutSpecification const & layout, 
    ServiceModelTypeName const & appTypeName) const
{
    vector<wstring> filenames;

    // Delete checksum files first - ImageBuilder uses the checksum files to mark a completed upload
    //
    filenames.push_back(layout.GetServiceManifestChecksumFile(appTypeName.Value, name_, version_));
    filenames.push_back(layout.GetServiceManifestFile(appTypeName.Value, name_, version_));

    for (auto it = codePackages_.Map.begin(); it != codePackages_.Map.end(); ++it)
    {
        filenames.push_back(layout.GetCodePackageChecksumFile(appTypeName.Value, name_, it->first, it->second));
        wstring packageName = layout.GetCodePackageFolder(appTypeName.Value, name_, it->first, it->second);
        // The package may be compressed, add the zip name as well as the folder name
        filenames.push_back(layout.GetSubPackageArchiveFile(packageName));
        filenames.push_back(move(packageName));
    }

    for (auto it = configPackages_.Map.begin(); it != configPackages_.Map.end(); ++it)
    {
        filenames.push_back(layout.GetConfigPackageChecksumFile(appTypeName.Value, name_, it->first, it->second));
        wstring packageName = layout.GetConfigPackageFolder(appTypeName.Value, name_, it->first, it->second);
        // The package may be compressed, add the zip name as well as the folder name
        filenames.push_back(layout.GetSubPackageArchiveFile(packageName));
        filenames.push_back(move(packageName));
    }

    for (auto it = dataPackages_.Map.begin(); it != dataPackages_.Map.end(); ++it)
    {
        filenames.push_back(layout.GetDataPackageChecksumFile(appTypeName.Value, name_, it->first, it->second));
        wstring packageName = layout.GetDataPackageFolder(appTypeName.Value, name_, it->first, it->second);
        // The package may be compressed, add the zip name as well as the folder name
        filenames.push_back(layout.GetSubPackageArchiveFile(packageName));
        filenames.push_back(move(packageName));
    }

    return move(filenames);
}

void ServiceModelServiceManifestDescription::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("[{0}, {1} : ", name_, version_);
    w.Write("ServiceTypeNames = {0}, ", serviceTypeNames_);
    w.Write("Code = {0}, ", codePackages_);
    w.Write("Config = {0}, ", configPackages_);
    w.Write("Data = {0}", dataPackages_);
    w.Write("]");
}

// *** inner class

void ServiceModelServiceManifestDescription::PackageMap::AddPackage(std::wstring const & name, std::wstring const & version)
{
    map_.insert(NameVersionPair(name, version));
}

bool ServiceModelServiceManifestDescription::PackageMap::HasPackage(std::wstring const & name, std::wstring const & version)
{
    auto it = map_.find(name);
    if (it != map_.end())
    {
        return (it->second == version);
    }

    return false;
}

void ServiceModelServiceManifestDescription::PackageMap::TrimDuplicatePackages(PackageMap const & other)
{
    vector<wstring> toErase;
    for (auto itThis = map_.begin(); itThis != map_.end(); ++itThis)
    {
        for (auto itOther = other.map_.begin(); itOther != other.map_.end(); ++itOther)
        {
            if (itThis->first == itOther->first && itThis->second == itOther->second)
            {
                toErase.push_back(itThis->first);
                break;
            }
        }
    }

    for (auto it = toErase.begin(); it != toErase.end(); ++it)
    {
        map_.erase(*it);
    }
}

void ServiceModelServiceManifestDescription::PackageMap::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("( ");
    for (auto it = map_.begin(); it != map_.end(); ++it)
    {
        w.Write("{0}:{1} ", it->first, it->second); 
    }
    w.Write(")");
}
