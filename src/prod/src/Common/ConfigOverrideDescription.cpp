// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ConfigOverrideDescription::ConfigOverrideDescription()
    : ConfigPackageName(),
    SettingsOverride()
{
}

ConfigOverrideDescription::ConfigOverrideDescription(ConfigOverrideDescription const & other)
    : ConfigPackageName(other.ConfigPackageName),
    SettingsOverride(other.SettingsOverride)
{
}

ConfigOverrideDescription::ConfigOverrideDescription(ConfigOverrideDescription && other)
    : ConfigPackageName(move(other.ConfigPackageName)),
    SettingsOverride(move(other.SettingsOverride))
{
}

ConfigOverrideDescription const & ConfigOverrideDescription::operator = (ConfigOverrideDescription const & other)
{
    if (this != &other)
    {
        this->ConfigPackageName = other.ConfigPackageName;
        this->SettingsOverride = other.SettingsOverride;
    }

    return *this;
}

ConfigOverrideDescription const & ConfigOverrideDescription::operator = (ConfigOverrideDescription && other)
{
    if (this != &other)
    {
        this->ConfigPackageName = move(other.ConfigPackageName);
        this->SettingsOverride = move(other.SettingsOverride);
    }

    return *this;
}

bool ConfigOverrideDescription::operator == (ConfigOverrideDescription const & other) const
{
    bool equals = StringUtility::AreEqualCaseInsensitive(this->ConfigPackageName, other.ConfigPackageName);
    if (!equals) { return equals; }

    equals = this->SettingsOverride == other.SettingsOverride;
    return equals;
}

bool ConfigOverrideDescription::operator != (ConfigOverrideDescription const & other) const
{
    return !(*this == other);
}

void ConfigOverrideDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigOverrideDescription {");
    w.Write("ConfigPackageName = {0},", ConfigPackageName);
    w.Write("Settings = {0}", SettingsOverride);
    w.Write("}");
}

void ConfigOverrideDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are positioned on <ConfigOverride 
    xmlReader->StartElement(
        *SchemaNames::Element_ConfigOverride, 
        *SchemaNames::Namespace);

    // ConfigOverride Name=""
    this->ConfigPackageName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    if (xmlReader->IsEmptyElement())
    {
        // <ConfigOverride ... />
        xmlReader->ReadElement();
    }
    else
    {
        // <ConfigOverride ...>
        xmlReader->ReadStartElement();

        // <Settings ... >
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ConfigSettings,
            *SchemaNames::Namespace,
            false))
        {
            this->SettingsOverride.ReadFromXml(xmlReader);
        }

        xmlReader->SkipElement(
            *SchemaNames::Element_Environment, 
            *SchemaNames::Namespace, 
            true);

        // </ConfigOverride>
        xmlReader->ReadEndElement();
    }
}


ErrorCode ConfigOverrideDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<ConfigOverride>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigOverride, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->ConfigPackageName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = this->SettingsOverride.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</ConfigOverride>
	return xmlWriter->WriteEndElement();
}

void ConfigOverrideDescription::clear()
{
    this->ConfigPackageName.clear();
    this->SettingsOverride.clear();
}
