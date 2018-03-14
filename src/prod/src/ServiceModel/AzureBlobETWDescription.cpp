// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

AzureBlobETWDescription::AzureBlobETWDescription() 
    : AzureBlobDescription(),
    LevelFilter()
{
}

AzureBlobETWDescription::AzureBlobETWDescription(
    AzureBlobETWDescription const & other)
    : AzureBlobDescription((AzureBlobDescription const &)other),
    LevelFilter(other.LevelFilter)
{
}

AzureBlobETWDescription::AzureBlobETWDescription(AzureBlobETWDescription && other) 
    : AzureBlobDescription((AzureBlobDescription &&)other),
    LevelFilter(move(other.LevelFilter))
{
}

AzureBlobETWDescription const & AzureBlobETWDescription::operator = (AzureBlobETWDescription const & other)
{
    if (this != &other)
    {
        AzureBlobDescription::operator=((AzureBlobDescription const &) other);
        this->LevelFilter = other.LevelFilter;
    }
    return *this;
}

AzureBlobETWDescription const & AzureBlobETWDescription::operator = (AzureBlobETWDescription && other)
{
    if (this != &other)
    {
        AzureBlobDescription::operator=((AzureBlobDescription &&) other);
        this->LevelFilter = move(other.LevelFilter);
    }
    return *this;
}

bool AzureBlobETWDescription::operator == (AzureBlobETWDescription const & other) const
{
    bool equals = true;

    equals = ((AzureBlobDescription) *this) == ((AzureBlobDescription const &)other);
    if (!equals) { return equals; }

    equals = this->LevelFilter == other.LevelFilter;
    if (!equals) { return equals; }

    return equals;
}

bool AzureBlobETWDescription::operator != (AzureBlobETWDescription const & other) const
{
    return !(*this == other);
}

void AzureBlobETWDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("AzureBlobETWDescription { ");

    w.Write((AzureBlobDescription) *this);
    w.Write("LevelFilter = {0}", this->LevelFilter);

    w.Write("}");
}

void AzureBlobETWDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_AzureBlob,
        *SchemaNames::Namespace,
        false);

    // <AzureBlob LevelFilter="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_LevelFilter))
    {
        this->LevelFilter = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_LevelFilter);
    }

    AzureBlobDescription::ReadFromXmlWorker(xmlReader);

    if (xmlReader->IsEmptyElement())
    {
        // <AzureBlob />
        xmlReader->ReadElement();
        return;
    }

    // </AzureBlob>
    xmlReader->ReadEndElement();
}

ErrorCode AzureBlobETWDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_AzureBlob, L"", *SchemaNames::Namespace);
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
	return xmlWriter->WriteEndElement();
}

void AzureBlobETWDescription::clear()
{
    AzureBlobDescription::clear();
    this->LevelFilter.clear();
}
