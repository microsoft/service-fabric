// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServicePackageResourceGovernanceDescription::ServicePackageResourceGovernanceDescription()
    : IsGoverned(false),
    cpuCores_(0.0),
    MemoryInMB(0),
    notUsed_(0)
{
}

ServicePackageResourceGovernanceDescription::ServicePackageResourceGovernanceDescription(ServicePackageResourceGovernanceDescription const & other)
    : IsGoverned(other.IsGoverned),
    cpuCores_(other.cpuCores_),
    MemoryInMB(other.MemoryInMB),
    notUsed_(other.notUsed_)
{
}

ServicePackageResourceGovernanceDescription::ServicePackageResourceGovernanceDescription(ServicePackageResourceGovernanceDescription && other)
    : IsGoverned(other.IsGoverned),
    cpuCores_(other.cpuCores_),
    MemoryInMB(other.MemoryInMB),
    notUsed_(other.notUsed_)
{
}

ServicePackageResourceGovernanceDescription const & ServicePackageResourceGovernanceDescription::operator=(ServicePackageResourceGovernanceDescription const & other)
{
    if (this != &other)
    {
        IsGoverned = other.IsGoverned;
        cpuCores_ = other.cpuCores_;
        MemoryInMB = other.MemoryInMB;
        notUsed_ = other.notUsed_;
    }
    return *this;
}

ServicePackageResourceGovernanceDescription const & ServicePackageResourceGovernanceDescription::operator=(ServicePackageResourceGovernanceDescription && other)
{
    if (this != &other)
    {
        IsGoverned = other.IsGoverned;
        cpuCores_ = other.cpuCores_;
        MemoryInMB = other.MemoryInMB;
        notUsed_ = other.notUsed_;
    }
    return *this;
}

bool ServicePackageResourceGovernanceDescription::operator==(ServicePackageResourceGovernanceDescription const & other) const
{
    return IsGoverned == other.IsGoverned &&
        cpuCores_ == other.cpuCores_ &&
        MemoryInMB == other.MemoryInMB;
}

bool ServicePackageResourceGovernanceDescription::operator!=(ServicePackageResourceGovernanceDescription const & other) const
{
    return !(*this == other);
}

void ServicePackageResourceGovernanceDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

void ServiceModel::ServicePackageResourceGovernanceDescription::clear()
{
    IsGoverned = false;
    cpuCores_ = 0;
    MemoryInMB = 0;
    notUsed_ = 0;
}

void ServicePackageResourceGovernanceDescription::ReadFromXml(Common::XmlReaderUPtr const & xmlReader)
{
    clear();

    // <ServicePackageResourceGovernancePolicy CpuCores="" MemoryInMB="">
    xmlReader->StartElement(
        *SchemaNames::Element_ServicePackageResourceGovernancePolicy,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MemoryInMB))
    {
        auto memoryInMB = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_MemoryInMB);
        if (!StringUtility::TryFromWString<uint>(
            memoryInMB,
            this->MemoryInMB))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", memoryInMB);
        }
        IsGoverned = true;
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_CpuCores))
    {
        auto cpuCores = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CpuCores);
        if (!StringUtility::TryFromWString<double>(
            cpuCores,
            this->cpuCores_))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive number", cpuCores);
        }
        IsGoverned = true;
    }

    // Read the rest of the empty element
    xmlReader->ReadElement();
}

std::wstring ServicePackageResourceGovernanceDescription::ToString() const
{
    return wformatString(
        "ServicePackageRGDescription (CpuCores = {0}, MemoryInMB = {1})", CpuCores, MemoryInMB);
}

ErrorCode ServicePackageResourceGovernanceDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
    if (!IsGoverned)
    {
        return ErrorCodeValue::Success;
    }

    ErrorCode er;
    er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServicePackageResourceGovernancePolicy, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    if (this->CpuCores > 0)
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CpuCores, StringUtility::ToWString<double>(this->CpuCores));
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (this->MemoryInMB > 0)
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_MemoryInMB, StringUtility::ToWString<uint>(this->MemoryInMB));
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    return xmlWriter->WriteEndElement();
}

void ServicePackageResourceGovernanceDescription::CorrectCPULimits()
{
    if (notUsed_ != 0 && cpuCores_ == 0.0)
    {
        // When deserializing from old versions, CpuCores will be zero and notUsed_ will contain the number of cores.
        // PLB and LRM are using only CpuCores so we need to set it correctly.
        cpuCores_ = static_cast<double>(notUsed_);
    }
}

void ServicePackageResourceGovernanceDescription::SetMemoryInMB(std::vector<DigestedCodePackageDescription> const& codePackages)
{
    if (0 != MemoryInMB || 0 == codePackages.size())
    {
        // User has provided the value at SP level, or there are no CPs.
        return;
    }

    uint newValue = 0;

    for (auto codePackageDescription : codePackages)
    {
        if (0 == codePackageDescription.ResourceGovernancePolicy.MemoryInMB)
        {
            // All code packages must have MemoryInMB defined
            return;
        }
        newValue += codePackageDescription.ResourceGovernancePolicy.MemoryInMB;
    }

    MemoryInMB = newValue;
    IsGoverned = true;
}

bool ServicePackageResourceGovernanceDescription::HasRgChange(
    ServicePackageResourceGovernanceDescription const & other,
    std::vector<DigestedCodePackageDescription> const& myCodePackages,
    std::vector<DigestedCodePackageDescription> const& otherCodePackages) const
{
    bool cpuCoresUsed = other.CpuCores > 0;

    if (*this != other)
    {
        return true;
    }

    for (auto iter = myCodePackages.begin(); iter != myCodePackages.end(); ++iter)
    {
        bool isFound = false;

        for (auto iter2 = otherCodePackages.begin(); iter2 != otherCodePackages.end(); ++iter2)
        {
            if (iter->Name == iter2->Name && iter->ResourceGovernancePolicy.CodePackageRef == iter2->ResourceGovernancePolicy.CodePackageRef)
            {
                isFound = true;
                if (iter->ResourceGovernancePolicy != iter2->ResourceGovernancePolicy)
                {
                    return true;
                }
            }
        }
        //removing this CP affects other CPs as well as now they might get more cpu time
        if (!isFound && iter->ResourceGovernancePolicy.CpuShares > 0 && cpuCoresUsed)
        {
            return true;
        }
    }

    for (auto iter = otherCodePackages.begin(); iter != otherCodePackages.end(); ++iter)
    {
        bool isFound = false;

        for (auto iter2 = myCodePackages.begin(); iter2 != myCodePackages.end(); ++iter2)
        {
            if (iter->Name == iter2->Name && iter->ResourceGovernancePolicy.CodePackageRef == iter2->ResourceGovernancePolicy.CodePackageRef)
            {
                isFound = true;
            }
        }
        //add this CP can affect other CPs as well as they might get less cpu time
        if (!isFound && iter->ResourceGovernancePolicy.CpuShares > 0 && cpuCoresUsed)
        {
            return true;
        }
    }
    //if we are using cpu cores and the number of CPs is changing we may need to readjust
    if (myCodePackages.size() != otherCodePackages.size() && cpuCoresUsed)
    {
        return true;
    }

    return false;
}

ErrorCode ServicePackageResourceGovernanceDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_SERVICE_PACKAGE_RESOURCE_GOVERNANCE_DESCRIPTION & fabricSvcPkgResGovDesc) const
{
    UNREFERENCED_PARAMETER(heap);
    
    fabricSvcPkgResGovDesc.IsGoverned = this->IsGoverned;
    fabricSvcPkgResGovDesc.MemoryInMB = static_cast<ULONG>(this->MemoryInMB);
    fabricSvcPkgResGovDesc.CpuCores = this->cpuCores_;
    fabricSvcPkgResGovDesc.NotUsed = static_cast<ULONG>(this->notUsed_);

    fabricSvcPkgResGovDesc.Reserved = nullptr;

    return ErrorCode::Success();
}
