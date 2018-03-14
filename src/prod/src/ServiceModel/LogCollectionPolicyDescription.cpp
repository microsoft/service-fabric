// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

LogCollectionPolicyDescription::LogCollectionPolicyDescription() 
    : Path()
{
}

LogCollectionPolicyDescription::LogCollectionPolicyDescription(LogCollectionPolicyDescription const & other)
    : Path(other.Path)
{
}

LogCollectionPolicyDescription::LogCollectionPolicyDescription(LogCollectionPolicyDescription && other)
    : Path(move(other.Path))
{
}

LogCollectionPolicyDescription const & LogCollectionPolicyDescription::operator = (LogCollectionPolicyDescription const & other)
{
    if (this != &other)
    {
        this->Path = other.Path;
    }

    return *this;
}

LogCollectionPolicyDescription const & LogCollectionPolicyDescription::operator = (LogCollectionPolicyDescription && other)
{
    if (this != &other)
    {
        this->Path = move(other.Path);
    }

    return *this;
}

bool LogCollectionPolicyDescription::operator == (LogCollectionPolicyDescription const & other) const
{
    return StringUtility::AreEqualCaseInsensitive(this->Path, other.Path);            
}

bool LogCollectionPolicyDescription::operator != (LogCollectionPolicyDescription const & other) const
{
    return !(*this == other);
}

void LogCollectionPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("LogCollectionPolicyDescription { ");
    w.Write("Path = {0}", Path);
    w.Write("}");
}

void LogCollectionPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // <LogCollectionPolicy Path="">
    xmlReader->StartElement(
        *SchemaNames::Element_LogCollectionPolicy,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Path))
    {
        this->Path = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Path);
    }
    
    // Read the rest of the empty LogCollectionPolicy element 
    xmlReader->ReadElement();
}

ErrorCode LogCollectionPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<LogCollectionPolicy>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_LogCollectionPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Path, this->Path);
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->WriteEndElement();
	
}

void LogCollectionPolicyDescription::clear()
{
    this->Path.clear();
}
