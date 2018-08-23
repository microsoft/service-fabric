// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DigestedCodePackageDescription::DigestedCodePackageDescription() :
    RolloutVersionValue(),
    CodePackage(),
    RunAsPolicy(),
    SetupRunAsPolicy(),
    ContentChecksum(),
    IsShared(false),
    DebugParameters(),
    ContainerPolicies(),
    ResourceGovernancePolicy(),
    ConfigPackagePolicy()
{
}

DigestedCodePackageDescription::DigestedCodePackageDescription(DigestedCodePackageDescription const & other) :
    RolloutVersionValue(other.RolloutVersionValue),
    CodePackage(other.CodePackage),
    RunAsPolicy(other.RunAsPolicy),
    SetupRunAsPolicy(other.SetupRunAsPolicy),
    ContentChecksum(other.ContentChecksum),
    IsShared(other.IsShared),
    DebugParameters(other.DebugParameters),
    ContainerPolicies(other.ContainerPolicies),
    ResourceGovernancePolicy(other.ResourceGovernancePolicy),
    ConfigPackagePolicy(other.ConfigPackagePolicy)
{
}

DigestedCodePackageDescription::DigestedCodePackageDescription(DigestedCodePackageDescription && other) :
    RolloutVersionValue(move(other.RolloutVersionValue)),
    CodePackage(move(other.CodePackage)),
    RunAsPolicy(move(other.RunAsPolicy)),
    SetupRunAsPolicy(move(other.SetupRunAsPolicy)),
    ContentChecksum(move(other.ContentChecksum)),
    IsShared(other.IsShared),
    DebugParameters(move(other.DebugParameters)),
    ContainerPolicies(move(other.ContainerPolicies)),
    ResourceGovernancePolicy(move(other.ResourceGovernancePolicy)),
    ConfigPackagePolicy(move(other.ConfigPackagePolicy))
{
}

bool DigestedCodePackageDescription::operator == (DigestedCodePackageDescription const & other) const
{
    bool equals = true;

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }    

    equals = this->CodePackage == other.CodePackage;
    if (!equals) { return equals; }

    equals = this->RunAsPolicy == other.RunAsPolicy;
    if (!equals) { return equals; }

    equals = this->SetupRunAsPolicy == other.SetupRunAsPolicy;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ContentChecksum, other.ContentChecksum);
    if (!equals) { return equals; }

    equals = this->IsShared == other.IsShared;
    if (!equals) { return equals; }

    equals = this->DebugParameters == other.DebugParameters;
    if (!equals) { return equals; }

    equals = this->ContainerPolicies == other.ContainerPolicies;
    if (!equals) { return equals;  }

    equals = this->ResourceGovernancePolicy == other.ResourceGovernancePolicy;
    if (!equals) { return equals; }

    return equals;
}

bool DigestedCodePackageDescription::operator != (DigestedCodePackageDescription const & other) const
{
    return !(*this == other);
}

void DigestedCodePackageDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DigestedCodePackageDescription { ");
    w.Write("RolloutVersion = {0}, ", RolloutVersionValue);

    w.Write("CodePackage = {");
    w.Write("{0}", CodePackage);
    w.Write("}, ");

    w.Write("RunAsPolicy = {");
    w.Write("{0}", RunAsPolicy);
    w.Write("}, ");

    w.Write("SetupRunAsPolicy = {");
    w.Write("{0}", SetupRunAsPolicy);
    w.Write("}, ");

    w.Write("ContentChecksum = {0}", ContentChecksum);

    w.Write("IsShared = {0}", IsShared);
    w.Write("DebugParameters = {0}", DebugParameters);
    w.Write("ContainerPolicies = {0}", ContainerPolicies);
    w.Write("ResourceGovernancePolicy = {0}", ResourceGovernancePolicy);
    w.Write("ConfigPackagePolicies = {0}", ConfigPackagePolicy);
    w.Write("}");
}

void DigestedCodePackageDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DigestedCodePackage RolloutVersion=""
    xmlReader->StartElement(
        *SchemaNames::Element_DigestedCodePackage,
        *SchemaNames::Namespace);


    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);    
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("Format of RolloutVersion is invalid. RolloutVersion: {0}", rolloutVersionString);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_ContentChecksum))
    {
        this->ContentChecksum = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ContentChecksum);
    }
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_IsShared))
    {
        wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsShared);
        if (!StringUtility::TryFromWString<bool>(
            attrValue, 
            this->IsShared))
        {
            Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
        }
    }
    xmlReader->ReadStartElement();

    // <CodePackage ../>
    this->CodePackage.ReadFromXml(xmlReader);

    ParseRunasPolicies(xmlReader);
    ParseDebugParameters(xmlReader);
    ParseContainerPolicies(xmlReader);
    ParseResourceGovernancePolicy(xmlReader);
    ParseConfigPackagePolicy(xmlReader);
    // </DigestedCodePackage>
    xmlReader->ReadEndElement();
}

ErrorCode DigestedCodePackageDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{ //<DigestedCodePackage>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedCodePackage, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ContentChecksum, this->ContentChecksum);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsShared,
        this->IsShared);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = CodePackage.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = SetupRunAsPolicy.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = RunAsPolicy.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = DebugParameters.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = ContainerPolicies.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = ResourceGovernancePolicy.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = ConfigPackagePolicy.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    return xmlWriter->WriteEndElement();
}

void DigestedCodePackageDescription::clear()
{        
    this->CodePackage.clear();
    this->RunAsPolicy.clear();
    this->SetupRunAsPolicy.clear();
    this->DebugParameters.clear();
    this->ContainerPolicies.clear();
    this->ResourceGovernancePolicy.clear();
    this->ConfigPackagePolicy.clear();
}

void DigestedCodePackageDescription::ParseRunasPolicies(XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    do
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_RunAsPolicy,
            *SchemaNames::Namespace,
            false))
        {
            RunAsPolicyDescription runasPolicy;
            runasPolicy.ReadFromXml(xmlReader);

            if(runasPolicy.EntryPointType == RunAsPolicyTypeEntryPointType::All ||
                runasPolicy.EntryPointType == RunAsPolicyTypeEntryPointType::Main)
            {
                this->RunAsPolicy = runasPolicy;
            }

            if(runasPolicy.EntryPointType == RunAsPolicyTypeEntryPointType::All ||
                runasPolicy.EntryPointType == RunAsPolicyTypeEntryPointType::Setup)
            {
                this->SetupRunAsPolicy = runasPolicy;
            }
        }

        else
        {
            done = true;
        }
    }while(!done);
}

void DigestedCodePackageDescription::ParseDebugParameters(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_DebugParameters,
        *SchemaNames::Namespace,
        false))
    {
        this->DebugParameters.ReadFromXml(xmlReader);
    }
}

void DigestedCodePackageDescription::ParseContainerPolicies(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ContainerHostPolicies,
        *SchemaNames::Namespace,
        false))
    {
        this->ContainerPolicies.ReadFromXml(xmlReader);
    }
}

void DigestedCodePackageDescription::ParseResourceGovernancePolicy(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ResourceGovernancePolicy,
        *SchemaNames::Namespace))
    {
        this->ResourceGovernancePolicy.ReadFromXml(xmlReader);
    }
}

void DigestedCodePackageDescription::ParseConfigPackagePolicy(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ConfigPackagePolicies,
        *SchemaNames::Namespace))
    {
        this->ConfigPackagePolicy.ReadFromXml(xmlReader);
    }
}
