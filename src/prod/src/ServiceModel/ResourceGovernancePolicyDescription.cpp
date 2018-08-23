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
    CpuQuota(0),
    DiskQuotaInMB(0),
    KernelMemoryInMB(0),
    ShmSizeInMB(0)
{
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
        (this->CpuQuota == other.CpuQuota) &&
        (this->DiskQuotaInMB == other.DiskQuotaInMB) &&
        (this->KernelMemoryInMB == other.KernelMemoryInMB) &&
        (this->ShmSizeInMB == other.ShmSizeInMB);
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
    w.Write("DiskSizeInMB = {0} ", DiskQuotaInMB);
    w.Write("KernelMemoryInMB = {0} ", KernelMemoryInMB);
    w.Write("ShmSizeInMB = {0} ", ShmSizeInMB);
    w.Write("}");
}

wstring ResourceGovernancePolicyDescription::ToString() const
{
    return wformatString("ResourceGovernancePolicyDescription[ CpuShares = {0} , MemoryInMB = {1} ]", CpuShares, MemoryInMB);
}

void ResourceGovernancePolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <ResourceGovernancePolicy CodePackageRef="" MemoryInMB="" MemorySwapInMB="" MemoryReservationInMB="" CpuShares=""
    // CpuPercent="" MaximumIOps="" MaximumIOBytesps="" BlockIOWeight="" DiskQuotaInMB="" KernelMemoryInMB="" ShmSizeInMB="" />
    xmlReader->StartElement(
        *SchemaNames::Element_ResourceGovernancePolicy,
        *SchemaNames::Namespace);

    this->CodePackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CodePackageRef);

    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_MemoryInMB, this->MemoryInMB);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_MemorySwapInMB, this->MemorySwapInMB);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_MemoryReservationInMB, this->MemoryReservationInMB);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_CpuShares, this->CpuShares);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_CpuPercent, this->CpuPercent);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_MaximumIOps, this->MaximumIOps);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_MaximumIOBytesps, this->MaximumIOBytesps);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_BlockIOWeight, this->BlockIOWeight);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_DiskQuotaInMB, this->DiskQuotaInMB);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_KernelMemoryInMB, this->KernelMemoryInMB);
    ReadFromXmlHelper(xmlReader, *SchemaNames::Attribute_ShmSizeInMB, this->ShmSizeInMB);

    // Read the rest of the empty element
    xmlReader->ReadElement();
}

void ResourceGovernancePolicyDescription::ReadFromXmlHelper(XmlReaderUPtr const & xmlReader, wstring const & readAttrib, uint & valToUpdate)
{
    if (xmlReader->HasAttribute(readAttrib))
    {
        auto attrib = xmlReader->ReadAttributeValue(readAttrib);
        if (!StringUtility::TryFromWString<uint>(attrib, valToUpdate))
        {
            Parser::ThrowInvalidContent(xmlReader, L"positive integer", attrib);
        }
    }
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

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_DiskQuotaInMB, this->DiskQuotaInMB);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_KernelMemoryInMB, this->KernelMemoryInMB);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ShmSizeInMB, this->ShmSizeInMB);
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
    this->DiskQuotaInMB = 0;
    this->KernelMemoryInMB = 0;
    this->ShmSizeInMB = 0;
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

    auto fabricResourceGovernancePolicyDescEx1 = heap.AddItem<FABRIC_RESOURCE_GOVERNANCE_POLICY_DESCRIPTION_EX1>();
    fabricResourceGovernancePolicyDescEx1->DiskQuotaInMB = static_cast<ULONGLONG>(this->DiskQuotaInMB);
    fabricResourceGovernancePolicyDescEx1->KernelMemoryInMB = static_cast<ULONGLONG>(this->KernelMemoryInMB);
    fabricResourceGovernancePolicyDescEx1->ShmSizeInMB = static_cast<ULONGLONG>(this->ShmSizeInMB);
    fabricResourceGovernancePolicyDescEx1->Reserved = nullptr;

    fabricResourceGovernancePolicyDesc.Reserved = fabricResourceGovernancePolicyDescEx1.GetRawPointer();

    return ErrorCode::Success();
}

