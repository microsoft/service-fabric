// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DataPackageDescription::DataPackageDescription() :
    Name(),
    Version()
{
}

DataPackageDescription::DataPackageDescription(DataPackageDescription const & other) :
    Name(other.Name),
    Version(other.Version)
{
}

DataPackageDescription::DataPackageDescription(DataPackageDescription && other) :
    Name(move(other.Name)),
    Version(move(other.Version))
{
}

DataPackageDescription const & DataPackageDescription::operator = (DataPackageDescription const & other)
{
    if (this != & other)
    {
        this->Name = other.Name;
        this->Version = other.Version;
    }

    return *this;
}

DataPackageDescription const & DataPackageDescription::operator = (DataPackageDescription && other)
{
    if (this != & other)
    {
        this->Name = move(other.Name);
        this->Version = move(other.Version);
    }

    return *this;
}

bool DataPackageDescription::operator == (DataPackageDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Version, other.Version);
    if (!equals) { return equals; }

    return equals;
}

bool DataPackageDescription::operator != (DataPackageDescription const & other) const
{
    return !(*this == other);
}

void DataPackageDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DataPackageDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Version = {0}", Version);
    w.Write("}");
}

void DataPackageDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    // ensure that we are on <DataPackage
    xmlReader->StartElement(
        *SchemaNames::Element_DataPackage,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->Version = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Version);

    xmlReader->ReadElement();
}

ErrorCode DataPackageDescription::WriteToXml(XmlWriterUPtr const & xmlWriter, bool isDiagnostic)
{
	ErrorCode er = xmlWriter->WriteStartElement((isDiagnostic ? *SchemaNames::Element_ManifestDataPackage : *SchemaNames::Element_DataPackage), 
		L"", *SchemaNames::Namespace);
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
	return xmlWriter->WriteEndElement();
}

ErrorCode DataPackageDescription::FromPublicApi(FABRIC_DATA_PACKAGE_DESCRIPTION const & publicDescription)
{
    this->Name = publicDescription.Name;
    this->Version = publicDescription.Version;
    return ErrorCode(ErrorCodeValue::Success);
}

void DataPackageDescription::ToPublicApi(
    __in ScopedHeap & heap, 
    __in std::wstring const& serviceManifestName, 
    __in wstring const& serviceManifestVersion, 
    __out FABRIC_DATA_PACKAGE_DESCRIPTION & publicDescription) const
{
    publicDescription.Name = heap.AddString(this->Name);
    publicDescription.Version = heap.AddString(this->Version);
    publicDescription.ServiceManifestName = heap.AddString(serviceManifestName);
    publicDescription.ServiceManifestVersion = heap.AddString(serviceManifestVersion);
}

void DataPackageDescription::clear()
{
    this->Name.clear();
    this->Version.clear();
}
