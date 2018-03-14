// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

// ********************************************************************************************************************
// DllHostHostedManagedDllDescription Implementation
//
DllHostHostedManagedDllDescription::DllHostHostedManagedDllDescription()
    : AssemblyName()
{
}

DllHostHostedManagedDllDescription::DllHostHostedManagedDllDescription(DllHostHostedManagedDllDescription const & other)
    : AssemblyName(other.AssemblyName)
{
}

DllHostHostedManagedDllDescription::DllHostHostedManagedDllDescription(DllHostHostedManagedDllDescription && other)
    : AssemblyName(move(other.AssemblyName))
{
}

DllHostHostedManagedDllDescription const & DllHostHostedManagedDllDescription::operator = (DllHostHostedManagedDllDescription const & other)
{
    if (this != & other)
    {
        this->AssemblyName = other.AssemblyName;
    }

    return *this;
}

DllHostHostedManagedDllDescription const & DllHostHostedManagedDllDescription::operator = (DllHostHostedManagedDllDescription && other)
{
    if (this != & other)
    {
        this->AssemblyName = move(other.AssemblyName);
    }

    return *this;
}

bool DllHostHostedManagedDllDescription::operator == (DllHostHostedManagedDllDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->AssemblyName, other.AssemblyName);
    if (!equals) { return equals; }

    return equals;
}

bool DllHostHostedManagedDllDescription::operator != (DllHostHostedManagedDllDescription const & other) const
{
    return !(*this == other);
}

void DllHostHostedManagedDllDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("AssemblyName = {0}", AssemblyName);
}

void DllHostHostedManagedDllDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <ManagedAssembly
    xmlReader->StartElement(
        *SchemaNames::Element_ManagedAssembly,
        *SchemaNames::Namespace);

    // empty ManagedAssembly is not allowed
    xmlReader->ReadStartElement();
    this->AssemblyName = xmlReader->ReadElementValue(true);
    xmlReader->ReadEndElement();
}

ErrorCode DllHostHostedManagedDllDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	return xmlWriter->WriteElementWithContent(*SchemaNames::Element_ManagedAssembly, this->AssemblyName, L"", *SchemaNames::Namespace);
}

ErrorCode DllHostHostedManagedDllDescription::FromPublicApi(FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION const & publicDescription)
{
    this->AssemblyName = publicDescription.AssemblyName;
    return ErrorCode(ErrorCodeValue::Success);
}

void DllHostHostedManagedDllDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION & publicDescription) const
{
    publicDescription.AssemblyName = heap.AddString(this->AssemblyName);
}

void DllHostHostedManagedDllDescription::clear()
{
    AssemblyName.clear();
}

// ********************************************************************************************************************
// DllHostHostedUnmanagedDllDescription Implementation
//
DllHostHostedUnmanagedDllDescription::DllHostHostedUnmanagedDllDescription()
    : DllName()
{
}

DllHostHostedUnmanagedDllDescription::DllHostHostedUnmanagedDllDescription(DllHostHostedUnmanagedDllDescription const & other)
    : DllName(other.DllName)
{
}

DllHostHostedUnmanagedDllDescription::DllHostHostedUnmanagedDllDescription(DllHostHostedUnmanagedDllDescription && other)
    : DllName(move(other.DllName))
{
}

DllHostHostedUnmanagedDllDescription const & DllHostHostedUnmanagedDllDescription::operator = (DllHostHostedUnmanagedDllDescription const & other)
{
    if (this != & other)
    {
        this->DllName = other.DllName;
    }

    return *this;
}

DllHostHostedUnmanagedDllDescription const & DllHostHostedUnmanagedDllDescription::operator = (DllHostHostedUnmanagedDllDescription && other)
{
    if (this != & other)
    {
        this->DllName = move(other.DllName);
    }

    return *this;
}

bool DllHostHostedUnmanagedDllDescription::operator == (DllHostHostedUnmanagedDllDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->DllName, other.DllName);
    if (!equals) { return equals; }

    return equals;
}

bool DllHostHostedUnmanagedDllDescription::operator != (DllHostHostedUnmanagedDllDescription const & other) const
{
    return !(*this == other);
}

void DllHostHostedUnmanagedDllDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DllName = {0}", DllName);
}

void DllHostHostedUnmanagedDllDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <UnmanagedDll
    xmlReader->StartElement(
        *SchemaNames::Element_UnmanagedDll,
        *SchemaNames::Namespace);

    // empty UnmanagedAssembly is not allowed
    xmlReader->ReadStartElement();
    this->DllName = xmlReader->ReadElementValue(true);
    xmlReader->ReadEndElement();
}
ErrorCode DllHostHostedUnmanagedDllDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	return xmlWriter->WriteElementWithContent(*SchemaNames::Element_UnmanagedDll, this->DllName, L"", *SchemaNames::Namespace);
}

ErrorCode DllHostHostedUnmanagedDllDescription::FromPublicApi(FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION const & publicDescription)
{
    this->DllName = publicDescription.DllName;
    return ErrorCode(ErrorCodeValue::Success);
}

void DllHostHostedUnmanagedDllDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION & publicDescription) const
{
    publicDescription.DllName = heap.AddString(this->DllName);
}

void DllHostHostedUnmanagedDllDescription::clear()
{
    DllName.clear();
}


// ********************************************************************************************************************
// DllHostHostedDllDescription Implementation
//
DllHostHostedDllDescription::DllHostHostedDllDescription() :
    Kind(DllHostHostedDllKind::Invalid),
    Unmanaged(),
    Managed()
{
}

DllHostHostedDllDescription::DllHostHostedDllDescription(DllHostHostedDllDescription const & other) :
    Kind(other.Kind),
    Unmanaged(other.Unmanaged),
    Managed(other.Managed)
{
}

DllHostHostedDllDescription::DllHostHostedDllDescription(DllHostHostedDllDescription && other) :
    Kind(other.Kind),
    Unmanaged(move(other.Unmanaged)),
    Managed(move(other.Managed))
{
}

DllHostHostedDllDescription const & DllHostHostedDllDescription::operator = (DllHostHostedDllDescription const & other)
{
    if (this != & other)
    {
        this->Kind = other.Kind;
        this->Unmanaged = other.Unmanaged;
        this->Managed = other.Managed;
    }

    return *this;
}

DllHostHostedDllDescription const & DllHostHostedDllDescription::operator = (DllHostHostedDllDescription && other)
{
    if (this != & other)
    {
        this->Kind = other.Kind;
        this->Unmanaged = move(other.Unmanaged);
        this->Managed = move(other.Managed);
    }

    return *this;
}

bool DllHostHostedDllDescription::operator == (DllHostHostedDllDescription const & other) const
{
    bool equals = true;

    equals = this->Kind == other.Kind;
    if (!equals) { return equals; }

    switch (this->Kind)
    {
    case DllHostHostedDllKind::Unmanaged :
        equals = this->Unmanaged == other.Unmanaged;
        break;
    case DllHostHostedDllKind::Managed :
        equals = this->Managed == other.Managed;
        break;
    }
    if (!equals) { return equals; }

    return equals;
}

bool DllHostHostedDllDescription::operator != (DllHostHostedDllDescription const & other) const
{
    return !(*this == other);
}

void DllHostHostedDllDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DllHostHostedDllDescription { ");
    w.Write("Kind = {0}, ", Kind);
    switch (this->Kind)
    {
    case DllHostHostedDllKind::Unmanaged :
        w.Write(this->Unmanaged);
        break;
    case DllHostHostedDllKind::Managed :
        w.Write(this->Managed);
        break;
    }
    w.Write("}");
}

void DllHostHostedDllDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    if (xmlReader->IsStartElement(
            *SchemaNames::Element_UnmanagedDll,
            *SchemaNames::Namespace,
            false))
    {
        this->Kind = DllHostHostedDllKind::Unmanaged;
        this->Unmanaged.ReadFromXml(xmlReader);
    }
    else 
    {
        this->Kind = DllHostHostedDllKind::Managed;
        this->Managed.ReadFromXml(xmlReader);
    }
}

ErrorCode DllHostHostedDllDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	switch (this->Kind)
	{
	case DllHostHostedDllKind::Managed:
		er = this->Managed.WriteToXml(xmlWriter);
		break;
	case DllHostHostedDllKind::Unmanaged:
		er = this->Unmanaged.WriteToXml(xmlWriter);
		break;
	default:
		er = ErrorCodeValue::InvalidArgument;
	}
	return er;
}

ErrorCode DllHostHostedDllDescription::FromPublicApi(FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION const & description)
{
    auto error = DllHostHostedDllKind::FromPublicApi(description.Kind, this->Kind);
    if (!error.IsSuccess()) { return error; }

    if (this->Kind == DllHostHostedDllKind::Unmanaged)
    {
        auto unmanagedDescription = static_cast<FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION*>(description.Value);
        if (unmanagedDescription == nullptr) { return ErrorCode(ErrorCodeValue::InvalidArgument); }

        return Unmanaged.FromPublicApi(*unmanagedDescription);
    }
    else
    {
        auto managedDescription = static_cast<FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION*>(description.Value);
        if (managedDescription == nullptr) { return ErrorCode(ErrorCodeValue::InvalidArgument); }

        return Managed.FromPublicApi(*managedDescription);
    }
}

void DllHostHostedDllDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION & publicDescription) const
{
    publicDescription.Kind = DllHostHostedDllKind::ToPublicApi(this->Kind);

    switch (Kind)
    {
    case DllHostHostedDllKind::Unmanaged:
        {
            auto description = heap.AddItem<FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION>();
            this->Unmanaged.ToPublicApi(heap, *description);
            publicDescription.Value = description.GetRawPointer();
        }
        break;
    case DllHostHostedDllKind::Managed:
        {
            auto description = heap.AddItem<FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION>();
            this->Managed.ToPublicApi(heap, *description);
            publicDescription.Value = description.GetRawPointer();
        }
        break;
    default:
        publicDescription.Value = NULL;
    }
}

void DllHostHostedDllDescription::clear()
{
    this->Kind = DllHostHostedDllKind::Invalid;
    this->Unmanaged.clear();
    this->Managed.clear();
}

// ********************************************************************************************************************
// DllHostEntryPointDescription Implementation
//
DllHostEntryPointDescription::DllHostEntryPointDescription() :
    IsolationPolicyType(CodePackageIsolationPolicyType::DedicatedProcess),
    HostedDlls()
{
}

DllHostEntryPointDescription::DllHostEntryPointDescription(DllHostEntryPointDescription const & other) :
    IsolationPolicyType(other.IsolationPolicyType),
    HostedDlls(other.HostedDlls)
{
}

DllHostEntryPointDescription::DllHostEntryPointDescription(DllHostEntryPointDescription && other) :
    IsolationPolicyType(other.IsolationPolicyType),
    HostedDlls(move(other.HostedDlls))
{
}

DllHostEntryPointDescription const & DllHostEntryPointDescription::operator = (DllHostEntryPointDescription const & other)
{
    if (this != & other)
    {
        this->IsolationPolicyType = other.IsolationPolicyType;
        this->HostedDlls = other.HostedDlls;
    }

    return *this;
}

DllHostEntryPointDescription const & DllHostEntryPointDescription::operator = (DllHostEntryPointDescription && other)
{
    if (this != & other)
    {
        this->IsolationPolicyType = other.IsolationPolicyType;
        this->HostedDlls = move(other.HostedDlls);
    }

    return *this;
}

bool DllHostEntryPointDescription::operator == (DllHostEntryPointDescription const & other) const
{
    bool equals = true;

    equals = this->IsolationPolicyType == other.IsolationPolicyType;
    if (!equals) { return equals; }

    for (auto it = begin(HostedDlls); it != end(HostedDlls); ++it)
    {
        auto found = find(begin(other.HostedDlls), end(other.HostedDlls), *it);

        equals = found != end(other.HostedDlls);
        if (!equals) { return equals; }
    }

    return equals;
}

bool DllHostEntryPointDescription::operator != (DllHostEntryPointDescription const & other) const
{
    return !(*this == other);
}

void DllHostEntryPointDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DllHostEntryPointDescription { ");
    w.Write("IsolationPolicyType = {0}, ", IsolationPolicyType);    
    w.Write("Items = {");
    
    for (auto it = begin(HostedDlls); it != end(HostedDlls); ++it)
    {
        w.Write("{0}", *it);
    }

    w.Write("}");
}

void DllHostEntryPointDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DllHost
    xmlReader->StartElement(
        *SchemaNames::Element_DllHost,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsolationPolicy))
    {
        wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsolationPolicy);
        this->IsolationPolicyType = ParseIsolationPolicyType(xmlReader, attrValue);
    }

   // DllHost cannot be empty
    xmlReader->ReadStartElement();

    bool done = false;
    while(!done)
    {
        if (xmlReader->IsStartElement(
                *SchemaNames::Element_UnmanagedDll,
                *SchemaNames::Namespace,
                false) ||
            xmlReader->IsStartElement(
                *SchemaNames::Element_ManagedAssembly,
                *SchemaNames::Namespace,
                false)
            )
        {
            DllHostHostedDllDescription hostedDll;
            hostedDll.ReadFromXml(xmlReader);
            this->HostedDlls.push_back(move(hostedDll));
        }
        else
        {
            done = true;
        }
    }

    // </DllHost>
    xmlReader->ReadEndElement();
}

ErrorCode DllHostEntryPointDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DllHost, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}

	xmlWriter->WriteAttribute(*SchemaNames::Attribute_IsolationPolicy, this->EnumToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<DllHostHostedDllDescription>::iterator it = HostedDlls.begin(); it != HostedDlls.end(); ++it)
	{
		er = (*it).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return xmlWriter->WriteEndElement();
}

CodePackageIsolationPolicyType::Enum DllHostEntryPointDescription::ParseIsolationPolicyType(
    XmlReaderUPtr const & xmlReader,
    wstring const & value)
{
    CodePackageIsolationPolicyType::Enum retval = CodePackageIsolationPolicyType::Invalid;

    if (value == L"SharedDomain")
    {
        retval = CodePackageIsolationPolicyType::SharedDomain;
    }
    else if (value == L"DedicatedDomain")
    {
        retval = CodePackageIsolationPolicyType::DedicatedDomain;
    }
    else if (value == L"DedicatedProcess")
    {
        retval = CodePackageIsolationPolicyType::DedicatedProcess;
    }
    else
    {
        Parser::ThrowInvalidContent(
            xmlReader,
            L"SharedDomain/DedicatedDomain/DedicatedProcess",
            value);
    }

    return retval;
}

wstring DllHostEntryPointDescription::EnumToString()
{
	wstring retval;
	switch (this->IsolationPolicyType)
	{
	case CodePackageIsolationPolicyType::SharedDomain:
		retval = L"SharedDomain";
		break;
	case CodePackageIsolationPolicyType::DedicatedDomain:
		retval = L"DedicatedDomain";
		break;
	case CodePackageIsolationPolicyType::DedicatedProcess:
		retval = L"DedicatedProcess";
		break;	
	}

	return retval;
}

ErrorCode DllHostEntryPointDescription::FromPublicApi(FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION const & description)
{
    auto error = CodePackageIsolationPolicyType::FromPublicApi(description.IsolationPolicyType, this->IsolationPolicyType);
    if (!error.IsSuccess()) { return error; }

    auto descriptionList = description.HostedDlls;
    if (descriptionList != NULL)
    {
        for (size_t i = 0; i < descriptionList->Count; ++i)
        {
            DllHostHostedDllDescription item;
            error = item.FromPublicApi(descriptionList->Items[i]);
            if (!error.IsSuccess()) return error;
        
            this->HostedDlls.push_back(move(item));
        }
    }
    
    return ErrorCode::Success();
}

void DllHostEntryPointDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION & publicDescription) const
{
    auto descriptionList = heap.AddItem<FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION_LIST>();
    publicDescription.IsolationPolicyType = CodePackageIsolationPolicyType::ToPublicApi(this->IsolationPolicyType);    
    publicDescription.HostedDlls = descriptionList.GetRawPointer();
    publicDescription.HostedDlls->Count = (ULONG)this->HostedDlls.size();

    if (publicDescription.HostedDlls->Count > 0)
    {
        auto items = heap.AddArray<FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION>(publicDescription.HostedDlls->Count);
        for (size_t i = 0; i < publicDescription.HostedDlls->Count; ++i)
        {
            this->HostedDlls[i].ToPublicApi(heap, items[i]);
        }

        publicDescription.HostedDlls->Items = items.GetRawArray();
    }
    else
    {
        publicDescription.HostedDlls->Items = NULL;
    }
}

void DllHostEntryPointDescription::clear()
{
    this->IsolationPolicyType = CodePackageIsolationPolicyType::DedicatedProcess;
    this->HostedDlls.clear();
}
