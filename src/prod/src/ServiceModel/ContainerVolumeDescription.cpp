// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerVolumeDescription::ContainerVolumeDescription()
	: Source(),
	Destination(),
    Driver(),
    IsReadOnly(false),
    DriverOpts()
{
}

bool ContainerVolumeDescription::operator== (ContainerVolumeDescription const & other) const
{
    return (StringUtility::AreEqualCaseInsensitive(Source, other.Source) &&
        StringUtility::AreEqualCaseInsensitive(Destination, other.Destination) &&
        StringUtility::AreEqualCaseInsensitive(Driver, other.Driver) &&
        IsReadOnly == other.IsReadOnly);
}

bool ContainerVolumeDescription::operator!= (ContainerVolumeDescription const & other) const
{
    return !(*this == other);
}

void ContainerVolumeDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerVolumeDescription { ");
    w.Write("Source = {0}, ", Source);
    w.Write("Destination = {0}, ", Destination);
    w.Write("Driver = {0}, ", Driver);
    w.Write("IsReadOnly = {0}, ", IsReadOnly);
    w.Write("DriverOpts {");
    for (auto i = 0; i < DriverOpts.size(); i++)
    {
        w.Write("DriverOpt = {0}", DriverOpts[i]);
    }
    w.Write("}");
}

void ContainerVolumeDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_Volume,
        *SchemaNames::Namespace);

    this->Source = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Source);
    this->Destination = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Destination);
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Driver))
    {
        this->Driver = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Driver);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsReadOnly))
    {
        this->IsReadOnly = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_IsReadOnly);
    }
    if (xmlReader->IsEmptyElement())
    {
        // <Volume />
        xmlReader->ReadElement();
        return;
    }
    xmlReader->MoveToNextElement();
    bool hasPolicies = true;
    while (hasPolicies)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_DriverOption,
            *SchemaNames::Namespace))
        {
            DriverOptionDescription driverOpt;
            driverOpt.ReadFromXml(xmlReader);
            DriverOpts.push_back(driverOpt);
        }
        else
        {
            hasPolicies = false;
        }
    }
    // Read the rest of the empty element
    xmlReader->ReadEndElement();
}

Common::ErrorCode ContainerVolumeDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<ContainerVolume>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Volume, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Source, this->Source);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Destination, this->Destination);
	if (!er.IsSuccess())
	{
		return er;
	}
    if (!this->Driver.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Driver, this->Driver);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsReadOnly, this->IsReadOnly);
    if (!er.IsSuccess())
    {
        return er;
    }
    for (auto i = 0; i < DriverOpts.size(); i++)
    {
        er = DriverOpts[i].WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
	//</ContainerVolume>
	return xmlWriter->WriteEndElement();
}

void ContainerVolumeDescription::clear()
{
    this->Source.clear();
    this->Destination.clear();
    this->Driver.clear();
    this->DriverOpts.clear();
}

ErrorCode ContainerVolumeDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_VOLUME_DESCRIPTION & fabricDriverOptionDesc) const
{
    fabricDriverOptionDesc.Source = heap.AddString(this->Source);
    fabricDriverOptionDesc.Destination = heap.AddString(this->Destination);
    fabricDriverOptionDesc.Driver = heap.AddString(this->Driver);
    fabricDriverOptionDesc.IsReadOnly = this->IsReadOnly;

    fabricDriverOptionDesc.DriverOpts = nullptr;
    fabricDriverOptionDesc.Reserved = nullptr;

    auto driverOptions = heap.AddItem<FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION_LIST>();

    auto error = PublicApiHelper::ToPublicApiList<
        DriverOptionDescription,
        FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION,
        FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION_LIST>(heap, this->DriverOpts, *driverOptions);
    if (!error.IsSuccess())
    {
        return error;
    }

    fabricDriverOptionDesc.DriverOpts = driverOptions.GetRawPointer();

    return ErrorCode::Success();
}

