//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServicePackageContainerPolicyDescription::ServicePackageContainerPolicyDescription()
: PortBindings(), Hostname(), Isolation(ContainerIsolationMode::EnumToString(ContainerIsolationMode::process))
{
}

bool ServicePackageContainerPolicyDescription::operator== (ServicePackageContainerPolicyDescription const & other) const
{
    bool equals = true;

    for (auto i = 0; i != PortBindings.size(); i++)
    {
        equals = this->PortBindings[i] == other.PortBindings[i];
        if (!equals)
        {
            return equals;
        }
    }

    equals = StringUtility::AreEqualCaseInsensitive(this->Hostname, other.Hostname);
    if (!equals)
    {
        return equals;
    }

    equals = StringUtility::AreEqualCaseInsensitive(this->Isolation, other.Isolation);
    if (!equals)
    {
        return equals;
    }

    return equals;
}

bool ServicePackageContainerPolicyDescription::operator!= (ServicePackageContainerPolicyDescription const & other) const
{
    return !(*this == other);
}

void ServicePackageContainerPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServicePackageContainerPolicyDescription { ");
    w.Write("PortBindings {");
    for (auto i = 0; i != PortBindings.size(); i++)
    {
        w.Write("PortBinding {0}", PortBindings[i]);
    }
    w.Write("}");
    w.Write("Hostname = {0}", Hostname);
    w.Write("Isolation = {0}", Isolation);
    w.Write("}");
}

void ServicePackageContainerPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_ServicePackageContainerPolicy,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Hostname))
    {
        this->Hostname = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Hostname);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsolationMode))
    {
        this->Isolation = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsolationMode);
    }

    if (xmlReader->IsEmptyElement())
    {
        // <ServicePackageContainerPolicy />
        xmlReader->ReadElement();
        return;
    }
    xmlReader->MoveToNextElement();
    bool hasPolicies = true;
    while (hasPolicies)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_PortBinding,
            *SchemaNames::Namespace))
        {
            PortBindingDescription description;
            description.ReadFromXml(xmlReader);
            this->PortBindings.push_back(description);
        }
        else
        {
            hasPolicies = false;
        }
    }
    
    // Read the rest of the empty element
    xmlReader->ReadEndElement();
}

Common::ErrorCode ServicePackageContainerPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<ServicePackageContainerPolicy>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServicePackageContainerPolicy, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Hostname, this->Hostname);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_IsolationMode, this->Isolation);
    if (!er.IsSuccess())
    {
        return er;
    }

    for (auto i = 0; i < PortBindings.size(); i++)
    {
        PortBindings[i].WriteToXml(xmlWriter);
    }

    //</ServicePackageContainerPolicy>
    return xmlWriter->WriteEndElement();
}

void ServicePackageContainerPolicyDescription::clear()
{
    this->PortBindings.clear();
    this->Hostname.clear();
    this->Isolation.clear();
}
