// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DigestedDataPackageDescription::DigestedDataPackageDescription() :
    RolloutVersionValue(),    
    DataPackage(),
    ContentChecksum(),
    DebugParameters(),
    IsShared(false)
{
}

DigestedDataPackageDescription::DigestedDataPackageDescription(DigestedDataPackageDescription const & other) :
    RolloutVersionValue(other.RolloutVersionValue),    
    DataPackage(other.DataPackage),
    ContentChecksum(other.ContentChecksum),
    DebugParameters(other.DebugParameters),
    IsShared(other.IsShared)
{
}

DigestedDataPackageDescription::DigestedDataPackageDescription(DigestedDataPackageDescription && other) :
    RolloutVersionValue(move(other.RolloutVersionValue)),    
    DataPackage(move(other.DataPackage)),
    ContentChecksum(move(other.ContentChecksum)),
    DebugParameters(move(other.DebugParameters)),
    IsShared(move(other.IsShared))
{
}

DigestedDataPackageDescription const & DigestedDataPackageDescription::operator = (DigestedDataPackageDescription const & other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = other.RolloutVersionValue;        
        this->DataPackage = other.DataPackage;
        this->ContentChecksum = other.ContentChecksum;
        this->DebugParameters = other.DebugParameters;
        this->IsShared = other.IsShared;
    }

    return *this;
}

DigestedDataPackageDescription const & DigestedDataPackageDescription::operator = (DigestedDataPackageDescription && other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = move(other.RolloutVersionValue);        
        this->DataPackage = move(other.DataPackage);
        this->ContentChecksum = move(other.ContentChecksum);
        this->DebugParameters = move(other.DebugParameters);
        this->IsShared = other.IsShared;
    }

    return *this;
}

bool DigestedDataPackageDescription::operator == (DigestedDataPackageDescription const & other) const
{
    bool equals = true;

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }    
    
    equals = this->DataPackage == other.DataPackage;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ContentChecksum, other.ContentChecksum);
    if (!equals) { return equals; }

    equals = this->DebugParameters == other.DebugParameters;
    if (!equals) { return equals; }

    equals = this->IsShared == other.IsShared;
    if(!equals) { return IsShared; }

    return equals;
}

bool DigestedDataPackageDescription::operator != (DigestedDataPackageDescription const & other) const
{
    return !(*this == other);
}

void DigestedDataPackageDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DigestedDataPackageDescription { ");
    w.Write("RolloutVersion = {0}, ", RolloutVersionValue);

    w.Write("DataPackage = {");
    w.Write("{0}", DataPackage);
    w.Write("}, ");

    w.Write("ContentChecksum = {0}", ContentChecksum);
    w.Write("DebugParameters = {0}", DebugParameters);
    w.Write("IsShared = {0}", IsShared);

    w.Write("}");
}

void DigestedDataPackageDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DigestedDataPackage RolloutVersion=""
    xmlReader->StartElement(
        *SchemaNames::Element_DigestedDataPackage,
        *SchemaNames::Namespace);
    
    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);    
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("Format of RolloutVersion is invalid. RolloutVersion: {0}", rolloutVersionString);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_ContentChecksum))
    {
        this->ContentChecksum = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ContentChecksum);
    }
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_IsShared))
    {
        wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsShared);
        if (!StringUtility::TryFromWString<bool>(
            attrValue, 
            this->IsShared))
        {
            Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
        }
    }
    xmlReader->ReadStartElement();

    // <DataPackage .. />
    this->DataPackage.ReadFromXml(xmlReader);       
    ParseDebugParameters(xmlReader);

    // </DigestedDataPackage>
    xmlReader->ReadEndElement();
}

ErrorCode DigestedDataPackageDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{ //DigestedDataPackage
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedDataPackage, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ContentChecksum, this->ContentChecksum);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsShared,
		this->IsShared);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = DataPackage.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}

	//</DigestedDataPackage>
	return xmlWriter->WriteEndElement();
}

void DigestedDataPackageDescription::ParseDebugParameters(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_DebugParameters,
        *SchemaNames::Namespace,
        false))
    {
        this->DebugParameters.ReadFromXml(xmlReader);
    }
}

void DigestedDataPackageDescription::clear()
{        
    this->DataPackage.clear();    
}
