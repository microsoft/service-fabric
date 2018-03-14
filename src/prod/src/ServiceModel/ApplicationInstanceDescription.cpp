// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationInstanceDescription::ApplicationInstanceDescription() 
    : ApplicationId(),
    Version(),
    ApplicationTypeName(),
    ApplicationTypeVersion(),    
    ApplicationPackageReference(),
    ServicePackageReferences(),
    ServiceTemplates(),
	NameUri(),
    DefaultServices()    
{
}

ApplicationInstanceDescription::ApplicationInstanceDescription(
	ApplicationInstanceDescription const & other)
	: ApplicationId(other.ApplicationId),
	Version(other.Version),
	ApplicationTypeName(other.ApplicationTypeName),
	ApplicationTypeVersion(other.ApplicationTypeVersion),
	ApplicationPackageReference(other.ApplicationPackageReference),
	ServicePackageReferences(other.ServicePackageReferences),
	ServiceTemplates(other.ServiceTemplates),
	NameUri(other.NameUri),
    DefaultServices(other.DefaultServices)    
{
}

ApplicationInstanceDescription::ApplicationInstanceDescription(ApplicationInstanceDescription && other) 
    : ApplicationId(move(other.ApplicationId)),    
    Version(move(other.Version)),
    ApplicationTypeName(move(other.ApplicationTypeName)),
    ApplicationTypeVersion(move(other.ApplicationTypeVersion)),    
	NameUri(other.NameUri),
    ApplicationPackageReference(move(other.ApplicationPackageReference)),
    ServicePackageReferences(move(other.ServicePackageReferences)),
    ServiceTemplates(move(other.ServiceTemplates)),
    DefaultServices(move(other.DefaultServices))    
{
}

ApplicationInstanceDescription const & ApplicationInstanceDescription::operator = (ApplicationInstanceDescription const & other)
{
    if (this != &other)
    {
        this->ApplicationId = other.ApplicationId;        
        this->Version = other.Version;
        this->ApplicationTypeName = other.ApplicationTypeName;
        this->ApplicationTypeVersion = other.ApplicationTypeVersion;        
        this->ApplicationPackageReference = other.ApplicationPackageReference;
        this->ServicePackageReferences = other.ServicePackageReferences;
        this->ServiceTemplates = other.ServiceTemplates;
        this->DefaultServices = other.DefaultServices;
		this->NameUri = other.NameUri;
    }

    return *this;
}

ApplicationInstanceDescription const & ApplicationInstanceDescription::operator = (ApplicationInstanceDescription && other)
{
    if (this != &other)
    {
        this->ApplicationId = move(other.ApplicationId);        
        this->Version = move(other.Version);
        this->ApplicationTypeName = move(other.ApplicationTypeName);
        this->ApplicationTypeVersion = move(other.ApplicationTypeVersion);        
        this->ApplicationPackageReference = move(other.ApplicationPackageReference);
        this->ServicePackageReferences = move(other.ServicePackageReferences);
        this->ServiceTemplates = move(other.ServiceTemplates);
        this->DefaultServices = move(other.DefaultServices);      
		this->NameUri = move(other.NameUri);
    }

    return *this;
}

void ApplicationInstanceDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationInstanceDescription: { ");
    
    w.Write("ApplicationId = {0}", ApplicationId);
    w.Write("Version = {0}", Version);
    w.Write("ApplicationTypeName = {0}", ApplicationTypeName);
    w.Write("ApplicationTypeVersion = {0}", ApplicationTypeVersion);    
    w.Write("ApplicationPackageReference = {0}", ApplicationPackageReference);

    w.Write("ServicePackageReferences = {");
    for(auto iter = ServicePackageReferences.cbegin(); iter != ServicePackageReferences.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("ServiceTemplates = {");
    for(auto iter = ServiceTemplates.cbegin(); iter != ServiceTemplates.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("DefaultServices = {");
    for(auto iter = DefaultServices.cbegin(); iter != DefaultServices.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");   

    w.Write("}");
}

ErrorCode ApplicationInstanceDescription::FromXml(wstring const & fileName)
{
    return Parser::ParseApplicationInstance(fileName, *this);
}

void ApplicationInstanceDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_ApplicationInstance,
        *SchemaNames::Namespace);       

    this->ApplicationId = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ApplicationId);
    this->ApplicationTypeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ApplicationTypeName);
    this->ApplicationTypeVersion = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ApplicationTypeVersion);
	if (xmlReader->HasAttribute(*SchemaNames::Attribute_NameUri))
	{
		this->NameUri = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_NameUri);
	}
        
    wstring applicationInstanceVersion = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Version);    
    if (!StringUtility::TryFromWString<int>(
        applicationInstanceVersion, 
        this->Version))
    {
        Parser::ThrowInvalidContent(xmlReader, L"integer", applicationInstanceVersion);
    }
        
    xmlReader->ReadStartElement();    
    
    this->ApplicationPackageReference.ReadFromXml(xmlReader);
    
    bool done = false;
    do
    {
        if(xmlReader->IsStartElement(
            *SchemaNames::Element_ServicePackageRef, 
            *SchemaNames::Namespace,
            false))
        {
            ServicePackageReference servicePackageReference;
            servicePackageReference.ReadFromXml(xmlReader);
            this->ServicePackageReferences.push_back(move(servicePackageReference));
        }
        else
        {
            done = true;
        }
    }while(!done);

    this->ParseServiceTemplates(xmlReader);
    
    this->ParseDefaultServices(xmlReader);        

    //</ApplicationInstance>
    xmlReader->ReadEndElement();
}
ErrorCode ApplicationInstanceDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{	//ApplicationInstance
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ApplicationInstance, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	//write four attributes
	xmlWriter->Flush();
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_Version, this->Version);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_NameUri, this->NameUri);
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
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ApplicationTypeVersion, this->ApplicationTypeVersion);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = this->ApplicationPackageReference.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	for (int i = 0; i < this->ServicePackageReferences.size(); i++)
	{
		er = ServicePackageReferences[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	xmlWriter->Flush();

	if (!ServiceTemplates.empty())
	{	//<ServiceTemplates>
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceTemplates, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		for (int j = 0; j < ServiceTemplates.size(); j++)
		{
			er = ServiceTemplates[j].WriteToXml(xmlWriter);
			if (!er.IsSuccess())
			{
				return er;
			}
		}
		//</ServiceTemplates>
		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	xmlWriter->Flush();
	if (!DefaultServices.empty())
	{	//<DefaultServies>
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_DefaultServices, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		for (int k = 0; k < DefaultServices.size(); k++)
		{
			er = DefaultServices[k].WriteToXml(xmlWriter);
			if (!er.IsSuccess())
			{
				return er;
			}
		}
		//</DefaultServices>
		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</ApplicationInstance>
	er = xmlWriter->WriteEndElement();
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->Flush();
}


void ApplicationInstanceDescription::ParseServiceTemplates(XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_ServiceTemplates,
        *SchemaNames::Namespace);

    if(xmlReader->IsEmptyElement())
    {
        // <ServiceTemplates />
        xmlReader->ReadElement();
    }
    else
    {
        // <ServiceTemplates>
        xmlReader->ReadStartElement();

        bool done = false;
        do
        {
            bool isStateful = false;
            bool isServiceGroup = false;
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_StatefulService,
                *SchemaNames::Namespace,
                false))
            {
                isStateful = true;
            }
            else if (xmlReader->IsStartElement(
                *SchemaNames::Element_StatelessService,
                *SchemaNames::Namespace,
                false))
            {
                isStateful = false;
            }
            else if (xmlReader->IsStartElement(
                *SchemaNames::Element_StatefulServiceGroup,
                *SchemaNames::Namespace,
                false))
            {
                isStateful = true;
                isServiceGroup = true;
            }
            else if (xmlReader->IsStartElement(
                *SchemaNames::Element_StatelessServiceGroup,
                *SchemaNames::Namespace,
                false))
            {
                isStateful = false;
                isServiceGroup = true;
            }
            else
            {
                done = true;
            }

            if(!done)
            {
                ApplicationServiceDescription serviceDescription;                
                serviceDescription.ReadFromXml(xmlReader, isStateful, isServiceGroup);
                this->ServiceTemplates.push_back(move(serviceDescription));
            }
        }while(!done);

        // </ServiceTemplates>
        xmlReader->ReadEndElement();
    }
}

void ApplicationInstanceDescription::ParseDefaultServices(XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_DefaultServices,
        *SchemaNames::Namespace);

    if(xmlReader->IsEmptyElement())
    {
        // <DefaultServices />
        xmlReader->ReadElement();
    }
    else
    {

        // <DefaultServices>
        xmlReader->ReadStartElement();

        bool done = false;
        do
        {
            if (xmlReader->IsStartElement(*SchemaNames::Element_Service, *SchemaNames::Namespace, false) ||
                xmlReader->IsStartElement(*SchemaNames::Element_ServiceGroup, *SchemaNames::Namespace, false))
            {
                DefaultServiceDescription defaultServiceDescription;                
                defaultServiceDescription.ReadFromXml(xmlReader);
                this->DefaultServices.push_back(move(defaultServiceDescription));
            }
            else
            {
                done = true;
            }
        }while(!done);

        // </DefaultServices>
        xmlReader->ReadEndElement();
    }
}



void ApplicationInstanceDescription::clear()
{
    this->DefaultServices.clear();
    this->ServiceTemplates.clear();    
}
