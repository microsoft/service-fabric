// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ResourceGovernancePolicyDescription::ResourceGovernancePolicyDescription() 
    : CodePackageRef(),
    MemoryInMB(0),
    MemorySwapInMB(0),
    MemoryReservationInMB(0),
    CpuShares(0),
    CpuPercent(0),
    MaximumIOps(0),
    MaximumIOBytesps(0),
    BlockIOWeight(0),
    CpusetCpus(),
    NanoCpus(0),
    CpuQuota(0)
{
}

ResourceGovernancePolicyDescription::ResourceGovernancePolicyDescription(ResourceGovernancePolicyDescription const & other)
    : CodePackageRef(other.CodePackageRef),
    MemoryInMB(other.MemoryInMB),
    MemorySwapInMB(other.MemorySwapInMB),
    MemoryReservationInMB(other.MemoryReservationInMB),
    CpuShares(other.CpuShares),
    CpuPercent(other.CpuPercent),
    MaximumIOps(other.MaximumIOps),
    MaximumIOBytesps(other.MaximumIOBytesps),
    BlockIOWeight(other.BlockIOWeight),
    CpusetCpus(other.CpusetCpus),
    NanoCpus(other.NanoCpus),
    CpuQuota(other.CpuQuota)
{
}

ResourceGovernancePolicyDescription::ResourceGovernancePolicyDescription(ResourceGovernancePolicyDescription && other)
    : CodePackageRef(move(other.CodePackageRef)),
    MemoryInMB(other.MemoryInMB),
    MemorySwapInMB(other.MemorySwapInMB),
    MemoryReservationInMB(other.MemoryReservationInMB),
    CpuShares(other.CpuShares),
    CpuPercent(other.CpuPercent),
    MaximumIOps(other.MaximumIOps),
    MaximumIOBytesps(other.MaximumIOBytesps),
    BlockIOWeight(other.BlockIOWeight),
    CpusetCpus(move(other.CpusetCpus)),
    NanoCpus(other.NanoCpus),
    CpuQuota(other.CpuQuota)
{
}

ResourceGovernancePolicyDescription const & ResourceGovernancePolicyDescription::operator = (ResourceGovernancePolicyDescription const & other)
{
    if (this != &other)
    {
        this->CodePackageRef = other.CodePackageRef;
        this->MemoryInMB = other.MemoryInMB;
        this->MemorySwapInMB = other.MemorySwapInMB;
        this->MemoryReservationInMB = other.MemoryReservationInMB;
        this->CpuShares = other.CpuShares;
        this->CpuPercent = other.CpuPercent;
        this->MaximumIOps = other.MaximumIOps;
        this->MaximumIOBytesps = other.MaximumIOBytesps;
        this->BlockIOWeight = other.BlockIOWeight;
        this->CpusetCpus = other.CpusetCpus;
        this->NanoCpus = other.NanoCpus;
        this->CpuQuota = other.CpuQuota;
    }

    return *this;
}

ResourceGovernancePolicyDescription const & ResourceGovernancePolicyDescription::operator = (ResourceGovernancePolicyDescription && other)
{
    if (this != &other)
    {
        this->CodePackageRef = move(other.CodePackageRef);
        this->MemoryInMB = other.MemoryInMB;
        this->MemorySwapInMB = other.MemorySwapInMB;
        this->MemoryReservationInMB = other.MemoryReservationInMB;
        this->CpuShares = other.CpuShares;
        this->CpuPercent = other.CpuPercent;
        this->MaximumIOps = other.MaximumIOps;
        this->MaximumIOBytesps = other.MaximumIOBytesps;
        this->BlockIOWeight = other.BlockIOWeight;
        this->CpusetCpus = move(other.CpusetCpus);
        this->NanoCpus = other.NanoCpus;
        this->CpuQuota = other.CpuQuota;
    }

    return *this;
}

bool ResourceGovernancePolicyDescription::operator == (ResourceGovernancePolicyDescription const & other) const
{    
    return StringUtility::AreEqualCaseInsensitive(this->CodePackageRef, other.CodePackageRef) &&
        (this->MemoryInMB == other.MemoryInMB) &&
        (this->MemorySwapInMB == other.MemorySwapInMB) &&
        (this->MemoryReservationInMB == other.MemoryReservationInMB) &&
        (this->CpuShares == other.CpuShares) &&
        (this->CpuPercent == other.CpuPercent) &&
        (this->MaximumIOps == other.MaximumIOps) &&
        (this->MaximumIOBytesps == other.MaximumIOBytesps) &&
        (this->BlockIOWeight == other.BlockIOWeight) &&
        (StringUtility::AreEqualCaseInsensitive(this->CpusetCpus, other.CpusetCpus)) &&
        (this->NanoCpus == other.NanoCpus) &&
        (this->CpuQuota == other.CpuQuota);
}

bool ResourceGovernancePolicyDescription::operator != (ResourceGovernancePolicyDescription const & other) const
{
    return !(*this == other);
}

void ResourceGovernancePolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ResourceGovernancePolicyDescription { ");
    w.Write("CodePackageRef = {0}, ", CodePackageRef);
    w.Write("MemoryInMB = {0}, ", MemoryInMB);
    w.Write("MemorySwapInMB = {0} ", MemorySwapInMB);
    w.Write("MemoryReservationInMB = {0} ", MemoryReservationInMB);
    w.Write("CpuShares = {0} ", CpuShares);
    w.Write("CpuPercent = {0} ", CpuPercent);
    w.Write("MaximumIOps = {0} ", MaximumIOps);
    w.Write("MaximumIOBytesps = {0} ", MaximumIOBytesps);
    w.Write("BlockIOWeight = {0} ", BlockIOWeight);
    w.Write("CpusetCpus = {0} ", CpusetCpus);
    w.Write("NanoCpus = {0} ", NanoCpus);
    w.Write("CpuQuota = {0} ", CpuQuota);
    w.Write("}");
}

wstring ResourceGovernancePolicyDescription::ToString() const
{
    return wformatString("ResourceGovernancePolicyDescription[ CpuShares = {0} , MemoryInMB = {1} ]", CpuShares, MemoryInMB);
}

void ResourceGovernancePolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <ResourceGovernancePolicy CodePackageRef="" MemoryInMB="" MemorySwapInMB="" MemoryReservationInMB="" CpuShares=""/>
    xmlReader->StartElement(
        *SchemaNames::Element_ResourceGovernancePolicy,
        *SchemaNames::Namespace);

    this->CodePackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CodePackageRef);
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_MemoryInMB))
    {
        auto memoryInMB = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_MemoryInMB);
        if (!StringUtility::TryFromWString<uint>(
            memoryInMB,
            this->MemoryInMB))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", memoryInMB);
        }
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MemorySwapInMB))
    {
        auto memorySwap = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_MemorySwapInMB);
        if (!StringUtility::TryFromWString<uint>(
            memorySwap,
            this->MemorySwapInMB))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", memorySwap);
        }
    }
  
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MemoryReservationInMB))
    {
        auto memoryReservation = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_MemoryReservationInMB);
        if (!StringUtility::TryFromWString<uint>(
            memoryReservation,
            this->MemoryReservationInMB))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", memoryReservation);
        }
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_CpuShares))
    {
        auto cpuShares = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CpuShares);
        if (!StringUtility::TryFromWString<uint>(
            cpuShares,
            this->CpuShares))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", cpuShares);
        }
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_CpuPercent))
    {
        auto cpuPercent = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CpuPercent);
        if (!StringUtility::TryFromWString<uint>(
            cpuPercent,
            this->CpuPercent))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", cpuPercent);
        }
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MaximumIOps))
    {
        auto maximumIOps = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_MaximumIOps);
        if (!StringUtility::TryFromWString<uint>(
            maximumIOps,
            this->MaximumIOps))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", maximumIOps);
        }
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MaximumIOBytesps))
    {
        auto maximumIOBps = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_MaximumIOBytesps);
        if (!StringUtility::TryFromWString<uint>(
            maximumIOBps,
            this->MaximumIOBytesps))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", maximumIOBps);
        }
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_BlockIOWeight))
    {
        auto blkioweight = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_BlockIOWeight);
        if (!StringUtility::TryFromWString<uint>(
            blkioweight,
            this->BlockIOWeight))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", blkioweight);
        }
    }
    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode ResourceGovernancePolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    //<ResourceGovernancePolicy>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ResourceGovernancePolicy, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    //All the attributes
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CodePackageRef, this->CodePackageRef);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MemoryInMB, this->MemoryInMB);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MemorySwapInMB, this->MemorySwapInMB);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MemoryReservationInMB, this->MemoryReservationInMB);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_CpuShares, this->CpuShares);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_CpuPercent, this->CpuPercent);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MaximumIOps, this->MaximumIOps);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MaximumIOBytesps, this->MaximumIOBytesps);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_BlockIOWeight, this->BlockIOWeight);
    if (!er.IsSuccess())
    {
        return er;
    }
    //</ResourceGovernancePolicy>
    return xmlWriter->WriteEndElement();
}

void ResourceGovernancePolicyDescription::clear()
{
    this->CodePackageRef.clear();
    this->MemoryInMB = 0;
    this->MemorySwapInMB = 0;
    this->MemoryReservationInMB = 0;
    this->CpuShares = 0;
    this->CpuPercent = 0;
    this->MaximumIOps = 0;
    this->MaximumIOBytesps = 0;
    this->BlockIOWeight = 0;
    this->CpusetCpus.clear();
    this->NanoCpus = 0;
    this->CpuQuota = 0;
}

bool ResourceGovernancePolicyDescription::ShouldSetupCgroup() const
{
    return this->CpuQuota > 0 || this->MemoryInMB > 0;
}

ErrorCode ResourceGovernancePolicyDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_RESOURCE_GOVERNANCE_POLICY_DESCRIPTION & fabricResourceGovernancePolicyDesc) const
{
    fabricResourceGovernancePolicyDesc.CodePackageRef = heap.AddString(this->CodePackageRef);
    fabricResourceGovernancePolicyDesc.MemoryInMB = static_cast<ULONG>(this->MemoryInMB);
    fabricResourceGovernancePolicyDesc.MemorySwapInMB = static_cast<ULONG>(this->MemorySwapInMB);
    fabricResourceGovernancePolicyDesc.MemoryReservationInMB = static_cast<ULONG>(this->MemoryReservationInMB);
    fabricResourceGovernancePolicyDesc.CpuShares = static_cast<ULONG>(this->CpuShares);
    fabricResourceGovernancePolicyDesc.CpuPercent = static_cast<ULONG>(this->CpuPercent);
    fabricResourceGovernancePolicyDesc.MaximumIOps = static_cast<ULONG>(this->MaximumIOps);
    fabricResourceGovernancePolicyDesc.MaximumIOBytesps = static_cast<ULONG>(this->MaximumIOBytesps);
    fabricResourceGovernancePolicyDesc.BlockIOWeight = static_cast<ULONG>(this->BlockIOWeight);
    fabricResourceGovernancePolicyDesc.CpusetCpus = heap.AddString(this->CpusetCpus);
    fabricResourceGovernancePolicyDesc.NanoCpus = static_cast<ULONGLONG>(this->NanoCpus);
    fabricResourceGovernancePolicyDesc.CpuQuota = static_cast<ULONG>(this->CpuQuota);

    fabricResourceGovernancePolicyDesc.Reserved = nullptr;

    return ErrorCode::Success();
}

