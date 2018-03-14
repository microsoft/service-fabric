// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EndpointBindingPolicyDescription::EndpointBindingPolicyDescription() 
    : EndpointRef(),
    CertificateRef()
{
}

EndpointBindingPolicyDescription::EndpointBindingPolicyDescription(EndpointBindingPolicyDescription const & other)
    : EndpointRef(other.EndpointRef),
    CertificateRef(other.CertificateRef)
{
}

EndpointBindingPolicyDescription::EndpointBindingPolicyDescription(EndpointBindingPolicyDescription && other)
    : EndpointRef(move(other.EndpointRef)),
    CertificateRef(move(other.CertificateRef))
{
}

EndpointBindingPolicyDescription const & EndpointBindingPolicyDescription::operator = (EndpointBindingPolicyDescription const & other)
{
    if (this != &other)
    {
        this->EndpointRef = other.EndpointRef;
        this->CertificateRef = other.CertificateRef;
    }

    return *this;
}

EndpointBindingPolicyDescription const & EndpointBindingPolicyDescription::operator = (EndpointBindingPolicyDescription && other)
{
    if (this != &other)
    {
        this->EndpointRef = move(other.EndpointRef);
        this->CertificateRef = move(other.CertificateRef);
    }

    return *this;
}

bool EndpointBindingPolicyDescription::operator == (EndpointBindingPolicyDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->CertificateRef, other.CertificateRef);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->EndpointRef, other.EndpointRef);
    if (!equals) { return equals; }

    return equals;
}

bool EndpointBindingPolicyDescription::operator != (EndpointBindingPolicyDescription const & other) const
{
    return !(*this == other);
}

void EndpointBindingPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("EndpointBindingPolicyDescription { ");
    w.Write("EndpointRef = {0}, ", EndpointRef);
    w.Write("CertificateRef = {0}", CertificateRef);
    w.Write("}");
}

void EndpointBindingPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <EndpointBindingPolicy EndpointRef="" CertificateRef="" PrincipalRef="" />
    xmlReader->StartElement(
        *SchemaNames::Element_EndpointBindingPolicy,
        *SchemaNames::Namespace);

    this->EndpointRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_EndpointRef);
    this->CertificateRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CertificateRef);

    // Read empty EndpointBindingPolicy element
    xmlReader->ReadElement();
}

Common::ErrorCode EndpointBindingPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//EndpointBindingPolicy
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_EndpointBindingPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_EndpointRef, this->EndpointRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CertificateRef, this->CertificateRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	
	//</EndpointBindingPolicy>
	return xmlWriter->WriteEndElement();
}

void EndpointBindingPolicyDescription::clear()
{
    this->EndpointRef.clear();
    this->CertificateRef.clear();
}
