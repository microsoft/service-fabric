// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceDiagnosticsDescription::ServiceDiagnosticsDescription() 
    : EtwDescription()
{
}

ServiceDiagnosticsDescription::ServiceDiagnosticsDescription(
    ServiceDiagnosticsDescription const & other)
    : EtwDescription(other.EtwDescription)
{
}

ServiceDiagnosticsDescription::ServiceDiagnosticsDescription(ServiceDiagnosticsDescription && other) 
    : EtwDescription(move(other.EtwDescription))
{
}

ServiceDiagnosticsDescription const & ServiceDiagnosticsDescription::operator = (ServiceDiagnosticsDescription const & other)
{
    if (this != &other)
    {
        this->EtwDescription = other.EtwDescription;
    }
    return *this;
}

ServiceDiagnosticsDescription const & ServiceDiagnosticsDescription::operator = (ServiceDiagnosticsDescription && other)
{
    if (this != &other)
    {
        this->EtwDescription = move(other.EtwDescription);
    }
    return *this;
}

bool ServiceDiagnosticsDescription::operator == (ServiceDiagnosticsDescription const & other) const
{
    return this->EtwDescription == other.EtwDescription;
}

bool ServiceDiagnosticsDescription::operator != (ServiceDiagnosticsDescription const & other) const
{
    return !(*this == other);
}

void ServiceDiagnosticsDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceDiagnosticsDescription { ");

    w.Write(this->EtwDescription);

    w.Write("}");
}

void ServiceDiagnosticsDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // <Diagnostics>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Diagnostics,
        *SchemaNames::Namespace))
    {
        xmlReader->StartElement(
            *SchemaNames::Element_Diagnostics,
            *SchemaNames::Namespace,
            false);
        if (xmlReader->IsEmptyElement())
        {
            // <Diagnostics />
            xmlReader->ReadElement();
            return;
        }

        xmlReader->ReadStartElement();

        // <ETW>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ETW,
            *SchemaNames::Namespace))
        {
            this->EtwDescription.ReadFromXml(xmlReader);
        }

        // </Diagnostics>
        xmlReader->ReadEndElement();
    }
}

ErrorCode ServiceDiagnosticsDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Diagnostics, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}

	er = (this->EtwDescription).WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->WriteEndElement();
}

void ServiceDiagnosticsDescription::clear()
{
    this->EtwDescription.clear();
}
