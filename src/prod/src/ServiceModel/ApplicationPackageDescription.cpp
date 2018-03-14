// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationPackageDescription::ApplicationPackageDescription() 
    : ApplicationId(),
    RolloutVersionValue(),
    ApplicationTypeName(),
    ApplicationName(),
    DigestedEnvironment(),
    DigestedCertificates(),
    ContentChecksum()
{
}

ApplicationPackageDescription::ApplicationPackageDescription(
    ApplicationPackageDescription const & other) 
    : ApplicationId(other.ApplicationId),
    RolloutVersionValue(other.RolloutVersionValue),
    ApplicationTypeName(other.ApplicationTypeName),
    ApplicationName(other.ApplicationName),
    DigestedEnvironment(other.DigestedEnvironment),
    DigestedCertificates(other.DigestedCertificates),
    ContentChecksum(other.ContentChecksum)
{
}

ApplicationPackageDescription::ApplicationPackageDescription(ApplicationPackageDescription && other) 
    : ApplicationId(move(other.ApplicationId)),    
    RolloutVersionValue(move(other.RolloutVersionValue)),
    ApplicationTypeName(move(other.ApplicationTypeName)),
    ApplicationName(move(other.ApplicationName)),
    DigestedEnvironment(move(other.DigestedEnvironment)),
    DigestedCertificates(move(other.DigestedCertificates)),
    ContentChecksum(move(other.ContentChecksum))
{
}

ApplicationPackageDescription const & ApplicationPackageDescription::operator = (ApplicationPackageDescription const & other)
{
    if (this != &other)
    {
        this->ApplicationId = other.ApplicationId;        
        this->RolloutVersionValue = other.RolloutVersionValue;
        this->ApplicationTypeName = other.ApplicationTypeName;
        this->ApplicationName = other.ApplicationName;
        this->DigestedEnvironment = other.DigestedEnvironment;        
        this->DigestedCertificates = other.DigestedCertificates;        
        this->ContentChecksum = other.ContentChecksum;
    }

    return *this;
}

ApplicationPackageDescription const & ApplicationPackageDescription::operator = (ApplicationPackageDescription && other)
{
    if (this != &other)
    {
        this->ApplicationId = move(other.ApplicationId);        
        this->RolloutVersionValue = move(other.RolloutVersionValue);
        this->ApplicationTypeName = move(other.ApplicationTypeName);       
        this->ApplicationName = move(other.ApplicationName);       
        this->DigestedEnvironment = move(other.DigestedEnvironment);        
        this->DigestedCertificates = move(other.DigestedCertificates);        
        this->ContentChecksum = move(other.ContentChecksum); 
    }

    return *this;
}

void ApplicationPackageDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationPackageDescription: { ");
    
    w.Write("ApplicationId = {0}", ApplicationId);
    w.Write("ApplicationName = {0}", ApplicationName);       
    w.Write("ApplicationTypeName = {0}", ApplicationTypeName);       
    w.Write("RolloutVersionValue = {0}", RolloutVersionValue);
    w.Write("DigestedEnvironment = {0}", DigestedEnvironment);     
    w.Write("DigestedCertificates = {0}", DigestedCertificates);     
    w.Write("ContentChecksum = {0}", ContentChecksum);

    w.Write("}");
}

void ApplicationPackageDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_ApplicationPackage,
        *SchemaNames::Namespace);       

    this->ApplicationId = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ApplicationId);
    this->ApplicationTypeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ApplicationTypeName);
    this->ApplicationName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_NameUri);
            
    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);    
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("Format of RolloutVersion is invalid. RolloutVersion: {0}", rolloutVersionString);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_ContentChecksum))
    {
        this->ContentChecksum = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ContentChecksum);
    }
    
    xmlReader->ReadStartElement();    
    
    this->DigestedEnvironment.ReadFromXml(xmlReader);
    this->DigestedCertificates.ReadFromXml(xmlReader); 

    //</ApplicationInstance>
    xmlReader->ReadEndElement();
}

ErrorCode ApplicationPackageDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<ApplicationPackage>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ApplicationPackage, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	//5 attributes
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ApplicationId, this->ApplicationId);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ApplicationTypeName, this->ApplicationTypeName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_NameUri, this->ApplicationName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ContentChecksum, this->ContentChecksum);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = DigestedEnvironment.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = DigestedCertificates.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	//</ApplicationPackage>

	er = xmlWriter->WriteEndElement();
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->Flush();
	 
}

void ApplicationPackageDescription::clear()
{
    this->ApplicationId.clear();
    this->ApplicationName.clear();
    this->ApplicationTypeName.clear();
    this->DigestedEnvironment.clear();
    this->DigestedCertificates.clear();
    this->ContentChecksum.clear();
}
