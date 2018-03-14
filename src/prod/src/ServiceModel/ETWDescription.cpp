// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ETWDescription::ETWDescription() 
    : ProviderGuids(),
    ManifestDataPackages()
{
}

ETWDescription::ETWDescription(
    ETWDescription const & other)
    : ProviderGuids(other.ProviderGuids),
    ManifestDataPackages(other.ManifestDataPackages)
{
}

ETWDescription::ETWDescription(ETWDescription && other) 
    : ProviderGuids(move(other.ProviderGuids)),
    ManifestDataPackages(move(other.ManifestDataPackages))
{
}

ETWDescription const & ETWDescription::operator = (ETWDescription const & other)
{
    if (this != &other)
    {
        this->ProviderGuids = other.ProviderGuids;
        this->ManifestDataPackages = other.ManifestDataPackages;
    }
    return *this;
}

ETWDescription const & ETWDescription::operator = (ETWDescription && other)
{
    if (this != &other)
    {
        this->ProviderGuids = move(other.ProviderGuids);
        this->ManifestDataPackages = move(other.ManifestDataPackages);
    }
    return *this;
}

bool ETWDescription::operator == (ETWDescription const & other) const
{
    bool equals = true;

    equals = this->ProviderGuids.size() == other.ProviderGuids.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->ProviderGuids.size(); ++ix)
    {
        equals = this->ProviderGuids[ix] == other.ProviderGuids[ix];
        if (!equals) { return equals; }
    }

    equals = this->ManifestDataPackages.size() == other.ManifestDataPackages.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->ManifestDataPackages.size(); ++ix)
    {
        equals = this->ManifestDataPackages[ix] == other.ManifestDataPackages[ix];
        if (!equals) { return equals; }
    }

    return equals;
}

bool ETWDescription::operator != (ETWDescription const & other) const
{
    return !(*this == other);
}

void ETWDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ETWDescription { ");

    w.Write("ProviderGuids = {");
    for (size_t ix = 0; ix < this->ProviderGuids.size(); ++ix)
    {
        w.Write(this->ProviderGuids[ix]);
    }
    w.Write("}, ");

    w.Write("ManifestDataPackages = {");
    for (size_t ix = 0; ix < this->ManifestDataPackages.size(); ++ix)
    {
        w.Write(this->ManifestDataPackages[ix]);
    }
    w.Write("}, ");

    w.Write("}");
}

void ETWDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // <ETW>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ETW,
        *SchemaNames::Namespace))
    {
        xmlReader->StartElement(
            *SchemaNames::Element_ETW,
            *SchemaNames::Namespace,
            false);
        if (xmlReader->IsEmptyElement())
        {
            // <ETW />
            xmlReader->ReadElement();
            return;
        }

        xmlReader->ReadStartElement();

        // <ProviderGuids>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ProviderGuids,
            *SchemaNames::Namespace))
        {
            xmlReader->StartElement(
                *SchemaNames::Element_ProviderGuids,
                *SchemaNames::Namespace,
                false);
            if (xmlReader->IsEmptyElement())
            {
                // <ProviderGuids />
                xmlReader->ReadElement();
            }
            else
            {
                xmlReader->ReadStartElement();

                // <ProviderGuid Value="" />
                bool guidsAvailable = true;
                while (guidsAvailable)
                {
                    if (xmlReader->IsStartElement(
                        *SchemaNames::Element_ProviderGuid,
                        *SchemaNames::Namespace))
                    {
                        if (xmlReader->HasAttribute(*SchemaNames::Attribute_Value))
                        {
                            std::wstring providerGuid = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Value);
                            this->ProviderGuids.push_back(move(providerGuid));
                        }
                        xmlReader->ReadElement();
                    }
                    else
                    {
                        guidsAvailable = false;
                    }
                }

                // </ProviderGuids>
                xmlReader->ReadEndElement();
            }
        }

        // <ManifestDataPackages>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ManifestDataPackages,
            *SchemaNames::Namespace))
        {
            xmlReader->StartElement(
                *SchemaNames::Element_ManifestDataPackages,
                *SchemaNames::Namespace,
                false);

            if (xmlReader->IsEmptyElement())
            {
                // <ManifestDataPackages />
                xmlReader->ReadElement();
            }
            else
            {
                xmlReader->ReadStartElement();

                // <ManifestDataPackage>
                bool dataPackagesAvailable = true;
                while(dataPackagesAvailable)
                {
                    if (xmlReader->IsStartElement(
                        *SchemaNames::Element_ManifestDataPackage,
                        *SchemaNames::Namespace,
                        false))
                    {
                        xmlReader->StartElement(
                            *SchemaNames::Element_ManifestDataPackage,
                            *SchemaNames::Namespace);

                        DataPackageDescription dataPackage;
                        dataPackage.Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
                        dataPackage.Version = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Version);
                        xmlReader->ReadElement();

                        this->ManifestDataPackages.push_back(move(dataPackage));
                    }
                    else
                    {
                        dataPackagesAvailable = false;
                    }
                }
            }
        }

        // </ETW>
        xmlReader->ReadEndElement();
    }
}
bool ETWDescription::isEmpty()
{
	return (this->ProviderGuids).empty() && (this->ManifestDataPackages).empty();
}

ErrorCode ETWDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ETW, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteProviderGuids(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteManifestDataPackages(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->WriteEndElement();
}

ErrorCode ETWDescription::WriteProviderGuids(XmlWriterUPtr const & xmlWriter)
{
	if (ProviderGuids.empty())
	{
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ProviderGuids, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < ProviderGuids.size(); ++i)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_ProviderGuid, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Value, ProviderGuids[i]);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return xmlWriter->WriteEndElement();
}

ErrorCode ETWDescription::WriteManifestDataPackages(XmlWriterUPtr const & xmlWriter)
{
	if (ManifestDataPackages.empty())
	{
		return ErrorCodeValue::Success;
	}

	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ManifestDataPackages, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < ManifestDataPackages.size(); ++i)
	{
		er = (ManifestDataPackages[i]).WriteToXml(xmlWriter, true);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return xmlWriter->WriteEndElement();

}

void ETWDescription::clear()
{
    this->ProviderGuids.clear();
    this->ManifestDataPackages.clear();
}
