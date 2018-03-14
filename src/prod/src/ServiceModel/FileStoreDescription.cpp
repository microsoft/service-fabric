// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

FileStoreDescription::FileStoreDescription() 
    : IsEnabled(),
    Path(),
    AccountType(),
    AccountName(),
    Password(),
    PasswordEncrypted(),
    UploadIntervalInMinutes(),
    DataDeletionAgeInDays(),
    Parameters()
{
}

FileStoreDescription::FileStoreDescription(
    FileStoreDescription const & other)
    : IsEnabled(other.IsEnabled),
    Path(other.Path),
    AccountType(other.AccountType),
    AccountName(other.AccountName),
    Password(other.Password),
    PasswordEncrypted(other.PasswordEncrypted),
    UploadIntervalInMinutes(other.UploadIntervalInMinutes),
    DataDeletionAgeInDays(other.DataDeletionAgeInDays),
    Parameters(other.Parameters)
{
}

FileStoreDescription::FileStoreDescription(FileStoreDescription && other) 
    : IsEnabled(move(other.IsEnabled)),
    Path(move(other.Path)),
    AccountType(move(other.AccountType)),
    AccountName(move(other.AccountName)),
    Password(move(other.Password)),
    PasswordEncrypted(move(other.PasswordEncrypted)),
    UploadIntervalInMinutes(move(other.UploadIntervalInMinutes)),
    DataDeletionAgeInDays(move(other.DataDeletionAgeInDays)),
    Parameters(move(other.Parameters))
{
}

FileStoreDescription const & FileStoreDescription::operator = (FileStoreDescription const & other)
{
    if (this != &other)
    {
        this->IsEnabled = other.IsEnabled;
        this->Path = other.Path;
        this->AccountType = other.AccountType;
        this->AccountName = other.AccountName;
        this->Password = other.Password;
        this->PasswordEncrypted = other.PasswordEncrypted;
        this->UploadIntervalInMinutes = other.UploadIntervalInMinutes;
        this->DataDeletionAgeInDays = other.DataDeletionAgeInDays;
        this->Parameters = other.Parameters;
    }
    return *this;
}

FileStoreDescription const & FileStoreDescription::operator = (FileStoreDescription && other)
{
    if (this != &other)
    {
        this->IsEnabled = move(other.IsEnabled);
        this->Path = move(other.Path);
        this->AccountType = move(other.AccountType);
        this->AccountName = move(other.AccountName);
        this->Password = move(other.Password);
        this->PasswordEncrypted = move(other.PasswordEncrypted);
        this->UploadIntervalInMinutes = move(other.UploadIntervalInMinutes);
        this->DataDeletionAgeInDays = move(other.DataDeletionAgeInDays);
        this->Parameters = move(other.Parameters);
    }
    return *this;
}

bool FileStoreDescription::operator == (FileStoreDescription const & other) const
{
    bool equals = true;

    equals = this->IsEnabled == other.IsEnabled;
    if (!equals) { return equals; }

    equals = this->Path == other.Path;
    if (!equals) { return equals; }

    equals = this->AccountType == other.AccountType;
    if (!equals) { return equals; }

    equals = this->AccountName == other.AccountName;
    if (!equals) { return equals; }

    equals = this->Password == other.Password;
    if (!equals) { return equals; }

    equals = this->PasswordEncrypted == other.PasswordEncrypted;
    if (!equals) { return equals; }

    equals = this->UploadIntervalInMinutes == other.UploadIntervalInMinutes;
    if (!equals) { return equals; }

    equals = this->DataDeletionAgeInDays == other.DataDeletionAgeInDays;
    if (!equals) { return equals; }

    equals = this->Parameters == other.Parameters;
    if (!equals) { return equals; }


    return equals;
}

bool FileStoreDescription::operator != (FileStoreDescription const & other) const
{
    return !(*this == other);
}

void FileStoreDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FileStoreDescription { ");

    w.Write("IsEnabled = {0}", this->IsEnabled);
    w.Write("Path = {0}", this->Path);
    w.Write("AccountType = {0}", this->AccountType);
    w.Write("AccountName = {0}", this->AccountName);
    w.Write("Password = {0}", this->Password);
    w.Write("PasswordEncrypted = {0}", this->PasswordEncrypted);
    w.Write("UploadIntervalInMinutes = {0}", this->UploadIntervalInMinutes);
    w.Write("DataDeletionAgeInDays = {0}", this->DataDeletionAgeInDays);

    w.Write("Parameters = {");
    w.Write(this->Parameters);
    w.Write("}");

    w.Write("}");
}

void FileStoreDescription::ReadFromXmlWorker(
    XmlReaderUPtr const & xmlReader)
{
    // <FileStore IsEnabled="" 
    //            Path="" 
    //            UploadIntervalInMinutes="" 
    //            DataDeletionAgeInDays="" 
    //            AccountType="" 
    //            AccountName="" 
    //            Password="" 
    //            PasswordEncrypted="">
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsEnabled))
    {
        this->IsEnabled = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsEnabled);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Path))
    {
        this->Path = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Path);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UploadIntervalInMinutes))
    {
        this->UploadIntervalInMinutes = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UploadIntervalInMinutes);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DataDeletionAgeInDays))
    {
        this->DataDeletionAgeInDays = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DataDeletionAgeInDays);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_AccountType))
    {
        this->AccountType = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_AccountType);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_AccountName))
    {
        this->AccountName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_AccountName);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Password))
    {
        this->Password = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Password);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_PasswordEncrypted))
    {
        this->PasswordEncrypted = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PasswordEncrypted);
    }

    if (xmlReader->IsEmptyElement())
    {
        // <FileStore />
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

void FileStoreDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_FileStore,
        *SchemaNames::Namespace,
        false);

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

ErrorCode FileStoreDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<FileStore>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_FileStore, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return  er;
	}

	er = WriteBaseToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return  er;
	}

	//</Filestore>
	return xmlWriter->WriteEndElement();
}

ErrorCode FileStoreDescription::WriteBaseToXml(XmlWriterUPtr const & xmlWriter)
{
	//All the attributes
	ErrorCode er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_IsEnabled, this->IsEnabled);
	if (!er.IsSuccess())
	{
		return  er;
	}er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Path, this->Path);
	if (!er.IsSuccess())
	{
		return  er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_UploadIntervalInMinutes, this->UploadIntervalInMinutes);
	if (!er.IsSuccess())
	{
		return  er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DataDeletionAgeInDays, this->DataDeletionAgeInDays);
	if (!er.IsSuccess())
	{
		return  er;
	}
	if (!er.IsSuccess())
	{
		return  er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_AccountType, this->AccountType);
	if (!er.IsSuccess())
	{
		return  er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_AccountName, this->AccountName);
	if (!er.IsSuccess())
	{
		return  er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Password, this->Password);
	if (!er.IsSuccess())
	{
		return  er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_PasswordEncrypted, this->PasswordEncrypted);
	if (!er.IsSuccess())
	{
		return  er;
	}

	er = Parameters.WriteToXml(xmlWriter);
	return er;
}

void FileStoreDescription::clear()
{
    this->IsEnabled.clear();
    this->Path.clear();
    this->AccountType.clear();
    this->AccountName.clear();
    this->Password.clear();
    this->PasswordEncrypted.clear();
    this->UploadIntervalInMinutes.clear();
    this->DataDeletionAgeInDays.clear();
    this->Parameters.clear();
}
