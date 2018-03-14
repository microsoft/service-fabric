// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DigestedResourcesDescription::DigestedResourcesDescription() :
    RolloutVersionValue(),
    DigestedEndpoints(),
    DigestedCertificates()
{
}

DigestedResourcesDescription::DigestedResourcesDescription(DigestedResourcesDescription const & other) :
    RolloutVersionValue(other.RolloutVersionValue),
    DigestedEndpoints(other.DigestedEndpoints),
    DigestedCertificates(other.DigestedCertificates)
{
}

DigestedResourcesDescription::DigestedResourcesDescription(DigestedResourcesDescription && other) :
    RolloutVersionValue(move(other.RolloutVersionValue)),    
    DigestedEndpoints(move(other.DigestedEndpoints)),
    DigestedCertificates(move(other.DigestedCertificates))
{
}

DigestedResourcesDescription const & DigestedResourcesDescription::operator = (DigestedResourcesDescription const & other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = other.RolloutVersionValue;        
        this->DigestedEndpoints = other.DigestedEndpoints;
        this->DigestedCertificates = other.DigestedCertificates;
    }

    return *this;
}

DigestedResourcesDescription const & DigestedResourcesDescription::operator = (DigestedResourcesDescription && other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = move(other.RolloutVersionValue);        
        this->DigestedEndpoints = move(other.DigestedEndpoints);
        this->DigestedCertificates = move(other.DigestedCertificates);
    }

    return *this;
}

bool DigestedResourcesDescription::operator == (DigestedResourcesDescription const & other) const
{
    bool equals = true;

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }    
    
    equals = this->DigestedEndpoints.size() == other.DigestedEndpoints.size();
    if (!equals) { return equals; }

    for (auto iter = this->DigestedEndpoints.begin();
        iter != this->DigestedEndpoints.end();
        ++iter)
    {
        auto otherIter = other.DigestedEndpoints.find(iter->first);
        equals = ((otherIter != other.DigestedEndpoints.end()) && (iter->second == otherIter->second));
        if (!equals) { return equals; }
    }

    for (auto iter = this->DigestedCertificates.begin();
        iter != this->DigestedCertificates.end();
        ++iter)
    {
        auto otherIter = other.DigestedCertificates.find(iter->first);
        equals = ((otherIter != other.DigestedCertificates.end()) && (iter->second == otherIter->second));
        if (!equals) { return equals; }
    }

    return equals;
}

bool DigestedResourcesDescription::operator != (DigestedResourcesDescription const & other) const
{
    return !(*this == other);
}

void DigestedResourcesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DigestedResourcesDescription { ");
    w.Write("RolloutVersion = {0}, ", RolloutVersionValue);
    w.Write("DigestedEndpoints = {");
    for(auto iter = DigestedEndpoints.cbegin(); iter != DigestedEndpoints.cend(); ++ iter)
    {
        w.Write("{0},", iter->second);
    }
    w.Write("}");
    w.Write("DigestedCertificates = {");
    for (auto iter = DigestedCertificates.cbegin(); iter != DigestedCertificates.cend(); ++iter)
    {
        w.Write("{0},", iter->second);
    }
    w.Write("}");
    w.Write("}");
}

void DigestedResourcesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DigestedResources
    xmlReader->StartElement(
        *SchemaNames::Element_DigestedResources,
        *SchemaNames::Namespace);

    // <DigestedCodePackage RolloutVersion=""
    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);    
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("Format of RolloutVersion is invalid. RolloutVersion: {0}", rolloutVersionString);
    }

    if (xmlReader->IsEmptyElement())
    {
        xmlReader->ReadElement();
    }
    else
    {
        xmlReader->ReadStartElement();

        ParseDigestedEndpoints(xmlReader);

        ParseDigestedCertificates(xmlReader);

        xmlReader->ReadEndElement();
    }
}

void DigestedResourcesDescription::ParseDigestedEndpoints(
    XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_DigestedEndpoints,
        *SchemaNames::Namespace,
        false))
    {
        // <DigestedEndpoints>
        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_DigestedEndpoint,
                *SchemaNames::Namespace,
                false))
            {
                DigestedEndpointDescription digestedEndpoint;                
                digestedEndpoint.ReadFromXml(xmlReader);
                this->DigestedEndpoints.insert(make_pair(digestedEndpoint.Endpoint.Name, digestedEndpoint));
            }
            else
            {
                done = true;
            }
        }

        // </DigestedEndpoints>
        xmlReader->ReadEndElement();
    }
}

void DigestedResourcesDescription::ParseDigestedCertificates(
    XmlReaderUPtr const & xmlReader)
{
        if (xmlReader->IsStartElement(
        *SchemaNames::Element_DigestedCertificates,
        *SchemaNames::Namespace,
        false))
        {
            if (xmlReader->IsEmptyElement())
            {
                // <Stateful/StatelessServiceType .. />
                xmlReader->ReadElement();
            }
            else
            {

                // <DigestedCertificates .... />
                xmlReader->ReadStartElement();
                bool done = false;
                while (!done)
                {
                    if (xmlReader->IsStartElement(
                        *SchemaNames::Element_EndpointCertificate,
                        *SchemaNames::Namespace,
                        false))
                    {
                        EndpointCertificateDescription certificateDescription;
                        certificateDescription.ReadFromXml(xmlReader);
                        this->DigestedCertificates.insert(make_pair(certificateDescription.Name, certificateDescription));
                    }
                    else
                    {
                        done = true;
                    }
                }

                // </DigestedCertificates>
                xmlReader->ReadEndElement();
            }
        }
}



ErrorCode DigestedResourcesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<DigestedResources>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedResources, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteDigestedEndPoints(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteDigestedCertificates(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</DigestedResources>
	return xmlWriter->WriteEndElement();
}
ErrorCode DigestedResourcesDescription::WriteDigestedEndPoints(XmlWriterUPtr const & xmlWriter)
{	//DigestedEndPoints
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedEndpoints, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (map<wstring, DigestedEndpointDescription>::iterator it = DigestedEndpoints.begin(); it != DigestedEndpoints.end(); ++it)
	{
		er = (*it).second.WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</DigestedEndpoints>
	return xmlWriter->WriteEndElement();
}
ErrorCode DigestedResourcesDescription::WriteDigestedCertificates(XmlWriterUPtr const & xmlWriter)
{
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedCertificates, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    for (map<wstring, EndpointCertificateDescription>::iterator it = DigestedCertificates.begin(); it != DigestedCertificates.end(); ++it)
    {
        er = (*it).second.WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    //</DigestedCertificates>
    return xmlWriter->WriteEndElement();
}

void DigestedResourcesDescription::clear()
{        
    this->DigestedEndpoints.clear();
    this->DigestedCertificates.clear();
}
