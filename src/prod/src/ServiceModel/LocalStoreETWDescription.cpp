// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

LocalStoreETWDescription::LocalStoreETWDescription() 
    : LocalStoreDescription(),
    LevelFilter()
{
}

LocalStoreETWDescription::LocalStoreETWDescription(
    LocalStoreETWDescription const & other)
    : LocalStoreDescription((LocalStoreDescription const &)other),
    LevelFilter(other.LevelFilter)
{
}

LocalStoreETWDescription::LocalStoreETWDescription(LocalStoreETWDescription && other) 
    : LocalStoreDescription((LocalStoreDescription &&)other),
    LevelFilter(move(other.LevelFilter))
{
}

LocalStoreETWDescription const & LocalStoreETWDescription::operator = (LocalStoreETWDescription const & other)
{
    if (this != &other)
    {
        LocalStoreDescription::operator=((LocalStoreDescription const &) other);
        this->LevelFilter = other.LevelFilter;
    }
    return *this;
}

LocalStoreETWDescription const & LocalStoreETWDescription::operator = (LocalStoreETWDescription && other)
{
    if (this != &other)
    {
        LocalStoreDescription::operator=((LocalStoreDescription &&) other);
        this->LevelFilter = move(other.LevelFilter);
    }
    return *this;
}

bool LocalStoreETWDescription::operator == (LocalStoreETWDescription const & other) const
{
    bool equals = true;

    equals = ((LocalStoreDescription) *this) == ((LocalStoreDescription const &)other);
    if (!equals) { return equals; }

    equals = this->LevelFilter == other.LevelFilter;
    if (!equals) { return equals; }

    return equals;
}

bool LocalStoreETWDescription::operator != (LocalStoreETWDescription const & other) const
{
    return !(*this == other);
}

void LocalStoreETWDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("LocalStoreETWDescription { ");

    w.Write((LocalStoreDescription) *this);
    w.Write("LevelFilter = {0}", this->LevelFilter);

    w.Write("}");
}

void LocalStoreETWDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_LocalStore,
        *SchemaNames::Namespace,
        false);

    // <LocalStore LevelFilter="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_LevelFilter))
    {
        this->LevelFilter = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_LevelFilter);
    }

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

ErrorCode LocalStoreETWDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<LocalStore>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_LocalStore, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_LevelFilter, this->LevelFilter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = this->WriteBaseToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</LocalStore>
	return xmlWriter->WriteEndElement();
}

void LocalStoreETWDescription::clear()
{
    LocalStoreDescription::clear();
    this->LevelFilter.clear();
}
