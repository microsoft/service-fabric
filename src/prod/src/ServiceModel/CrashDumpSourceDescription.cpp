// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

CrashDumpSourceDescription::CrashDumpSourceDescription() 
    : IsEnabled(),
    Destinations(),
    Parameters()
{
}

CrashDumpSourceDescription::CrashDumpSourceDescription(
    CrashDumpSourceDescription const & other)
    : IsEnabled(other.IsEnabled),
    Destinations(other.Destinations),
    Parameters(other.Parameters)
{
}

CrashDumpSourceDescription::CrashDumpSourceDescription(CrashDumpSourceDescription && other) 
    : IsEnabled(move(other.IsEnabled)),
    Destinations(move(other.Destinations)),
    Parameters(move(other.Parameters))
{
}

CrashDumpSourceDescription const & CrashDumpSourceDescription::operator = (CrashDumpSourceDescription const & other)
{
    if (this != &other)
    {
        this->IsEnabled = other.IsEnabled;
        this->Destinations = other.Destinations;
        this->Parameters = other.Parameters;
    }
    return *this;
}

CrashDumpSourceDescription const & CrashDumpSourceDescription::operator = (CrashDumpSourceDescription && other)
{
    if (this != &other)
    {
        this->IsEnabled = move(other.IsEnabled);
        this->Destinations = move(other.Destinations);
        this->Parameters = move(other.Parameters);
    }
    return *this;
}

bool CrashDumpSourceDescription::operator == (CrashDumpSourceDescription const & other) const
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

bool CrashDumpSourceDescription::operator != (CrashDumpSourceDescription const & other) const
{
    return !(*this == other);
}

void CrashDumpSourceDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CrashDumpSourceDescription { ");

    w.Write("IsEnabled = {0}", this->IsEnabled);

    w.Write("Destinations = {");
    w.Write(this->Destinations);
    w.Write("}, ");

    w.Write("Parameters = {");
    w.Write(this->Parameters);
    w.Write("}");

    w.Write("}");
}

void CrashDumpSourceDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_CrashDumpSource,
        *SchemaNames::Namespace,
        false);

    // <CrashDumpSource IsEnabled="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsEnabled))
    {
        this->IsEnabled = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsEnabled);
    }

    if (xmlReader->IsEmptyElement())
    {
        // <CrashDumpSource />
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

    // </CrashDumpSource>
    xmlReader->ReadEndElement();
}

ErrorCode CrashDumpSourceDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<CrashDumpSource>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_CrashDumpSource, L"", *SchemaNames::Namespace);
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
		er = Destinations.WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}

	}
	if (!Parameters.empty())
	{
		er = Parameters.WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	
	//</CrashDumpSource>
	return xmlWriter->WriteEndElement();
}

void CrashDumpSourceDescription::clear()
{
    this->IsEnabled.clear();
    this->Destinations.clear();
    this->Parameters.clear();
}
