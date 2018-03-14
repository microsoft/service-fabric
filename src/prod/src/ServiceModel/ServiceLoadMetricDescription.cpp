// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceLoadMetricDescription::ServiceLoadMetricDescription() :
    PrimaryDefaultLoad(),
	SecondaryDefaultLoad(),
    Name(),
    Weight(WeightType::Low)
{
}

ServiceLoadMetricDescription::ServiceLoadMetricDescription(wstring name, 
            WeightType::Enum weight,
            uint primaryDefaultLoad,
            uint secondaryDefaultLoad) :
    PrimaryDefaultLoad(primaryDefaultLoad),
    SecondaryDefaultLoad(secondaryDefaultLoad),
	Name(name),
    Weight(weight)

{
}

ServiceLoadMetricDescription::ServiceLoadMetricDescription(ServiceLoadMetricDescription const & other) :
    PrimaryDefaultLoad(other.PrimaryDefaultLoad),
	SecondaryDefaultLoad(other.SecondaryDefaultLoad),
    Name(other.Name),
    Weight(other.Weight)
{
}

ServiceLoadMetricDescription::ServiceLoadMetricDescription(ServiceLoadMetricDescription && other) :
    PrimaryDefaultLoad(other.PrimaryDefaultLoad),
	SecondaryDefaultLoad(other.SecondaryDefaultLoad),
    Name(move(other.Name)),
    Weight(move(other.Weight))
{
}

ServiceLoadMetricDescription const & ServiceLoadMetricDescription::operator = (ServiceLoadMetricDescription const & other)
{
    if (this != & other)
    {
        this->PrimaryDefaultLoad = other.PrimaryDefaultLoad;
        this->SecondaryDefaultLoad = other.SecondaryDefaultLoad;
        this->Name = other.Name;
        this->Weight = other.Weight;
    }

    return *this;
}

ServiceLoadMetricDescription const & ServiceLoadMetricDescription::operator = (ServiceLoadMetricDescription && other)
{
    if (this != & other)
    {
        this->PrimaryDefaultLoad = other.PrimaryDefaultLoad;
        this->SecondaryDefaultLoad = other.SecondaryDefaultLoad;
        this->Name = move(other.Name);
        this->Weight = move(other.Weight);
    }

    return *this;
}

bool ServiceLoadMetricDescription::operator == (ServiceLoadMetricDescription const & other) const
{
    bool equals = true;

    equals = this->PrimaryDefaultLoad == other.PrimaryDefaultLoad && this->SecondaryDefaultLoad == other.SecondaryDefaultLoad;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = this->Weight == other.Weight;
    if (!equals) { return equals; }

    return equals;
}

bool ServiceLoadMetricDescription::operator != (ServiceLoadMetricDescription const & other) const
{
    return !(*this == other);
}

void ServiceLoadMetricDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceLoadMetricDescription { ");
    w.Write("PrimaryDefaultLoad = {0}, ", PrimaryDefaultLoad);
    w.Write("SecondaryDefaultLoad = {0}, ", SecondaryDefaultLoad);
    w.Write("Name = {0}", Name);
    w.Write("Weight = {0}", Weight);
    w.Write("}");
}

void ServiceLoadMetricDescription::ReadFromXml(Common::XmlReaderUPtr const & xmlReader)   
{
    this->clear();
    // ensure that we are on <LoadMetric
    xmlReader->StartElement(
        *SchemaNames::Element_LoadMetric,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_PrimaryDefaultLoad))
    {
        wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PrimaryDefaultLoad);
        if (!StringUtility::TryFromWString<uint>(
            attrValue, 
            this->PrimaryDefaultLoad))
        {
            Parser::ThrowInvalidContent(xmlReader, L"uint", attrValue);
        }
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_DefaultLoad))
    {
        wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DefaultLoad);
        if (!StringUtility::TryFromWString<uint>(
            attrValue, 
            this->PrimaryDefaultLoad))
        {
            Parser::ThrowInvalidContent(xmlReader, L"uint", attrValue);
        }
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_SecondaryDefaultLoad))
    {
        wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_SecondaryDefaultLoad);
        if (!StringUtility::TryFromWString<uint>(
            attrValue, 
            this->SecondaryDefaultLoad))
        {
            Parser::ThrowInvalidContent(xmlReader, L"uint", attrValue);
        }
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_Weight))
    {
        wstring weight = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Weight);
        this->Weight = WeightType::GetWeightType(weight);

    }

    xmlReader->ReadElement();
}
ErrorCode ServiceLoadMetricDescription::WriteToXml(XmlWriterUPtr const & xmlWriter){
	//<LoadMetric>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_LoadMetric, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_DefaultLoad, this->PrimaryDefaultLoad);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_PrimaryDefaultLoad, this->PrimaryDefaultLoad);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_SecondaryDefaultLoad, this->SecondaryDefaultLoad);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Weight, WeightType::ToString(Weight));
	if (!er.IsSuccess())
	{
		return er;
	}
	//</LoadMetric>
	return er = xmlWriter->WriteEndElement();
}

void ServiceLoadMetricDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION & publicDescription) const
{
    publicDescription.Name = heap.AddString(this->Name);    
    publicDescription.Weight = WeightType::ToPublicApi(this->Weight);
    publicDescription.PrimaryDefaultLoad = this->PrimaryDefaultLoad;
    publicDescription.SecondaryDefaultLoad = this->SecondaryDefaultLoad;
}

Common::ErrorCode ServiceLoadMetricDescription::FromPublicApi(__in FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION const & publicDescription)
{
    this->Name = publicDescription.Name;
    this->Weight = WeightType::FromPublicApi(publicDescription.Weight);
    this->PrimaryDefaultLoad = publicDescription.PrimaryDefaultLoad;
    this->SecondaryDefaultLoad = publicDescription.SecondaryDefaultLoad;

    return ErrorCode(ErrorCodeValue::Success);
}

void ServiceLoadMetricDescription::clear()
{
    this->Name.clear();
}

std::wstring ServiceLoadMetricDescription::ToString() const
{
    return wformatString("Name = '{0}'; Weight = '{1}'; PrimaryDefaultLoad = '{2}'; SecondaryDefaultLoad = '{3}'", 
        Name, 
        static_cast<uint>(Weight),
        PrimaryDefaultLoad,
        SecondaryDefaultLoad); 
}
