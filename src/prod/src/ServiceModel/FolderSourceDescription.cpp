// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

FolderSourceDescription::FolderSourceDescription() 
    : IsEnabled(),
    RelativeFolderPath(),
    DataDeletionAgeInDays(),
    Destinations(),
    Parameters()
{
}

FolderSourceDescription::FolderSourceDescription(
    FolderSourceDescription const & other)
    : IsEnabled(other.IsEnabled),
    RelativeFolderPath(other.RelativeFolderPath),
    DataDeletionAgeInDays(other.DataDeletionAgeInDays),
    Destinations(other.Destinations),
    Parameters(other.Parameters)
{
}

FolderSourceDescription::FolderSourceDescription(FolderSourceDescription && other) 
    : IsEnabled(move(other.IsEnabled)),
    RelativeFolderPath(move(other.RelativeFolderPath)),
    DataDeletionAgeInDays(move(other.DataDeletionAgeInDays)),
    Destinations(move(other.Destinations)),
    Parameters(move(other.Parameters))
{
}

FolderSourceDescription const & FolderSourceDescription::operator = (FolderSourceDescription const & other)
{
    if (this != &other)
    {
        this->IsEnabled = other.IsEnabled;
        this->RelativeFolderPath = other.RelativeFolderPath;
        this->DataDeletionAgeInDays = other.DataDeletionAgeInDays;
        this->Destinations = other.Destinations;
        this->Parameters = other.Parameters;
    }
    return *this;
}

FolderSourceDescription const & FolderSourceDescription::operator = (FolderSourceDescription && other)
{
    if (this != &other)
    {
        this->IsEnabled = move(other.IsEnabled);
        this->RelativeFolderPath = move(other.RelativeFolderPath);
        this->DataDeletionAgeInDays = move(other.DataDeletionAgeInDays);
        this->Destinations = move(other.Destinations);
        this->Parameters = move(other.Parameters);
    }
    return *this;
}

bool FolderSourceDescription::operator == (FolderSourceDescription const & other) const
{
    bool equals = true;

    equals = this->IsEnabled == other.IsEnabled;
    if (!equals) { return equals; }

    equals = this->RelativeFolderPath == other.RelativeFolderPath;
    if (!equals) { return equals; }

    equals = this->DataDeletionAgeInDays == other.DataDeletionAgeInDays;
    if (!equals) { return equals; }

    equals = this->Destinations == other.Destinations;
    if (!equals) { return equals; }

    equals = this->Parameters == other.Parameters;
    if (!equals) { return equals; }

    return equals;
}

bool FolderSourceDescription::operator != (FolderSourceDescription const & other) const
{
    return !(*this == other);
}

void FolderSourceDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FolderSourceDescription { ");

    w.Write("IsEnabled = {0}", this->IsEnabled);
    w.Write("RelativeFolderPath = {0}", this->RelativeFolderPath);
    w.Write("DataDeletionAgeInDays = {0}", this->DataDeletionAgeInDays);

    w.Write("Destinations = {");
    w.Write(this->Destinations);
    w.Write("}, ");

    w.Write("Parameters = {");
    w.Write(this->Parameters);
    w.Write("}");

    w.Write("}");
}

void FolderSourceDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_FolderSource,
        *SchemaNames::Namespace,
        false);

    // <FolderSource IsEnabled="" RelativeFolderPath="" DataDeletionAgeInDays="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsEnabled))
    {
        this->IsEnabled = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsEnabled);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_RelativeFolderPath))
    {
        this->RelativeFolderPath = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RelativeFolderPath);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DataDeletionAgeInDays))
    {
        this->DataDeletionAgeInDays = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DataDeletionAgeInDays);
    }

    if (xmlReader->IsEmptyElement())
    {
        // <FolderSource />
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

    // </FolderSource>
    xmlReader->ReadEndElement();
}

ErrorCode FolderSourceDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<FolderSourceDescrption>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_FolderSource, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_IsEnabled, this->IsEnabled);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RelativeFolderPath, this->RelativeFolderPath);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DataDeletionAgeInDays, this->DataDeletionAgeInDays);
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
	
	//</FolderSourceDescription>
	return xmlWriter->WriteEndElement();
}

void FolderSourceDescription::clear()
{
    this->IsEnabled.clear();
    this->RelativeFolderPath.clear();
    this->DataDeletionAgeInDays.clear();
    this->Destinations.clear();
    this->Parameters.clear();
}
