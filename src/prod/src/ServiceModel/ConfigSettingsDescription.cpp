//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void ConfigSettingsDescription::ReadFromXml(Common::XmlReaderUPtr const & xmlReader)
{
    Settings = ReadSettings(*xmlReader);
}

ConfigSettings ConfigSettingsDescription::ReadSettings(XmlReader & xmlReader)
{
    xmlReader.StartElement(
        *SchemaNames::Element_ConfigSettings,
        *SchemaNames::Namespace);

    auto sectionMap = xmlReader.ReadMapWithInlineName<ConfigSettings::SectionMapType>(
        *SchemaNames::Element_ConfigSection,
        *SchemaNames::Namespace,
        ReadSection);
        
    return ConfigSettings(move(sectionMap));
}

ConfigSection ConfigSettingsDescription::ReadSection(XmlReader & xmlReader)
{
    xmlReader.StartElement(
        *SchemaNames::Element_ConfigSection,
        *SchemaNames::Namespace);

    // Section Name=""
    auto name = xmlReader.ReadAttributeValue(*SchemaNames::Attribute_Name);

    auto parameterMap = xmlReader.ReadMapWithInlineName<ConfigSection::ParametersMapType>(
        *SchemaNames::Element_ConfigParameter,
        *SchemaNames::Namespace,
        ReadParameter);
    
    return ConfigSection(move(name), move(parameterMap));
}

ConfigParameter ConfigSettingsDescription::ReadParameter(Common::XmlReader & xmlReader)
{
    // ensure that we are positioned on ConfigParameter
    xmlReader.StartElement(
        *SchemaNames::Element_ConfigParameter,
        *SchemaNames::Namespace);

    auto name = xmlReader.ReadAttributeValue(*SchemaNames::Attribute_Name);
    auto value = xmlReader.ReadAttributeValue(*SchemaNames::Attribute_Value);
    auto mustOverride = false;
    if (xmlReader.HasAttribute(*SchemaNames::Attribute_MustOverride))
    {
        mustOverride = xmlReader.ReadAttributeValueAs<bool>(*SchemaNames::Attribute_MustOverride);
    }

    auto isEncrypted = false;
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

    return ConfigParameter(
        move(name),
        move(value),
        mustOverride,
        isEncrypted,
        move(type));
}