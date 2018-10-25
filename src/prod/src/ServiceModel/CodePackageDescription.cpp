// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

CodePackageDescription::CodePackageDescription() :
    Name(),
    Version(),
    IsActivator(false),
    IsShared(false),
    SetupEntryPoint(),
    HasSetupEntryPoint(false),
    EntryPoint(),
    EnvironmentVariables()
{
}

CodePackageDescription::CodePackageDescription(CodePackageDescription const & other) :
    Name(other.Name),
    Version(other.Version),
    IsActivator(other.IsActivator),
    IsShared(other.IsShared),
    SetupEntryPoint(other.SetupEntryPoint),
    HasSetupEntryPoint(other.HasSetupEntryPoint),
    EntryPoint(other.EntryPoint),
    EnvironmentVariables(other.EnvironmentVariables)
{
}

CodePackageDescription::CodePackageDescription(CodePackageDescription && other) :
    Name(move(other.Name)),
    Version(move(other.Version)),
    IsActivator(other.IsActivator),
    IsShared(other.IsShared),
    SetupEntryPoint(move(other.SetupEntryPoint)),
    HasSetupEntryPoint(other.HasSetupEntryPoint),
    EntryPoint(move(other.EntryPoint)),
    EnvironmentVariables(move(other.EnvironmentVariables))
{
}

CodePackageDescription const & CodePackageDescription::operator = (CodePackageDescription const & other)
{
    if (this != & other)
    {
        this->Name = other.Name;
        this->Version = other.Version;
        this->IsActivator = other.IsActivator;
        this->IsShared = other.IsShared;
        this->SetupEntryPoint = other.SetupEntryPoint;
        this->HasSetupEntryPoint = other.HasSetupEntryPoint;
        this->EntryPoint = other.EntryPoint;
        this->EnvironmentVariables = other.EnvironmentVariables;
    }

    return *this;
}

CodePackageDescription const & CodePackageDescription::operator = (CodePackageDescription && other)
{
    if (this != & other)
    {
        this->Name = move(other.Name);
        this->Version = move(other.Version);
        this->IsActivator = other.IsActivator;
        this->IsShared = other.IsShared;
        this->SetupEntryPoint = move(other.SetupEntryPoint);
        this->HasSetupEntryPoint = other.HasSetupEntryPoint;
        this->EntryPoint = move(other.EntryPoint);
        this->EnvironmentVariables = move(other.EnvironmentVariables);
    }

    return *this;
}

bool CodePackageDescription::operator == (CodePackageDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Version, other.Version);
    if (!equals) { return equals; }
    
    equals = this->IsShared == other.IsShared;
    if (!equals) { return equals; }

    equals = this->IsActivator == other.IsActivator;
    if (!equals) { return equals; }

    equals = this->HasSetupEntryPoint == other.HasSetupEntryPoint;
    if (!equals) { return equals; }

    equals = this->SetupEntryPoint == other.SetupEntryPoint;
    if (!equals) { return equals; }

    equals = this->EntryPoint == other.EntryPoint;
    if (!equals) { return equals; }    
    equals = this->EnvironmentVariables == other.EnvironmentVariables;
    if (!equals) { return equals; }

    return equals;
}

bool CodePackageDescription::operator != (CodePackageDescription const & other) const
{
    return !(*this == other);
}

void CodePackageDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CodePackageDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Version = {0}, ", Version);
    w.Write("IsActivator = {0}, ", IsActivator);
    w.Write("IsShared = {0}, ", IsShared);
    w.Write("HasSetupEntryPoint = {0}, ", HasSetupEntryPoint);
    w.Write("SetupEntryPoint = {0}, ", SetupEntryPoint);
    w.Write("EntryPoint = {0}, ", EntryPoint);
    w.Write("}");
}

void CodePackageDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <CodePackage
    xmlReader->StartElement(
        *SchemaNames::Element_CodePackage,
        *SchemaNames::Namespace);

    // Attributes <CodePackage Name="" Version=""
    {
        this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
        this->Version = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Version);
        
        if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsShared))
        {
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsShared);
            if (!StringUtility::TryFromWString<bool>(
                attrValue, 
                this->IsShared))
            {
                Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
            }
        }

        if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsActivator))
        {
            auto attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsActivator);
            if (!StringUtility::TryFromWString<bool>(
                attrValue,
                this->IsActivator))
            {
                Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
            }
        }
    }

    if (xmlReader->IsEmptyElement())
    {
        xmlReader->ReadElement();
    }
    else
    {
        // <CodePackage ..>
        xmlReader->ReadStartElement();

        // SetupEntryPoint
        {
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_SetupEntryPoint,
                *SchemaNames::Namespace,
                false))
            {         
                xmlReader->ReadStartElement();

                this->SetupEntryPoint.ReadFromXml(xmlReader);

                xmlReader->ReadEndElement();

                this->HasSetupEntryPoint = true;
            }
        }

        // Main entrypoint
        {
            EntryPoint.ReadFromXml(xmlReader);
        }
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_EnvironmentVariables,
            *SchemaNames::Namespace))
        {
            EnvironmentVariables.ReadFromXml(xmlReader);
        }
   
        // </CodePackage>
        xmlReader->ReadEndElement();
    }
}

ErrorCode CodePackageDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    //<CodePackage>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_CodePackage, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Version, this->Version);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsShared, this->IsShared);
    if (!er.IsSuccess())
    {
        return er;
    }

    if (this->IsActivator)
    {
        er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsActivator, this->IsActivator);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (this->HasSetupEntryPoint)
    {
        er = WriteSetupEntryPoint(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    er = WriteEntryPoint(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    //</CodePackage>
    return xmlWriter->WriteEndElement();
}

ErrorCode CodePackageDescription::WriteSetupEntryPoint(XmlWriterUPtr const & xmlWriter)
{
    //<SetupEntryPoint>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_SetupEntryPoint, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = this->SetupEntryPoint.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    //</SetupEntryPoint>
    return xmlWriter->WriteEndElement();
}

ErrorCode CodePackageDescription::WriteEntryPoint(XmlWriterUPtr const & xmlWriter)
{
    //<EntryPoint>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_EntryPoint, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = this->EntryPoint.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    //</EntryPoint>
    return xmlWriter->WriteEndElement();
}

ErrorCode CodePackageDescription::FromPublicApi(FABRIC_CODE_PACKAGE_DESCRIPTION const & description)
{
    this->clear();

    this->Name = description.Name;
    this->Version = description.Version;
    this->IsShared = (description.IsShared == TRUE ? true : false);

    if (description.SetupEntryPoint != NULL)
    {
        auto error = this->SetupEntryPoint.FromPublicApi(*(description.SetupEntryPoint));
        if (!error.IsSuccess()) { return error; }

        this->HasSetupEntryPoint = true;
    }

    if (description.EntryPoint == NULL)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    auto error = EntryPoint.FromPublicApi(*(description.EntryPoint));
    if (!error.IsSuccess()) { return error; }

    return ErrorCode(ErrorCodeValue::Success);
}

void CodePackageDescription::ToPublicApi(
    __in ScopedHeap & heap, 
    __in std::wstring const& serviceManifestName, 
    __in wstring const& serviceManifestVersion, 
    __out FABRIC_CODE_PACKAGE_DESCRIPTION & publicDescription) const
{
    publicDescription.Name = heap.AddString(this->Name);
    publicDescription.Version = heap.AddString(this->Version);
    publicDescription.ServiceManifestName = heap.AddString(serviceManifestName);
    publicDescription.ServiceManifestVersion = heap.AddString(serviceManifestVersion);
    publicDescription.IsShared = (this->IsShared ? true : false);

    if (this->HasSetupEntryPoint)
    {
        auto setupEntryPoint = heap.AddItem<FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION>();
        this->SetupEntryPoint.ToPublicApi(heap, *setupEntryPoint);
        publicDescription.SetupEntryPoint = setupEntryPoint.GetRawPointer();
    }
    else
    {
        publicDescription.SetupEntryPoint = NULL;
    }

    auto entryPoint = heap.AddItem<FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION>();
    this->EntryPoint.ToPublicApi(heap, *entryPoint);
    publicDescription.EntryPoint = entryPoint.GetRawPointer();
}

void CodePackageDescription::clear()
{
    this->Name.clear();
    this->Version.clear();
    this->IsShared = false;
    this->SetupEntryPoint.clear();
    this->HasSetupEntryPoint = false;
    this->EntryPoint.clear();
    this->EnvironmentVariables.clear();
}
