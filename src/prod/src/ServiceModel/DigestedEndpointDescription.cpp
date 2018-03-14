// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DigestedEndpointDescription::DigestedEndpointDescription() :
Endpoint(),    
    SecurityAccessPolicy(),
    EndpointBindingPolicy()
{
}

DigestedEndpointDescription::DigestedEndpointDescription(DigestedEndpointDescription const & other) :
Endpoint(other.Endpoint),    
    SecurityAccessPolicy(other.SecurityAccessPolicy),
    EndpointBindingPolicy(other.EndpointBindingPolicy)
{
}

DigestedEndpointDescription::DigestedEndpointDescription(DigestedEndpointDescription && other) :
Endpoint(move(other.Endpoint)),    
    SecurityAccessPolicy(move(other.SecurityAccessPolicy)),
    EndpointBindingPolicy(move(other.EndpointBindingPolicy))
{
}

DigestedEndpointDescription const & DigestedEndpointDescription::operator = (DigestedEndpointDescription const & other)
{
    if (this != & other)
    {
        this->Endpoint = other.Endpoint;        
        this->SecurityAccessPolicy = other.SecurityAccessPolicy;
        this->EndpointBindingPolicy = other.EndpointBindingPolicy;
    }

    return *this;
}

DigestedEndpointDescription const & DigestedEndpointDescription::operator = (DigestedEndpointDescription && other)
{
    if (this != & other)
    {
        this->Endpoint = move(other.Endpoint);        
        this->SecurityAccessPolicy = move(other.SecurityAccessPolicy);
        this->EndpointBindingPolicy = move(other.EndpointBindingPolicy);
    }

    return *this;
}

bool DigestedEndpointDescription::operator == (DigestedEndpointDescription const & other) const
{
    bool equals = true;        

    equals = this->Endpoint == other.Endpoint;
    if (!equals) { return equals; }

    equals = this->SecurityAccessPolicy == other.SecurityAccessPolicy;
    if (!equals) { return equals; }

    equals = this->EndpointBindingPolicy == other.EndpointBindingPolicy;
    if (!equals) { return equals; }

    return equals;
}

bool DigestedEndpointDescription::operator != (DigestedEndpointDescription const & other) const
{
    return !(*this == other);
}

void DigestedEndpointDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DigestedEndpointDescription { ");    

    w.Write("Endpoint = {");
    w.Write("{0}", Endpoint);
    w.Write("}, ");

    w.Write("SecurityAccessPolicy = {");
    w.Write("{0}", SecurityAccessPolicy);
    w.Write("}");

    w.Write("EndpointBindingPolicy = {");
    w.Write("{0}", EndpointBindingPolicy);
    w.Write("}");

    w.Write("}");
}

void DigestedEndpointDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DigestedEndpoint>
    xmlReader->StartElement(
        *SchemaNames::Element_DigestedEndpoint,
        *SchemaNames::Namespace);    

    xmlReader->ReadStartElement();

    // <Endpoint .. />
    this->Endpoint.ReadFromXml(xmlReader);

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_SecurityAccessPolicy,
        *SchemaNames::Namespace,
        false))
    {        
        // <SecurityAccessPolicy .. />
        this->SecurityAccessPolicy.ReadFromXml(xmlReader);
    }

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_EndpointBindingPolicy,
        *SchemaNames::Namespace,
        false))
    {
        // <EndpointBindingPolicy .. />
        this->EndpointBindingPolicy.ReadFromXml(xmlReader);
    }

    // </DigestedEndpoint>
    xmlReader->ReadEndElement();
}

ErrorCode DigestedEndpointDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//DigestedEndPoint
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedEndpoint, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = Endpoint.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = SecurityAccessPolicy.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</DigestedEndpoint>
	return xmlWriter->WriteEndElement();
}

void DigestedEndpointDescription::clear()
{        
    this->Endpoint.clear();
    this->SecurityAccessPolicy.clear();
}
