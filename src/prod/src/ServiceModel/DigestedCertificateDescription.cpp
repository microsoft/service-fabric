// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DigestedCertificatesDescription::DigestedCertificatesDescription() :
    RolloutVersionValue(),    
    SecretsCertificate(),
    EndpointCertificates()
{
}

DigestedCertificatesDescription::DigestedCertificatesDescription(DigestedCertificatesDescription const & other) :
    RolloutVersionValue(other.RolloutVersionValue),    
    SecretsCertificate(other.SecretsCertificate),
    EndpointCertificates(other.EndpointCertificates)
{
}

DigestedCertificatesDescription::DigestedCertificatesDescription(DigestedCertificatesDescription && other) :
    RolloutVersionValue(move(other.RolloutVersionValue)),    
    SecretsCertificate(move(other.SecretsCertificate)),
    EndpointCertificates(move(other.EndpointCertificates))
{
}

DigestedCertificatesDescription const & DigestedCertificatesDescription::operator = (DigestedCertificatesDescription const & other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = other.RolloutVersionValue;        
        this->SecretsCertificate = other.SecretsCertificate;
        this->EndpointCertificates = other.EndpointCertificates;
    }

    return *this;
}

DigestedCertificatesDescription const & DigestedCertificatesDescription::operator = (DigestedCertificatesDescription && other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = move(other.RolloutVersionValue);        
        this->SecretsCertificate = move(other.SecretsCertificate);
        this->EndpointCertificates = move(other.EndpointCertificates);
    }

    return *this;
}

bool DigestedCertificatesDescription::operator == (DigestedCertificatesDescription const & other) const
{
    bool equals = true;

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }    

    equals = this->SecretsCertificate == other.SecretsCertificate;
    if (!equals) { return equals; }

    equals = this->EndpointCertificates == other.EndpointCertificates;
    if (!equals) { return equals; }

    return equals;
}

bool DigestedCertificatesDescription::operator != (DigestedCertificatesDescription const & other) const
{
    return !(*this == other);
}

void DigestedCertificatesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DigestedCertificatesDescription { ");
    w.Write("RolloutVersion = {0}, ", RolloutVersionValue);

    w.Write("SecretsCertificates = {");
    for(auto iter = SecretsCertificate.begin(); iter != SecretsCertificate.end(); ++iter)
    {
        w.Write("{0}", *iter);
    }
    w.Write("}");

    w.Write("EndpointCertificates = {");
    for (auto iter = EndpointCertificates.begin(); iter != EndpointCertificates.end(); ++iter)
    {
        w.Write("{0}", *iter);
    }
    w.Write("}");

    w.Write("}");
}

void DigestedCertificatesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DigestedCertificates RolloutVersion=""
    xmlReader->StartElement(
        *SchemaNames::Element_DigestedCertificates,
        *SchemaNames::Namespace);

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
        this->ReadSecretCertificates(xmlReader);
        xmlReader->ReadEndElement();
    }
}

void DigestedCertificatesDescription::ReadSecretCertificates(XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    while(!done)
    {
        done = true;

        if (xmlReader->IsStartElement(*SchemaNames::Element_SecretsCertificate, *SchemaNames::Namespace))
        {
            SecretsCertificateDescription secretCertificate;
            secretCertificate.ReadFromXml(xmlReader);
            this->SecretsCertificate.push_back(secretCertificate);
            done = false;
        }
        if (xmlReader->IsStartElement(*SchemaNames::Element_EndpointCertificate, *SchemaNames::Namespace))
        {
            EndpointCertificateDescription endpointCertificate;
            endpointCertificate.ReadFromXml(xmlReader);
            this->EndpointCertificates.push_back(endpointCertificate);
            done = false;
        }
    }
}

ErrorCode DigestedCertificatesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<DigestedCertificates>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedCertificates, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	//rolloutversion
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	for (size_t i = 0; i < SecretsCertificate.size(); i++)
	{
		er = SecretsCertificate[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</DigestedCertificates>
	return xmlWriter->WriteEndElement();
}

void DigestedCertificatesDescription::clear()
{        
    this->SecretsCertificate.clear();    
}
