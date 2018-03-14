// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

LogConfigDescription::LogConfigDescription()
    : Driver(),
    DriverOpts()
{
}

bool LogConfigDescription::operator== (LogConfigDescription const & other) const
{
    return StringUtility::AreEqualCaseInsensitive(Driver, other.Driver);
}

bool LogConfigDescription::operator!= (LogConfigDescription const & other) const
{
    return !(*this == other);
}

void LogConfigDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("LogConfigDescription { ");
    w.Write("Driver = {0}, ", Driver);
    w.Write("DriverOpts {");
    for (auto i = 0; i < DriverOpts.size(); i++)
    {
        w.Write("DriverOpt = {0}", DriverOpts[i]);
    }
 
    w.Write("}");
}

void LogConfigDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_LogConfig,
        *SchemaNames::Namespace);

    this->Driver = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Driver);
    if (xmlReader->IsEmptyElement())
    {
        // <LogConfig />
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

Common::ErrorCode LogConfigDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<LogConfig>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_LogConfig, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Driver, this->Driver);
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
    //</LogConfig>
    return xmlWriter->WriteEndElement();
}

void LogConfigDescription::clear()
{
    this->Driver.clear();
    this->DriverOpts.clear();
}

ErrorCode LogConfigDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_LOG_CONFIG_DESCRIPTION & fabricLogConfigDesc) const
{
    fabricLogConfigDesc.Driver = heap.AddString(this->Driver);

    auto driverOptions = heap.AddItem<FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION_LIST>();

    auto error = PublicApiHelper::ToPublicApiList<
        DriverOptionDescription,
        FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION,
        FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION_LIST>(heap, this->DriverOpts, *driverOptions);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricLogConfigDesc.DriverOpts = driverOptions.GetRawPointer();

    fabricLogConfigDesc.Reserved = nullptr;

    return ErrorCode::Success();
}

