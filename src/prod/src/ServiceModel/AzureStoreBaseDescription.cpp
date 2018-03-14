// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

AzureStoreBaseDescription::AzureStoreBaseDescription() 
    : IsEnabled(),
    ConnectionString(),
    ConnectionStringIsEncrypted(),
    UploadIntervalInMinutes(),
    DataDeletionAgeInDays(),
    Parameters()
{
}

AzureStoreBaseDescription::AzureStoreBaseDescription(
    AzureStoreBaseDescription const & other)
    : IsEnabled(other.IsEnabled),
    ConnectionString(other.ConnectionString),
    ConnectionStringIsEncrypted(other.ConnectionStringIsEncrypted),
    UploadIntervalInMinutes(other.UploadIntervalInMinutes),
    DataDeletionAgeInDays(other.DataDeletionAgeInDays),
    Parameters(other.Parameters)
{
}

AzureStoreBaseDescription::AzureStoreBaseDescription(AzureStoreBaseDescription && other) 
    : IsEnabled(move(other.IsEnabled)),
    ConnectionString(move(other.ConnectionString)),
    ConnectionStringIsEncrypted(move(other.ConnectionStringIsEncrypted)),
    UploadIntervalInMinutes(move(other.UploadIntervalInMinutes)),
    DataDeletionAgeInDays(move(other.DataDeletionAgeInDays)),
    Parameters(move(other.Parameters))
{
}

AzureStoreBaseDescription const & AzureStoreBaseDescription::operator = (AzureStoreBaseDescription const & other)
{
    if (this != &other)
    {
        this->IsEnabled = other.IsEnabled;
        this->ConnectionString = other.ConnectionString;
        this->ConnectionStringIsEncrypted = other.ConnectionStringIsEncrypted;
        this->UploadIntervalInMinutes = other.UploadIntervalInMinutes;
        this->DataDeletionAgeInDays = other.DataDeletionAgeInDays;
        this->Parameters = other.Parameters;
    }
    return *this;
}

AzureStoreBaseDescription const & AzureStoreBaseDescription::operator = (AzureStoreBaseDescription && other)
{
    if (this != &other)
    {
        this->IsEnabled = move(other.IsEnabled);
        this->ConnectionString = move(other.ConnectionString);
        this->ConnectionStringIsEncrypted = move(other.ConnectionStringIsEncrypted);
        this->UploadIntervalInMinutes = move(other.UploadIntervalInMinutes);
        this->DataDeletionAgeInDays = move(other.DataDeletionAgeInDays);
        this->Parameters = move(other.Parameters);
    }
    return *this;
}

bool AzureStoreBaseDescription::operator == (AzureStoreBaseDescription const & other) const
{
    bool equals = true;

    equals = this->IsEnabled == other.IsEnabled;
    if (!equals) { return equals; }

    equals = this->ConnectionString == other.ConnectionString;
    if (!equals) { return equals; }

    equals = this->ConnectionStringIsEncrypted == other.ConnectionStringIsEncrypted;
    if (!equals) { return equals; }

    equals = this->UploadIntervalInMinutes == other.UploadIntervalInMinutes;
    if (!equals) { return equals; }

    equals = this->DataDeletionAgeInDays == other.DataDeletionAgeInDays;
    if (!equals) { return equals; }

    equals = this->Parameters == other.Parameters;
    if (!equals) { return equals; }

    return equals;
}

bool AzureStoreBaseDescription::operator != (AzureStoreBaseDescription const & other) const
{
    return !(*this == other);
}

void AzureStoreBaseDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("AzureStoreBaseDescription { ");

    w.Write("IsEnabled = {0}", this->IsEnabled);
    w.Write("UploadIntervalInMinutes = {0}", this->UploadIntervalInMinutes);
    w.Write("DataDeletionAgeInDays = {0}", this->DataDeletionAgeInDays);

    w.Write("Parameters = {");
    w.Write(this->Parameters);
    w.Write("}");

    w.Write("}");
}

void AzureStoreBaseDescription::ReadFromXmlWorker(
    XmlReaderUPtr const & xmlReader)
{
    // <Azure* IsEnabled="" ConnectionString="" ConnectionStringIsEncrypted="" UploadIntervalInMinutes="" DataDeletionAgeInDays="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsEnabled))
    {
        this->IsEnabled = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsEnabled);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ConnectionString))
    {
        this->ConnectionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ConnectionString);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ConnectionStringIsEncrypted))
    {
        this->ConnectionStringIsEncrypted = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ConnectionStringIsEncrypted);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UploadIntervalInMinutes))
    {
        this->UploadIntervalInMinutes = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UploadIntervalInMinutes);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DataDeletionAgeInDays))
    {
        this->DataDeletionAgeInDays = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DataDeletionAgeInDays);
    }

    if (xmlReader->IsEmptyElement())
    {
        // <Azure* />
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

ErrorCode AzureStoreBaseDescription::WriteXmlWorker(XmlWriterUPtr const & xmlWriter)
{
	// all the attributes
	ErrorCode er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_IsEnabled, this->IsEnabled);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ConnectionString, this->ConnectionString);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ConnectionStringIsEncrypted, this->ConnectionStringIsEncrypted);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_UploadIntervalInMinutes, this->UploadIntervalInMinutes);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DataDeletionAgeInDays, this->DataDeletionAgeInDays);
	if (!er.IsSuccess())
	{
		return er;
	}
	//parameters
	return this->Parameters.WriteToXml(xmlWriter);
}

void AzureStoreBaseDescription::clear()
{
    this->IsEnabled.clear();
    this->ConnectionString.clear();
    this->UploadIntervalInMinutes.clear();
    this->DataDeletionAgeInDays.clear();
    this->Parameters.clear();
}
