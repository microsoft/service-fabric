// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ConfigPackagePoliciesDescription::ConfigPackagePoliciesDescription()
: ConfigPackages(),
CodePackageRef()
{
}

bool ConfigPackagePoliciesDescription::operator== (ConfigPackagePoliciesDescription const & other) const
{
    bool equals = true;
    equals = CodePackageRef == other.CodePackageRef;
    if (!equals)
    {
        return equals;
    }

    for (auto i = 0; i != ConfigPackages.size(); i++)
    {
        equals = this->ConfigPackages[i] == other.ConfigPackages[i];
        if (!equals)
        {
            return equals;
        }
    }

    return equals;
}

bool ConfigPackagePoliciesDescription::operator!= (ConfigPackagePoliciesDescription const & other) const
{
    return !(*this == other);
}

void ConfigPackagePoliciesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigPackagePoliciesDescription { ");
    w.Write("CodePackageRef = {0}", CodePackageRef);
    w.Write("ConfigPackages {");
    for (auto i = 0; i != ConfigPackages.size(); i++)
    {
        w.Write("ConfigPackages = {0}", ConfigPackages[i]);
    }
    w.Write("}");
}

void ConfigPackagePoliciesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_ConfigPackagePolicies,
        *SchemaNames::Namespace);
   
    if (xmlReader->IsEmptyElement())
    {
        // <ConfigPackagePolicies />
        xmlReader->ReadElement();
        return;
    }

    this->CodePackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CodePackageRef);

    xmlReader->ReadStartElement();

    bool hasConfigPackages = true;
    while (hasConfigPackages)
    {
        if(xmlReader->IsStartElement(
            *SchemaNames::Element_ConfigPackage,
            *SchemaNames::Namespace))
        {
            ConfigPackageSettingDescription configPackage;
            configPackage.ReadFromXml(xmlReader);
            this->ConfigPackages.push_back(move(configPackage));
        }
        else
        {
            hasConfigPackages = false;
        }
    }
    
    // Read the rest of the empty element
    xmlReader->ReadEndElement();
}

Common::ErrorCode ConfigPackagePoliciesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<ConfigPackagePolicies>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigPackagePolicies, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CodePackageRef, this->CodePackageRef);
    if (!er.IsSuccess())
    {
        return er;
    }

    for (auto i = 0; i < this->ConfigPackages.size(); ++i)
    {
        er = ConfigPackages[i].WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    //</ConfigPackagePolicies>
    return xmlWriter->WriteEndElement();
}

void ConfigPackagePoliciesDescription::clear()
{
    this->CodePackageRef.clear();
    this->ConfigPackages.clear();
}
