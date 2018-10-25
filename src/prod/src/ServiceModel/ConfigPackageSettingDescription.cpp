// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ConfigPackageSettingDescription::ConfigPackageSettingDescription() :
    Name(),
    SectionName(),
    MountPoint(L""),
    EnvironmentVariableName(L"")
{
}

bool ConfigPackageSettingDescription::operator == (ConfigPackageSettingDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->SectionName, other.SectionName);
    if (!equals) { return equals; }


    equals = StringUtility::AreEqualCaseInsensitive(this->MountPoint, other.MountPoint);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->EnvironmentVariableName, other.EnvironmentVariableName);
    if (!equals) { return equals; }

    return equals;
}

bool ConfigPackageSettingDescription::operator != (ConfigPackageSettingDescription const & other) const
{
    return !(*this == other);
}

void ConfigPackageSettingDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigPackageSettingDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("SectionName = {0}", SectionName);
    w.Write("MountPoint = {0}", MountPoint);
    w.Write("EnvironmentVariableName = {0}", EnvironmentVariableName);
    w.Write("}");
}

void ConfigPackageSettingDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    // ensure that we are on <ConfigPackage
    xmlReader->StartElement(
        *SchemaNames::Element_ConfigPackage,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->SectionName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_SectionName);
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MountPoint))
    {
        this->MountPoint = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_MountPoint);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_EnvironmentVariableName))
    {
        this->EnvironmentVariableName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_EnvironmentVariableName);
    }
    xmlReader->ReadElement();
}

ErrorCode ConfigPackageSettingDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
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

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_SectionName, this->SectionName);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_MountPoint, this->MountPoint);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_EnvironmentVariableName, this->EnvironmentVariableName);
    if (!er.IsSuccess())
    {
        return er;
    }

    return xmlWriter->WriteEndElement();
}

void ConfigPackageSettingDescription::clear()
{
    this->Name.clear();
    this->SectionName.clear();
    this->MountPoint.clear();
    this->EnvironmentVariableName.clear();
}
