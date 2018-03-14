// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

FileStoreETWDescription::FileStoreETWDescription() 
    : FileStoreDescription(),
    LevelFilter()
{
}

FileStoreETWDescription::FileStoreETWDescription(
    FileStoreETWDescription const & other)
    : FileStoreDescription((FileStoreDescription const &)other),
    LevelFilter(other.LevelFilter)
{
}

FileStoreETWDescription::FileStoreETWDescription(FileStoreETWDescription && other) 
    : FileStoreDescription((FileStoreDescription &&)other),
    LevelFilter(move(other.LevelFilter))
{
}

FileStoreETWDescription const & FileStoreETWDescription::operator = (FileStoreETWDescription const & other)
{
    if (this != &other)
    {
        FileStoreDescription::operator=((FileStoreDescription const &) other);
        this->LevelFilter = other.LevelFilter;
    }
    return *this;
}

FileStoreETWDescription const & FileStoreETWDescription::operator = (FileStoreETWDescription && other)
{
    if (this != &other)
    {
        FileStoreDescription::operator=((FileStoreDescription &&) other);
        this->LevelFilter = move(other.LevelFilter);
    }
    return *this;
}

bool FileStoreETWDescription::operator == (FileStoreETWDescription const & other) const
{
    bool equals = true;

    equals = ((FileStoreDescription) *this) == ((FileStoreDescription const &)other);
    if (!equals) { return equals; }

    equals = this->LevelFilter == other.LevelFilter;
    if (!equals) { return equals; }

    return equals;
}

bool FileStoreETWDescription::operator != (FileStoreETWDescription const & other) const
{
    return !(*this == other);
}

void FileStoreETWDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FileStoreETWDescription { ");

    w.Write((FileStoreDescription) *this);
    w.Write("LevelFilter = {0}", this->LevelFilter);

    w.Write("}");
}

void FileStoreETWDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_FileStore,
        *SchemaNames::Namespace,
        false);

    // <FileStore LevelFilter="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_LevelFilter))
    {
        this->LevelFilter = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_LevelFilter);
    }

    ReadFromXmlWorker(xmlReader);

    if (xmlReader->IsEmptyElement())
    {
        // <FileStore />
        xmlReader->ReadElement();
        return;
    }

    // </FileStore>
    xmlReader->ReadEndElement();
}

ErrorCode FileStoreETWDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_FileStore, L"", *SchemaNames::Namespace);
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

void FileStoreETWDescription::clear()
{
    FileStoreDescription::clear();
    this->LevelFilter.clear();
}
