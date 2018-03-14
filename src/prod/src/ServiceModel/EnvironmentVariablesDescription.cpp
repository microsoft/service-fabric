// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EnvironmentVariablesDescription::EnvironmentVariablesDescription()
    : EnvironmentVariables()
{
}

EnvironmentVariablesDescription::EnvironmentVariablesDescription(EnvironmentVariablesDescription const & other)
    : EnvironmentVariables(other.EnvironmentVariables)
{
}

EnvironmentVariablesDescription::EnvironmentVariablesDescription(EnvironmentVariablesDescription && other)
    : EnvironmentVariables(move(other.EnvironmentVariables))
{
}

EnvironmentVariablesDescription const & EnvironmentVariablesDescription::operator = (EnvironmentVariablesDescription const & other)
{
    if (this != &other)
    {
        this->EnvironmentVariables = other.EnvironmentVariables;
    }

    return *this;
}

EnvironmentVariablesDescription const & EnvironmentVariablesDescription::operator = (EnvironmentVariablesDescription && other)
{
    if (this != &other)
    {
        this->EnvironmentVariables = move(other.EnvironmentVariables);
    }

    return *this;
}

bool EnvironmentVariablesDescription::operator == (EnvironmentVariablesDescription const & other) const
{
    bool equals = true;
    for (auto i = 0; i < EnvironmentVariables.size(); i++)
    {
        equals = EnvironmentVariables[i] == other.EnvironmentVariables[i];
        if (!equals) { return equals; }
    }
    return equals;
}

bool EnvironmentVariablesDescription::operator != (EnvironmentVariablesDescription const & other) const
{
    return !(*this == other);
}

void EnvironmentVariablesDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are positioned on <EnvironmentVariables 
    xmlReader->StartElement(
        *SchemaNames::Element_EnvironmentVariables, 
        *SchemaNames::Namespace);


    if (xmlReader->IsEmptyElement())
    {
        // <EnvironmentVariables ... />
        xmlReader->ReadElement();
    }
    else
    {
        // <EnvironmentVariables ...>
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
                EnvironmentVariableDescription description;
                description.ReadFromXml(xmlReader);
                EnvironmentVariables.push_back(description);
            }
            else
            {
                hasEnvVariables = false;
            }
        }
        // </EnvironmentVariables>
        xmlReader->ReadEndElement();
    }
}


ErrorCode EnvironmentVariablesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<EnvironmentVariables>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_EnvironmentVariables, L"", *SchemaNames::Namespace);
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
	//</EnvironmentVariables>
	return xmlWriter->WriteEndElement();
}

void EnvironmentVariablesDescription::clear()
{
    this->EnvironmentVariables.clear();
}
