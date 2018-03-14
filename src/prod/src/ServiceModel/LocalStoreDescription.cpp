// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

LocalStoreDescription::LocalStoreDescription() 
    : IsEnabled(),
    RelativeFolderPath(),
    DataDeletionAgeInDays(),
    Parameters()
{
}

LocalStoreDescription::LocalStoreDescription(
    LocalStoreDescription const & other)
    : IsEnabled(other.IsEnabled),
    RelativeFolderPath(other.RelativeFolderPath),
    DataDeletionAgeInDays(other.DataDeletionAgeInDays),
    Parameters(other.Parameters)
{
}

LocalStoreDescription::LocalStoreDescription(LocalStoreDescription && other) 
    : IsEnabled(move(other.IsEnabled)),
    RelativeFolderPath(move(other.RelativeFolderPath)),
    DataDeletionAgeInDays(move(other.DataDeletionAgeInDays)),
    Parameters(move(other.Parameters))
{
}

LocalStoreDescription const & LocalStoreDescription::operator = (LocalStoreDescription const & other)
{
    if (this != &other)
    {
        this->IsEnabled = other.IsEnabled;
        this->RelativeFolderPath = other.RelativeFolderPath;
        this->DataDeletionAgeInDays = other.DataDeletionAgeInDays;
        this->Parameters = other.Parameters;
    }
    return *this;
}

LocalStoreDescription const & LocalStoreDescription::operator = (LocalStoreDescription && other)
{
    if (this != &other)
    {
        this->IsEnabled = move(other.IsEnabled);
        this->RelativeFolderPath = move(other.RelativeFolderPath);
        this->DataDeletionAgeInDays = move(other.DataDeletionAgeInDays);
        this->Parameters = move(other.Parameters);

    }
    return *this;
}

bool LocalStoreDescription::operator == (LocalStoreDescription const & other) const
{
    bool equals = true;

    equals = this->IsEnabled == other.IsEnabled;
    if (!equals) { return equals; }

    equals = this->RelativeFolderPath == other.RelativeFolderPath;
    if (!equals) { return equals; }

    equals = this->DataDeletionAgeInDays == other.DataDeletionAgeInDays;
    if (!equals) { return equals; }

    equals = this->Parameters == other.Parameters;
    if (!equals) { return equals; }


    return equals;
}

bool LocalStoreDescription::operator != (LocalStoreDescription const & other) const
{
    return !(*this == other);
}

void LocalStoreDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FileStoreDescription { ");

    w.Write("IsEnabled = {0}", this->IsEnabled);
    w.Write("RelativeFolderPath = {0}", this->RelativeFolderPath);
    w.Write("DataDeletionAgeInDays = {0}", this->DataDeletionAgeInDays);

    w.Write("Parameters = {");
    w.Write(this->Parameters);
    w.Write("}");

    w.Write("}");
}

void LocalStoreDescription::ReadFromXmlWorker(
    XmlReaderUPtr const & xmlReader)
{
    // <LocalStore IsEnabled="" RelativeFolderPath="" DataDeletionAgeInDays="">
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
        // <LocalStore />
        return;
    }

    xmlReader->ReadStartElement();

    // <Parameters>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Parameters,
        *SchemaNames::Namespace))
    {
        this->Parameters.ReadFromXml(xmlReader);
    }
}

void LocalStoreDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_LocalStore,
        *SchemaNames::Namespace,
        false);

    ReadFromXmlWorker(xmlReader);

    if (xmlReader->IsEmptyElement())
    {
        // <LocalStore />
        xmlReader->ReadElement();
        return;
    }

    // </LocalStore>
    xmlReader->ReadEndElement();
}

ErrorCode LocalStoreDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<LocalStore>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_LocalStore, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteBaseToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->WriteEndElement();
}

ErrorCode LocalStoreDescription::WriteBaseToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_IsEnabled, this->IsEnabled);
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
	er = this->Parameters.WriteToXml(xmlWriter);
	return er;
}

void LocalStoreDescription::clear()
{
    this->IsEnabled.clear();
    this->RelativeFolderPath.clear();
    this->DataDeletionAgeInDays.clear();
    this->Parameters.clear();
}
