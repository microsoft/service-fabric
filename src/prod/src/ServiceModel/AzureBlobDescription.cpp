// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

AzureBlobDescription::AzureBlobDescription() 
    : AzureStoreBaseDescription(),
    ContainerName()
{
}

AzureBlobDescription::AzureBlobDescription(
    AzureBlobDescription const & other)
    : AzureStoreBaseDescription((AzureStoreBaseDescription const &)other),
    ContainerName(other.ContainerName)
{
}

AzureBlobDescription::AzureBlobDescription(AzureBlobDescription && other) 
    : AzureStoreBaseDescription((AzureStoreBaseDescription &&)other),
    ContainerName(move(other.ContainerName))
{
}

AzureBlobDescription const & AzureBlobDescription::operator = (AzureBlobDescription const & other)
{
    if (this != &other)
    {
        AzureStoreBaseDescription::operator=((AzureStoreBaseDescription const &) other);
        this->ContainerName = other.ContainerName;
    }
    return *this;
}

AzureBlobDescription const & AzureBlobDescription::operator = (AzureBlobDescription && other)
{
    if (this != &other)
    {
        AzureStoreBaseDescription::operator=((AzureStoreBaseDescription &&) other);
        this->ContainerName = move(other.ContainerName);
    }
    return *this;
}

bool AzureBlobDescription::operator == (AzureBlobDescription const & other) const
{
    bool equals = true;

    equals = ((AzureStoreBaseDescription) *this) == ((AzureStoreBaseDescription const &)other);
    if (!equals) { return equals; }

    equals = this->ContainerName == other.ContainerName;
    if (!equals) { return equals; }

    return equals;
}

bool AzureBlobDescription::operator != (AzureBlobDescription const & other) const
{
    return !(*this == other);
}

void AzureBlobDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("AzureBlobDescription { ");

    w.Write((AzureStoreBaseDescription) *this);
    w.Write("ContainerName = {0}", this->ContainerName);

    w.Write("}");
}

void AzureBlobDescription::ReadFromXmlWorker(
    XmlReaderUPtr const & xmlReader)
{
    // <AzureBlob ContainerName="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ContainerName))
    {
        this->ContainerName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ContainerName);
    }

    AzureStoreBaseDescription::ReadFromXmlWorker(xmlReader);
}

void AzureBlobDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_AzureBlob,
        *SchemaNames::Namespace,
        false);

    ReadFromXmlWorker(xmlReader);

    if (xmlReader->IsEmptyElement())
    {
        // <AzureBlob />
        xmlReader->ReadElement();
        return;
    }

    // </AzureBlob>
    xmlReader->ReadEndElement();
}

ErrorCode AzureBlobDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<AzureBlob>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_AzureBlob, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteBaseToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</AzureBlob>
	return xmlWriter->WriteEndElement();
}

ErrorCode AzureBlobDescription::WriteBaseToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ContainerName, this->ContainerName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteXmlWorker(xmlWriter);
	return er;
}

void AzureBlobDescription::clear()
{
    AzureStoreBaseDescription::clear();
    this->ContainerName.clear();
}
