// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ETWSourceDescription::ETWSourceDescription() 
    : IsEnabled(),
    Destinations(),
    Parameters()
{
}

ETWSourceDescription::ETWSourceDescription(
    ETWSourceDescription const & other)
    : IsEnabled(other.IsEnabled),
    Destinations(other.Destinations),
    Parameters(other.Parameters)
{
}

ETWSourceDescription::ETWSourceDescription(ETWSourceDescription && other) 
    : IsEnabled(move(other.IsEnabled)),
    Destinations(move(other.Destinations)),
    Parameters(move(other.Parameters))
{
}

ETWSourceDescription const & ETWSourceDescription::operator = (ETWSourceDescription const & other)
{
    if (this != &other)
    {
        this->IsEnabled = other.IsEnabled;
        this->Destinations = other.Destinations;
        this->Parameters = other.Parameters;
    }
    return *this;
}

ETWSourceDescription const & ETWSourceDescription::operator = (ETWSourceDescription && other)
{
    if (this != &other)
    {
        this->IsEnabled = move(other.IsEnabled);
        this->Destinations = move(other.Destinations);
        this->Parameters = move(other.Parameters);
    }
    return *this;
}

bool ETWSourceDescription::operator == (ETWSourceDescription const & other) const
{
    bool equals = true;

    equals = this->IsEnabled == other.IsEnabled;
    if (!equals) { return equals; }

    equals = this->Destinations == other.Destinations;
    if (!equals) { return equals; }

    equals = this->Parameters == other.Parameters;
    if (!equals) { return equals; }

    return equals;
}

bool ETWSourceDescription::operator != (ETWSourceDescription const & other) const
{
    return !(*this == other);
}

void ETWSourceDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ETWSourceDescription { ");

    w.Write("IsEnabled = {0}", this->IsEnabled);

    w.Write("Destinations = {");
    w.Write(this->Destinations);
    w.Write("}, ");

    w.Write("Parameters = {");
    w.Write(this->Parameters);
    w.Write("}");

    w.Write("}");
}

void ETWSourceDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_ETWSource,
        *SchemaNames::Namespace,
        false);

    // <ETWSource IsEnabled="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsEnabled))
    {
        this->IsEnabled = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsEnabled);
    }

    if (xmlReader->IsEmptyElement())
    {
        // <ETWSource />
        xmlReader->ReadElement();
        return;
    }

    xmlReader->ReadStartElement();

    // <Destinations>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Destinations,
        *SchemaNames::Namespace))
    {
        this->Destinations.ReadFromXml(xmlReader);
    }

    // <Parameters>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Parameters,
        *SchemaNames::Namespace))
    {
        this->Parameters.ReadFromXml(xmlReader);
    }

    // </ETWSource>
    xmlReader->ReadEndElement();
}


ErrorCode ETWSourceDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<ETWSource>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ETWSource, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_IsEnabled, this->IsEnabled);
	if (!er.IsSuccess())
	{
		return er;
	}
	if (!Destinations.empty())
	{
		er = this->Destinations.WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	if (!Parameters.empty())
	{
		er = this->Parameters.WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	
	//</ETWSource>
	return xmlWriter->WriteEndElement();
}
void ETWSourceDescription::clear()
{
    this->IsEnabled.clear();
    this->Destinations.clear();
    this->Parameters.clear();
}
