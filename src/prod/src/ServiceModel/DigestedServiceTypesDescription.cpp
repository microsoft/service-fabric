// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DigestedServiceTypesDescription::DigestedServiceTypesDescription() :
    RolloutVersionValue(),
    ServiceTypes()
{
}

DigestedServiceTypesDescription::DigestedServiceTypesDescription(DigestedServiceTypesDescription const & other) :
    RolloutVersionValue(other.RolloutVersionValue),
    ServiceTypes(other.ServiceTypes)    
{
}

DigestedServiceTypesDescription::DigestedServiceTypesDescription(DigestedServiceTypesDescription && other) :
    RolloutVersionValue(move(other.RolloutVersionValue)),    
    ServiceTypes(move(other.ServiceTypes))
{
}

DigestedServiceTypesDescription const & DigestedServiceTypesDescription::operator = (DigestedServiceTypesDescription const & other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = other.RolloutVersionValue;        
        this->ServiceTypes = other.ServiceTypes;
    }

    return *this;
}

DigestedServiceTypesDescription const & DigestedServiceTypesDescription::operator = (DigestedServiceTypesDescription && other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = move(other.RolloutVersionValue);        
        this->ServiceTypes = move(other.ServiceTypes);
    }

    return *this;
}

bool DigestedServiceTypesDescription::operator == (DigestedServiceTypesDescription const & other) const
{
    bool equals = true;

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }    
    
    equals = this->ServiceTypes.size() == other.ServiceTypes.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->ServiceTypes.size(); ++ix)
    {
        equals = this->ServiceTypes[ix] == other.ServiceTypes[ix];
        if (!equals) { return equals; }
    }

    return equals;
}

bool DigestedServiceTypesDescription::operator != (DigestedServiceTypesDescription const & other) const
{
    return !(*this == other);
}

void DigestedServiceTypesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DigestedServiceTypesDescription { ");
    w.Write("RolloutVersion = {0}, ", RolloutVersionValue);
    w.Write("ServiceTypes = {");
    for(auto iter = ServiceTypes.cbegin(); iter != ServiceTypes.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");
    w.Write("}");
}

void DigestedServiceTypesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DigestedServiceTypes RolloutVersion=""
    xmlReader->StartElement(
        *SchemaNames::Element_DigestedServiceTypes,
        *SchemaNames::Namespace);
    
    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);    
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("Format of RolloutVersion is invalid. RolloutVersion: {0}", rolloutVersionString);
    }

    xmlReader->ReadStartElement();

    ParseServiceTypes(xmlReader);

    xmlReader->ReadEndElement();
}

ErrorCode DigestedServiceTypesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<DigestedServiceTypes>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedServiceTypes, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceTypes, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < ServiceTypes.size(); i++)
	{
		er = ServiceTypes[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	er = xmlWriter->WriteEndElement();
	if (!er.IsSuccess())
	{
		return er;
	}
	//</DigestedServiceTypes>
	return xmlWriter->WriteEndElement();
}

void DigestedServiceTypesDescription::ParseServiceTypes(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_ServiceTypes,
        *SchemaNames::Namespace,
        false);

    xmlReader->ReadStartElement();

    bool done = false;
    while(!done)
    {
        bool statelessServiceTypePresent = false;
        bool statefulServiceTypePresent = false;

        // <StatelessServiceType>
        statelessServiceTypePresent = xmlReader->IsStartElement(
            *SchemaNames::Element_StatelessServiceType,
            *SchemaNames::Namespace);

        if (statelessServiceTypePresent)
        {
            // parse StatelessServiceTypeDescription
            ParseStatelessServiceType(xmlReader);
        }

        // <StatefulServiceType>
        statefulServiceTypePresent = xmlReader->IsStartElement(
            *SchemaNames::Element_StatefulServiceType,
            *SchemaNames::Namespace);

        if (statefulServiceTypePresent)
        {
            // parse StatefulServiceTypeDescription
            ParseStatefulServiceType(xmlReader);
        }

        done = ((!statelessServiceTypePresent) && (!statefulServiceTypePresent));
    }

    xmlReader->ReadEndElement();
}

void DigestedServiceTypesDescription::ParseStatelessServiceType(
    XmlReaderUPtr const & xmlReader)
{
    ServiceTypeDescription serviceTypeDescription;
    serviceTypeDescription.ReadFromXml(xmlReader, false);
    this->ServiceTypes.push_back(serviceTypeDescription);
}

void DigestedServiceTypesDescription::ParseStatefulServiceType(
    XmlReaderUPtr const & xmlReader)
{
    ServiceTypeDescription serviceTypeDescription;
    serviceTypeDescription.ReadFromXml(xmlReader, true);
    this->ServiceTypes.push_back(serviceTypeDescription);
}


void DigestedServiceTypesDescription::clear()
{        
    this->ServiceTypes.clear();    
}
