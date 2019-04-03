// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ConfigSectionOverride::ConfigSectionOverride()
    : Name(),
    Parameters()
{
}

ConfigSectionOverride::ConfigSectionOverride(ConfigSectionOverride const & other)
    : Name(other.Name),
    Parameters(other.Parameters)
{
}

ConfigSectionOverride::ConfigSectionOverride(ConfigSectionOverride && other)
    : Name(move(other.Name)),
    Parameters(move(other.Parameters))
{
}

ConfigSectionOverride const & ConfigSectionOverride::operator = (ConfigSectionOverride const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Parameters = other.Parameters;
    }

    return *this;
}

ConfigSectionOverride const & ConfigSectionOverride::operator = (ConfigSectionOverride && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->Parameters = move(other.Parameters);
    }

    return *this;
}

bool ConfigSectionOverride::operator == (ConfigSectionOverride const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = this->Parameters.size() == other.Parameters.size();
    if (!equals) { return equals; }

    for(auto iter1 = this->Parameters.cbegin(); iter1 != this->Parameters.cend(); ++iter1)
    {
        auto iter2 = other.Parameters.find(iter1->first);
        if (iter2 == other.Parameters.end()) { return false; }

        equals = iter1->second == iter2->second;
        if (!equals) { return equals; }
    }

    return equals;
}

bool ConfigSectionOverride::operator != (ConfigSectionOverride const & other) const
{
    return !(*this == other);
}

void ConfigSectionOverride::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigSectionOverride {");
    w.Write("Name = {0},", Name);

    w.Write("Parameters = {");
    for(auto iter = Parameters.cbegin(); iter != Parameters.cend(); ++ iter)
    {
        w.WriteLine("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void ConfigSectionOverride::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are positioned on <ConfigSection 
    xmlReader->StartElement(
        *SchemaNames::Element_ConfigSection, 
        *SchemaNames::Namespace);

    // Section Name=""
    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    if (xmlReader->IsEmptyElement())
    {
        // <Section ... />
        xmlReader->ReadElement();
    }
    else
    {
        // <Section ...>
        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            // <Parameter ... >
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_ConfigParameter,
                *SchemaNames::Namespace,
                false))
            {
                ConfigParameterOverride parameter;
                parameter.ReadFromXml(xmlReader);
                wstring parameterName(parameter.Name);

                if (!TryAddParameter(move(parameter)))
                {
                    Trace.WriteError(
                        "Common",
                        L"XMlParser",
                        "Parameter {0} with a different name than existing parameters in the section. Input={1}, Line={2}, Position={3}.",
                        parameterName,
                        xmlReader->FileName,
                        xmlReader->GetLineNumber(),
                        xmlReader->GetLinePosition());

                    throw XmlException(ErrorCode(ErrorCodeValue::XmlInvalidContent));
                }
            }
            else
            {
                done = true;
            }
        }

        // </Section>
        xmlReader->ReadEndElement();
    }
}

ErrorCode ConfigSectionOverride::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<Section>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigSection, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto it = Parameters.begin();
		it != Parameters.end(); ++it)
	{
		er = (*it).second.WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</Section>
	return xmlWriter->WriteEndElement();
}

bool ConfigSectionOverride::TryAddParameter(ConfigParameterOverride && parameter)
{
    wstring parameterName(parameter.Name);

    auto iter = this->Parameters.find(parameterName);
    if (iter != this->Parameters.end())
    {
        return false;   
    }
    else
    {
        this->Parameters.insert(make_pair(move(parameterName), move(parameter)));
        return true;
    }
}

void ConfigSectionOverride::clear()
{
    this->Name.clear();
    this->Parameters.clear();
}
