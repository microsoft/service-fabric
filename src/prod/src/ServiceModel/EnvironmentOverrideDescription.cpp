// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EnvironmentOverridesDescription::EnvironmentOverridesDescription()
    : EnvironmentVariables(),
    CodePackageRef()
{
}

bool EnvironmentOverridesDescription::operator == (EnvironmentOverridesDescription const & other) const
{
    bool equals = true;
    for (auto i = 0; i < EnvironmentVariables.size(); i++)
    {
        equals = EnvironmentVariables[i] == other.EnvironmentVariables[i];
        if (!equals) { return equals; }
    }
    this->CodePackageRef == other.CodePackageRef;
    if (!equals) { return equals; }
    return equals;
}

bool EnvironmentOverridesDescription::operator != (EnvironmentOverridesDescription const & other) const
{
    return !(*this == other);
}

void EnvironmentOverridesDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are positioned on <EnvironmentVariables 
    xmlReader->StartElement(
        *SchemaNames::Element_EnvironmentOverrides, 
        *SchemaNames::Namespace);

    // EnvironmentOverrides CodePackageRef=""
    this->CodePackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CodePackageRef);

    if (xmlReader->IsEmptyElement())
    {
        // <EnvironmentOverrides ... />
        xmlReader->ReadElement();
    }
    else
    {
        // <EnvironmentOverrides ...>
        xmlReader->ReadStartElement();
        bool hasEnvVariables = true;
        while (hasEnvVariables)
        {
            // <EnvironmentVariable ... >
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_EnvironmentVariable,
                *SchemaNames::Namespace,
                false))
            {
                EnvironmentVariableOverrideDescription description;
                description.ReadFromXml(xmlReader);
                EnvironmentVariables.push_back(description);
            }
            else
            {
                hasEnvVariables = false;
            }
        }
        // </EnvironmentOverrides>
        xmlReader->ReadEndElement();
    }
}


ErrorCode EnvironmentOverridesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    //<EnvironmentOverrides>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_EnvironmentOverrides, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CodePackageRef, this->CodePackageRef);
    if (!er.IsSuccess())
    {
        return er;
    }
    for (auto i = 0; i < EnvironmentVariables.size(); i++)
    {
        er = EnvironmentVariables[i].WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    //</EnvironmentOverrides>
    return xmlWriter->WriteEndElement();
}

void EnvironmentOverridesDescription::clear()
{
    this->CodePackageRef.clear();
    this->EnvironmentVariables.clear();
}
