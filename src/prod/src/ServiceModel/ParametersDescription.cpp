// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ParametersDescription::ParametersDescription() 
    : Parameter()
{
}

ParametersDescription::ParametersDescription(
    ParametersDescription const & other)
    : Parameter(other.Parameter)
{
}

ParametersDescription::ParametersDescription(ParametersDescription && other) 
    : Parameter(move(other.Parameter))
{
}

ParametersDescription const & ParametersDescription::operator = (ParametersDescription const & other)
{
    if (this != &other)
    {
        this->Parameter = other.Parameter;
    }
    return *this;
}

ParametersDescription const & ParametersDescription::operator = (ParametersDescription && other)
{
    if (this != &other)
    {
        this->Parameter = move(other.Parameter);
    }
    return *this;
}

bool ParametersDescription::operator == (ParametersDescription const & other) const
{
    bool equals = true;

    equals = this->Parameter.size() == other.Parameter.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->Parameter.size(); ++ix)
    {
        equals = this->Parameter[ix] == other.Parameter[ix];
        if (!equals) { return equals; }
    }
    
    return equals;
}

bool ParametersDescription::operator != (ParametersDescription const & other) const
{
    return !(*this == other);
}

void ParametersDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ParametersDescription { ");
    w.Write(this->Parameter);
    w.Write("}");
}

void ParametersDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_Parameters,
        *SchemaNames::Namespace,
        false);

    if (xmlReader->IsEmptyElement())
    {
        // <Parameters />
        xmlReader->ReadElement();
        return;
    }

    xmlReader->ReadStartElement();

    // <Parameter>
    bool parametersAvailable = true;
    while (parametersAvailable)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_Parameter,
            *SchemaNames::Namespace))
        {
            ParameterDescription parameter;
            parameter.ReadFromXml(xmlReader);
            this->Parameter.push_back(move(parameter));
        }
        else
        {
            parametersAvailable = false;
        }
    }

    // </Parameters>
    xmlReader->ReadEndElement();
}

ErrorCode ParametersDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
	if (this->Parameter.empty())
	{
		return ErrorCodeValue::Success;
	}
	//<Parameters>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Parameters, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < Parameter.size(); ++i)
	{
		er = Parameter[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//</Parameters>
	return xmlWriter->WriteEndElement();
}
void ParametersDescription::clear()
{
    this->Parameter.clear();
}

bool ParametersDescription::empty()
{
	return this->Parameter.empty();
}
