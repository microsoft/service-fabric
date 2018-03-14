// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

PartitionDescription::PartitionDescription() 
    : Scheme(PartitionSchemeDescription::Invalid),
    PartitionCount(),
    LowKey(),
    HighKey(),
    PartitionNames()
{
}

PartitionDescription::PartitionDescription(PartitionDescription const & other) 
    : Scheme(other.Scheme),
    PartitionCount(other.PartitionCount),
    LowKey(other.LowKey),
    HighKey(other.HighKey),
    PartitionNames(other.PartitionNames)
{
}

PartitionDescription::PartitionDescription(PartitionDescription && other) 
    : Scheme(other.Scheme),
    PartitionCount(move(other.PartitionCount)),
    LowKey(move(other.LowKey)),
    HighKey(move(other.HighKey)),
    PartitionNames(move(other.PartitionNames))
{
}

PartitionDescription::PartitionDescription(
    PartitionSchemeDescription::Enum partitionScheme,
    int partitionCount,
    __int64 lowKey,
    __int64 highKey,
    vector<wstring> const &names)
    : Scheme(partitionScheme)
    , PartitionCount(partitionCount)
    , LowKey(lowKey)
    , HighKey(highKey)
    , PartitionNames(names)
{
}

PartitionDescription const & PartitionDescription::operator = (PartitionDescription const & other)
{
    if (this != &other)
    {
        this->Scheme = other.Scheme;
        this->PartitionCount = other.PartitionCount;
        this->LowKey = other.LowKey;
        this->HighKey = other.HighKey;
        this->PartitionNames = other.PartitionNames;
    }

    return *this;
}

PartitionDescription const & PartitionDescription::operator = (PartitionDescription && other)
{
    if (this != &other)
    {
        this->Scheme = move(other.Scheme);
        this->PartitionCount = move(other.PartitionCount);
        this->LowKey = move(other.LowKey);
        this->HighKey = move(other.HighKey);
        this->PartitionNames = move(other.PartitionNames);
    }

    return *this;
}

bool PartitionDescription::operator == (PartitionDescription const & other) const
{
    bool equals = true;

    equals = (this->Scheme == other.Scheme);
    if (!equals) { return equals; }

    equals = (this->PartitionCount == other.PartitionCount);
    if (!equals) { return equals; }

    equals = (this->LowKey == other.LowKey);
    if (!equals) { return equals; }

    equals = (this->HighKey == other.HighKey);
    if (!equals) { return equals; }

    equals = StringUtility::AreStringCollectionsEqualCaseInsensitive(this->PartitionNames, other.PartitionNames);
    if (!equals) { return equals; }

    return equals;
}

bool PartitionDescription::operator != (PartitionDescription const & other) const
{
    return !(*this == other);
}

void PartitionDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("PartitionDescription { ");
    w.Write("Scheme = {0}, ", Scheme);
    w.Write("PartitionCount = {0}, ", PartitionCount);
    w.Write("LowKey = {0}, ", LowKey);
    w.Write("HighKey = {0}, ", HighKey);   
    w.Write("PartitionNames = {");
    for (auto it = PartitionNames.begin(); it != PartitionNames.end(); ++it)
    {
        w << *it << " ";
    }
    w.Write("} "); // close PartitionNames enumeration
    w.Write("}"); // close PartitionDescription
}

void PartitionDescription::ReadFromXml(XmlReaderUPtr const & xmlReader, PartitionSchemeDescription::Enum scheme)
{
    clear();    

    this->Scheme = scheme;

    switch (scheme)
    {
    case PartitionSchemeDescription::Singleton:
        ReadSingletonPartitionFromXml(xmlReader);
        break;
    case PartitionSchemeDescription::UniformInt64Range:
        ReadUniformInt64PartitionFromXml(xmlReader);
        break;
    case PartitionSchemeDescription::Named:
        ReadNamedPartitionFromXml(xmlReader);
        break;
    default:
        Assert::CodingError("Unknown PartitionSchemeDescription value {0}", (int)scheme);
    }
}

void PartitionDescription::ReadSingletonPartitionFromXml(XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_SingletonPartition,
        *SchemaNames::Namespace,
        false);        
    this->PartitionCount = 1;  
    xmlReader->ReadElement();
}

void PartitionDescription::ReadUniformInt64PartitionFromXml(XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_UniformInt64Partition,
        *SchemaNames::Namespace);

    wstring partitionCountValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PartitionCount);
    if (!StringUtility::TryFromWString<uint>(
        partitionCountValue, 
        this->PartitionCount))
    {
        Parser::ThrowInvalidContent(xmlReader, L"positive integer", partitionCountValue);
    }

    wstring lowKeyValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_LowKey);
    if (!StringUtility::TryFromWString<__int64>(
        lowKeyValue, 
        this->LowKey))
    {
        Parser::ThrowInvalidContent(xmlReader, L"int64", lowKeyValue);
    }

    wstring highKeyValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_HighKey);
    if (!StringUtility::TryFromWString<__int64>(
        highKeyValue, 
        this->HighKey))
    {
        Parser::ThrowInvalidContent(xmlReader, L"int64", highKeyValue);
    }    

    xmlReader->ReadElement();
}     

void PartitionDescription::ReadNamedPartitionFromXml(XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_NamedPartition,
        *SchemaNames::Namespace);
    
    // <NamedPartition>
    xmlReader->ReadStartElement();

    bool done = false;
    while(!done)
    {
        done = true;
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_Partition,
            *SchemaNames::Namespace))
        {
            // <Partition>
            xmlReader->StartElement(
                *SchemaNames::Element_Partition, 
                *SchemaNames::Namespace);

            wstring name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
            if (name.empty())
            {
                Parser::ThrowInvalidContent(xmlReader, L"valid partition name", name);
            }
            this->PartitionNames.push_back(name);

            // </Partition>
            xmlReader->ReadElement();
            done = false;
        }
    }

    // </NamedPartition>
    xmlReader->ReadEndElement();

    this->PartitionCount = (ULONG)this->PartitionNames.size();
}


ErrorCode PartitionDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	if (this->Scheme == PartitionSchemeDescription::Singleton)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_SingletonPartition, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	else if (this->Scheme == PartitionSchemeDescription::UniformInt64Range)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_UniformInt64Partition, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_PartitionCount, PartitionCount);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_LowKey, LowKey);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_HighKey, HighKey);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	else
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_NamedPartition, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		for (auto i = 0; i < this->PartitionNames.size(); i++)
		{
			er = xmlWriter->WriteStartElement(*SchemaNames::Element_Partition, L"", *SchemaNames::Namespace);
			if (!er.IsSuccess())
			{
				return er;
			}
			er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, PartitionNames[i]);
			if (!er.IsSuccess())
			{
				return er;
			}
			er = xmlWriter->WriteEndElement();
			if (!er.IsSuccess())
			{
				return er;
			}
		}
	}
	return xmlWriter->WriteEndElement();
}

void PartitionDescription::clear()
{
    this->PartitionNames.clear();
}
