// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DigestedConfigPackageDescription::DigestedConfigPackageDescription() :
RolloutVersionValue(),
    ConfigPackage(),
    ConfigOverrides(),
    ContentChecksum(),
    DebugParameters(),
    IsShared(false)
{
}

DigestedConfigPackageDescription::DigestedConfigPackageDescription(DigestedConfigPackageDescription const & other) :
RolloutVersionValue(other.RolloutVersionValue),
    ConfigPackage(other.ConfigPackage),
    ConfigOverrides(other.ConfigOverrides),
    ContentChecksum(other.ContentChecksum),
    DebugParameters(other.DebugParameters),
    IsShared(other.IsShared)
{
}

DigestedConfigPackageDescription::DigestedConfigPackageDescription(DigestedConfigPackageDescription && other) :
RolloutVersionValue(move(other.RolloutVersionValue)),
    ConfigPackage(move(other.ConfigPackage)),
    ConfigOverrides(move(other.ConfigOverrides)),
    ContentChecksum(move(other.ContentChecksum)),
    DebugParameters(move(other.DebugParameters)),
    IsShared(move(other.IsShared))
{
}

DigestedConfigPackageDescription const & DigestedConfigPackageDescription::operator = (DigestedConfigPackageDescription const & other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = other.RolloutVersionValue;
        this->ConfigPackage = other.ConfigPackage;
        this->ConfigOverrides = other.ConfigOverrides;
        this->ContentChecksum = other.ContentChecksum;
        this->DebugParameters = other.DebugParameters;
        this->IsShared = other.IsShared;
    }

    return *this;
}

DigestedConfigPackageDescription const & DigestedConfigPackageDescription::operator = (DigestedConfigPackageDescription && other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = move(other.RolloutVersionValue);
        this->ConfigPackage = move(other.ConfigPackage);
        this->ConfigOverrides = move(other.ConfigOverrides);
        this->ContentChecksum = move(other.ContentChecksum);
        this->DebugParameters = move(other.DebugParameters);
        this->IsShared = other.IsShared;
    }

    return *this;
}

bool DigestedConfigPackageDescription::operator == (DigestedConfigPackageDescription const & other) const
{
    bool equals = true;

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }    

    equals = this->ConfigPackage == other.ConfigPackage;
    if (!equals) { return equals; }

    equals = this->ConfigOverrides == other.ConfigOverrides;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ContentChecksum, other.ContentChecksum);
    if (!equals) { return equals; }

    equals = this->DebugParameters == other.DebugParameters;
    if (!equals) { return equals; }

    equals = this->IsShared == other.IsShared;
    if(!equals) { return IsShared; }

    return equals;
}

bool DigestedConfigPackageDescription::operator != (DigestedConfigPackageDescription const & other) const
{
    return !(*this == other);
}

void DigestedConfigPackageDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DigestedConfigPackageDescription { ");
    w.Write("RolloutVersion = {0}, ", RolloutVersionValue);

    w.Write("ConfigPackage = {");
    w.Write("{0}", ConfigPackage);
    w.Write("}, ");

    w.Write("ConfigOverrides = {");
    w.Write("{0}", ConfigOverrides);
    w.Write("}, ");

    w.Write("ContentChecksum = {0}", ContentChecksum);
    w.Write("DebugParameters = {0}", DebugParameters);
    w.Write("IsShared = {0}", IsShared);

    w.Write("}");
}

void DigestedConfigPackageDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DigestedConfigPackage RolloutVersion=""
    xmlReader->StartElement(
        *SchemaNames::Element_DigestedConfigPackage,
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

    // <ConfigPackage ../>
    this->ConfigPackage.ReadFromXml(xmlReader);

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ConfigOverride,
        *SchemaNames::Namespace,
        false))
    {        
        // <ConfigOverride ../>
        this->ConfigOverrides.ReadFromXml(xmlReader);
    }

    ParseDebugParameters(xmlReader);
    // </DigestedConfigPackage>
    xmlReader->ReadEndElement();
}
ErrorCode DigestedConfigPackageDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<DigestedConfigPackage>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedConfigPackage, L"", *SchemaNames::Namespace);
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
	er = ConfigPackage.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = ConfigOverrides.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
    er = DebugParameters.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
	//</DigestedConfigPackage>
	return xmlWriter->WriteEndElement();
}

void DigestedConfigPackageDescription::ParseDebugParameters(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_DebugParameters,
        *SchemaNames::Namespace,
        false))
    {
        this->DebugParameters.ReadFromXml(xmlReader);
    }
}

void DigestedConfigPackageDescription::clear()
{        
    this->ConfigPackage.clear();
    this->ConfigOverrides.clear();
}

