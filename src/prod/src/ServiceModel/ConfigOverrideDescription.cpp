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
            this->SettingsOverride = ReadSettingsOverride(*xmlReader);
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

    er = WriteSettingsOverride(SettingsOverride, xmlWriter);
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

ConfigSettingsOverride ConfigOverrideDescription::ReadSettingsOverride(XmlReader & xmlReader)
{
    xmlReader.StartElement(
        *SchemaNames::Element_ConfigSettings,
        *SchemaNames::Namespace);

    auto sectionMap = xmlReader.ReadMapWithInlineName<ConfigSettingsOverride::SectionMapType>(
        *SchemaNames::Element_ConfigSection,
        *SchemaNames::Namespace,
        ReadSectionOverride);

    return ConfigSettingsOverride(move(sectionMap));
}

ConfigSectionOverride ConfigOverrideDescription::ReadSectionOverride(XmlReader &  xmlReader)
{
    xmlReader.StartElement(
        *SchemaNames::Element_ConfigSection,
        *SchemaNames::Namespace);

    auto name = xmlReader.ReadAttributeValue(*SchemaNames::Attribute_Name);

    auto parametersMap = xmlReader.ReadMapWithInlineName<ConfigSectionOverride::ParametersMapType>(
        *SchemaNames::Element_ConfigParameter,
        *SchemaNames::Namespace,
        ReadParameterOverride);
        
    return ConfigSectionOverride(move(name), move(parametersMap));
}

ConfigParameterOverride ConfigOverrideDescription::ReadParameterOverride(XmlReader & xmlReader)
{
    xmlReader.StartElement(
        *SchemaNames::Element_ConfigParameter,
        *SchemaNames::Namespace);

    auto name = xmlReader.ReadAttributeValue(*SchemaNames::Attribute_Name);
    auto value = xmlReader.ReadAttributeValue(*SchemaNames::Attribute_Value);

    bool isEncrypted = false;
    if (xmlReader.HasAttribute(*SchemaNames::Attribute_IsEncrypted))
    {
        isEncrypted = xmlReader.ReadAttributeValueAs<bool>(*SchemaNames::Attribute_IsEncrypted);
    }

    wstring type;
    if (xmlReader.HasAttribute(*SchemaNames::Attribute_Type))
    {
        type = xmlReader.ReadAttributeValue(*SchemaNames::Attribute_Type);
    }

    xmlReader.ReadElement();

    return ConfigParameterOverride(move(name), move(value), isEncrypted, move(type));
}

ErrorCode ConfigOverrideDescription::WriteSettingsOverride(ConfigSettingsOverride const & settings, XmlWriterUPtr const & xmlWriter)
{
    if (settings.Sections.empty())
    {
        //Nothing to do, return success
        return ErrorCodeValue::Success;
    }

    //<Settings>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigSettings, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    for (auto it = settings.Sections.begin(); it != settings.Sections.end(); ++it)
    {
        er = WriteSectionOverride(it->second, xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    //</Settings>
    return xmlWriter->WriteEndElement();
}

ErrorCode ConfigOverrideDescription::WriteSectionOverride(ConfigSectionOverride const & section, XmlWriterUPtr const & xmlWriter)
{
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigSection, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, section.Name);
    if (!er.IsSuccess())
    {
        return er;
    }

    for (auto it = section.Parameters.begin(); it != section.Parameters.end(); ++it)
    {
        er = WriteParameterOverride(it->second, xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    //</Section>
    return xmlWriter->WriteEndElement();
}

ErrorCode ConfigOverrideDescription::WriteParameterOverride(ConfigParameterOverride const & parameter, XmlWriterUPtr const & xmlWriter)
{
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigParameter, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, parameter.Name);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Value, parameter.Value);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsEncrypted, parameter.IsEncrypted);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Type, parameter.Type);
    if (!er.IsSuccess())
    {
        return er;
    }
    //</Paramter>
    return xmlWriter->WriteEndElement();
}
