// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ConfigPackageDescription::ConfigPackageDescription() :
    Name(),
    Version()
{
}

ConfigPackageDescription::ConfigPackageDescription(ConfigPackageDescription const & other) :
    Name(other.Name),
    Version(other.Version)
{
}

ConfigPackageDescription::ConfigPackageDescription(ConfigPackageDescription && other) :
    Name(move(other.Name)),
    Version(move(other.Version))
{
}

ConfigPackageDescription const & ConfigPackageDescription::operator = (ConfigPackageDescription const & other)
{
    if (this != & other)
    {
        this->Name = other.Name;
        this->Version = other.Version;
    }

    return *this;
}

ConfigPackageDescription const & ConfigPackageDescription::operator = (ConfigPackageDescription && other)
{
    if (this != & other)
    {
        this->Name = move(other.Name);
        this->Version = move(other.Version);
    }

    return *this;
}

bool ConfigPackageDescription::operator == (ConfigPackageDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Version, other.Version);
    if (!equals) { return equals; }

    return equals;
}

bool ConfigPackageDescription::operator != (ConfigPackageDescription const & other) const
{
    return !(*this == other);
}

void ConfigPackageDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigPackageDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Version = {0}", Version);
    w.Write("}");
}

void ConfigPackageDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    // ensure that we are on <ConfigPackage
    xmlReader->StartElement(
        *SchemaNames::Element_ConfigPackage,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->Version = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Version);

    xmlReader->ReadElement();
}

ErrorCode ConfigPackageDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigPackage, L"", *SchemaNames::Namespace);
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

ErrorCode ConfigPackageDescription::FromPublicApi(FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION const & publicDescription)
{
    this->Name = publicDescription.Name;
    this->Version = publicDescription.Version;
    return ErrorCode(ErrorCodeValue::Success);
}

void ConfigPackageDescription::ToPublicApi(
    __in ScopedHeap & heap, 
    __in std::wstring const& serviceManifestName, 
    __in wstring const& serviceManifestVersion, 
    __out FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION & publicDescription) const
{
    publicDescription.Name = heap.AddString(this->Name);
    publicDescription.Version = heap.AddString(this->Version);
    publicDescription.ServiceManifestName = heap.AddString(serviceManifestName);
    publicDescription.ServiceManifestVersion = heap.AddString(serviceManifestVersion);
}

void ConfigPackageDescription::clear()
{
    this->Name.clear();
    this->Version.clear();
}
