// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EntryPointDescription::EntryPointDescription() :
    DllHostEntryPoint(),
    ExeEntryPoint(),
    ContainerEntryPoint(),
    EntryPointType(EntryPointType::None)
{
}

EntryPointDescription::EntryPointDescription(EntryPointDescription const & other) :
    DllHostEntryPoint(other.DllHostEntryPoint),
    ExeEntryPoint(other.ExeEntryPoint),
    ContainerEntryPoint(other.ContainerEntryPoint),
    EntryPointType(other.EntryPointType)
{
}

EntryPointDescription::EntryPointDescription(EntryPointDescription && other) :
    DllHostEntryPoint(move(other.DllHostEntryPoint)),
    ExeEntryPoint(move(other.ExeEntryPoint)),
    ContainerEntryPoint(move(other.ContainerEntryPoint)),
    EntryPointType(other.EntryPointType)
{
}

EntryPointDescription const & EntryPointDescription::operator = (EntryPointDescription const & other)
{
    if (this != & other)
    {
        this->EntryPointType = other.EntryPointType;
        this->DllHostEntryPoint = other.DllHostEntryPoint;
        this->ExeEntryPoint = other.ExeEntryPoint;
        this->ContainerEntryPoint = other.ContainerEntryPoint;
    }

    return *this;
}

EntryPointDescription const & EntryPointDescription::operator = (EntryPointDescription && other)
{
    if (this != & other)
    {
       this->EntryPointType = other.EntryPointType;
       this->DllHostEntryPoint = move(other.DllHostEntryPoint);
       this->ExeEntryPoint = move(other.ExeEntryPoint);
       this->ContainerEntryPoint = move(other.ContainerEntryPoint);
    }

    return *this;
}

bool EntryPointDescription::operator == (EntryPointDescription const & other) const
{
    bool equals = true;

    equals = this->EntryPointType == other.EntryPointType;
    if (!equals) { return equals; }

    switch (this->EntryPointType)
    {
    case EntryPointType::Exe :
        equals = this->ExeEntryPoint == other.ExeEntryPoint;
        break;
    case EntryPointType::DllHost :
        equals = this->DllHostEntryPoint == other.DllHostEntryPoint;
        break;
    case EntryPointType::ContainerHost :
        equals = this->ContainerEntryPoint == other.ContainerEntryPoint;
        break;
    }
    if (!equals) { return equals; }

    return equals;
}

bool EntryPointDescription::operator != (EntryPointDescription const & other) const
{
    return !(*this == other);
}

void EntryPointDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("EntryPointDescription { ");

    w.Write("EntryPointType = {0}", this->EntryPointType);

    switch (this->EntryPointType)
    {
    case EntryPointType::Exe :
        w.Write("Executable = {0}", this->ExeEntryPoint);
        break;
    case EntryPointType::DllHost :
        w.Write("DllHost = {0}", this->DllHostEntryPoint);
        break;
    case EntryPointType::ContainerHost:
        w.Write("ContainerHost = {0}", this->ContainerEntryPoint);
        break;
    }

    w.Write("}");
}

void EntryPointDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    this->EntryPointType = EntryPointType::None;

    xmlReader->StartElement(
        *SchemaNames::Element_EntryPoint,
        *SchemaNames::Namespace,
        false);

    xmlReader->ReadStartElement();

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ExeHost,
        *SchemaNames::Namespace,
        false))
    {
        this->EntryPointType = EntryPointType::Exe;
        this->ExeEntryPoint.ReadFromXml(xmlReader);
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_DllHost,
        *SchemaNames::Namespace,
        false))
    {
        this->EntryPointType = EntryPointType::DllHost;
        this->DllHostEntryPoint.ReadFromXml(xmlReader);
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_ContainerHost,
        *SchemaNames::Namespace,
        false))
    {
        this->EntryPointType = EntryPointType::ContainerHost;
        this->ContainerEntryPoint.ReadFromXml(xmlReader);
    }
    xmlReader->ReadEndElement();
}

ErrorCode EntryPointDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	switch (this->EntryPointType)
	{
	case EntryPointType::Exe:
		er = this->ExeEntryPoint.WriteToXml(xmlWriter);
		break;
	case EntryPointType::DllHost:
		er = this->DllHostEntryPoint.WriteToXml(xmlWriter);
		break;
    case EntryPointType::ContainerHost:
        er = this->ContainerEntryPoint.WriteToXml(xmlWriter);
	default:
		er = ErrorCodeValue::InvalidConfiguration;
	}
	return er;
}

ErrorCode EntryPointDescription::FromPublicApi(FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION const & description)
{
    auto error = EntryPointType::FromPublicApi(description.Kind, this->EntryPointType);
    if (!error.IsSuccess()) { return error; }

    switch (EntryPointType)
    {
    case EntryPointType::Exe :
        {
            auto exeDescription = static_cast<FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION*>(description.Value);
            if (exeDescription == nullptr) { return ErrorCode(ErrorCodeValue::InvalidArgument); }
            error = ExeEntryPoint.FromPublicApi(*exeDescription);
        }
        break;
    case EntryPointType::DllHost :
        {
            auto hostDescription = static_cast<FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION*>(description.Value);
            if (hostDescription == nullptr) { return ErrorCode(ErrorCodeValue::InvalidArgument); }
            error = DllHostEntryPoint.FromPublicApi(*hostDescription);
        }
        break;
    case EntryPointType::ContainerHost:
        {
            auto hostDescription = static_cast<FABRIC_CONTAINERHOST_ENTRY_POINT_DESCRIPTION*>(description.Value);
            if (hostDescription == nullptr) { return ErrorCode(ErrorCodeValue::InvalidArgument); }
            error = ContainerEntryPoint.FromPublicApi(*hostDescription);
        }
        break;
    }

    return ErrorCode::Success();
}

void EntryPointDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION & publicDescription) const
{
    publicDescription.Kind = EntryPointType::ToPublicApi(this->EntryPointType);

    switch (EntryPointType)
    {
    case EntryPointType::Exe:
        {
            auto exe = heap.AddItem<FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION>();
            this->ExeEntryPoint.ToPublicApi(heap, *exe);
            publicDescription.Value = exe.GetRawPointer();
        }
        break;
    case EntryPointType::DllHost:
        {
            auto host = heap.AddItem<FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION>();
            this->DllHostEntryPoint.ToPublicApi(heap, *host);
            publicDescription.Value = host.GetRawPointer();
        }
        break;
    case EntryPointType::ContainerHost:
        {
            auto host = heap.AddItem<FABRIC_CONTAINERHOST_ENTRY_POINT_DESCRIPTION>();
            this->ContainerEntryPoint.ToPublicApi(heap, *host);
            publicDescription.Value = host.GetRawPointer();
        }
        break;
    default:
        publicDescription.Value = NULL;
    }
}

void EntryPointDescription::clear()
{
    this->EntryPointType = EntryPointType::None;
    this->ExeEntryPoint.clear();
    this->DllHostEntryPoint.clear();
    this->ContainerEntryPoint.clear();
} 

CodePackageIsolationPolicyType::Enum EntryPointDescription::get_IsolationPolicy() const
{
    if (this->EntryPointType == EntryPointType::Exe)
    {
        return CodePackageIsolationPolicyType::DedicatedProcess;
    }
    else if (this->EntryPointType == EntryPointType::ContainerHost)
    {
        return CodePackageIsolationPolicyType::Container;
    }
    else
    {
        return this->DllHostEntryPoint.IsolationPolicyType;
    }
}
