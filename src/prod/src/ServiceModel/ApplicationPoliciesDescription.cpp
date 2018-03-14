// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationPoliciesDescription::ApplicationPoliciesDescription()
    : DefaultRunAs(),
    LogCollectionEntries(),
    HealthPolicy(),
    SecurityAccessPolicies()
{
}

ApplicationPoliciesDescription::ApplicationPoliciesDescription(ApplicationPoliciesDescription const & other)
    : DefaultRunAs(other.DefaultRunAs),
    LogCollectionEntries(other.LogCollectionEntries),
    HealthPolicy(other.HealthPolicy),
    SecurityAccessPolicies(other.SecurityAccessPolicies)
{
}

ApplicationPoliciesDescription::ApplicationPoliciesDescription(ApplicationPoliciesDescription && other)
    : DefaultRunAs(move(other.DefaultRunAs)),
    LogCollectionEntries(move(other.LogCollectionEntries)),
    HealthPolicy(move(other.HealthPolicy)),
    SecurityAccessPolicies(move(other.SecurityAccessPolicies))
{
}

ApplicationPoliciesDescription const & ApplicationPoliciesDescription::operator = (ApplicationPoliciesDescription const & other)
{
    if (this != &other)
    {
        this->DefaultRunAs = other.DefaultRunAs;
        this->LogCollectionEntries = other.LogCollectionEntries;
        this->HealthPolicy = other.HealthPolicy;
        this->SecurityAccessPolicies = other.SecurityAccessPolicies;
    }

    return *this;
}

ApplicationPoliciesDescription const & ApplicationPoliciesDescription::operator = (ApplicationPoliciesDescription && other)
{
    if (this != &other)
    {
        this->DefaultRunAs = move(other.DefaultRunAs);
        this->LogCollectionEntries = move(other.LogCollectionEntries);
        this->HealthPolicy = move(other.HealthPolicy);
        this->SecurityAccessPolicies = move(other.SecurityAccessPolicies);
    }

    return *this;
}

bool ApplicationPoliciesDescription::operator == (ApplicationPoliciesDescription const & other) const
{
    bool equals = true;

    equals = this->LogCollectionEntries.size() == other.LogCollectionEntries.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->LogCollectionEntries.size(); ++ix)
    {
        equals = this->LogCollectionEntries[ix] == other.LogCollectionEntries[ix];
        if (!equals) { return equals; }
    }

    equals = this->DefaultRunAs == other.DefaultRunAs;
    if (!equals) { return equals; }

    equals = (this->HealthPolicy == other.HealthPolicy);

    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->SecurityAccessPolicies.size(); ++ix)
    {
        equals = this->SecurityAccessPolicies[ix] == other.SecurityAccessPolicies[ix];
        if (!equals) { return equals; }
    }

    return equals;
}

bool ApplicationPoliciesDescription::operator != (ApplicationPoliciesDescription const & other) const
{
    return !(*this == other);
}

void ApplicationPoliciesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationPoliciesDescription { ");

    w.Write("DefaultRunAs = {");
    w.Write(DefaultRunAs);
    w.Write("}, ");

    w.Write("LogCollectionEntries = {");
    for(auto iter = LogCollectionEntries.cbegin(); iter != LogCollectionEntries.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}, ");

    w.Write("HealthPolicy = {");
    w.Write(HealthPolicy);
    w.Write("}");

    w.Write("SecurityAccessPolices = {");
    for(auto iter = SecurityAccessPolicies.cbegin(); iter != SecurityAccessPolicies.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}, ");

    w.Write("}");
}

void ApplicationPoliciesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <Policies>
    xmlReader->StartElement(
        *SchemaNames::Element_Policies,
        *SchemaNames::Namespace);

    if (xmlReader->IsEmptyElement())
    {
        // <Policies />
        xmlReader->ReadElement();
    }
    else
    {
        bool readPolicies = false;
        xmlReader->ReadStartElement();
        do
        {

            // <LogCollectionPolicies ..>
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_LogCollectionPolicies,
                *SchemaNames::Namespace))
            {
                xmlReader->ReadStartElement();
                bool done = false;
                while(!done)
                {
                    done = true;

                    if (xmlReader->IsStartElement(
                        *SchemaNames::Element_LogCollectionPolicy,
                        *SchemaNames::Namespace))
                    {
                        LogCollectionPolicyDescription logCollectionPolicy;
                        logCollectionPolicy.ReadFromXml(xmlReader);
                        this->LogCollectionEntries.push_back(move(logCollectionPolicy));
                        done = false;
                    }
                }

                xmlReader->ReadEndElement();
            }
            // <DefaultRunAsPolicy ..>
            else if (xmlReader->IsStartElement(
                *SchemaNames::Element_DefaultRunAsPolicy,
                *SchemaNames::Namespace))
            {
                this->DefaultRunAs.ReadFromXml(xmlReader);
            }
            // <HealthPolicy ..>
            else if (xmlReader->IsStartElement(
                *SchemaNames::Element_HealthPolicy,
                *SchemaNames::Namespace))
            {
                this->HealthPolicy.ReadFromXml(xmlReader);
            }
            //<SecurityAccessPolicies>
            else if(xmlReader->IsStartElement(
                 *SchemaNames::Element_SecurityAccessPolicies,
                *SchemaNames::Namespace))
            {
                xmlReader->ReadStartElement();
                bool done = false;
                while(!done)
                {
                    done = true;

                    if (xmlReader->IsStartElement(
                        *SchemaNames::Element_SecurityAccessPolicy,
                        *SchemaNames::Namespace))
                    {
                        SecurityAccessPolicyDescription accessPolicy;
                        accessPolicy.ReadFromXml(xmlReader);
                        this->SecurityAccessPolicies.push_back(move(accessPolicy));
                        done = false;
                    }
                }

                xmlReader->ReadEndElement();
            }
            else
            {
                readPolicies = true;
            }
        }while(!readPolicies);
        // </Policies>
        xmlReader->ReadEndElement();
    }
}

ErrorCode ApplicationPoliciesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<Policies>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Policies, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    if (!LogCollectionEntries.empty())
    {
        //LogCollectionPolicies
        er = xmlWriter->WriteStartElement(*SchemaNames::Element_LogCollectionPolicies, L"", *SchemaNames::Namespace);
        if (!er.IsSuccess())
        {
            return er;
        }
        for (auto i = 0; i < LogCollectionEntries.size(); i++)
        {
            er = (LogCollectionEntries[i]).WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }
        //</LogcollectionPolicies>
        er = xmlWriter->WriteEndElement();
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    er = this->DefaultRunAs.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = this->HealthPolicy.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    if (!SecurityAccessPolicies.empty())
    {	//<SecurityAccessPolicies>
        er = xmlWriter->WriteStartElement(*SchemaNames::Element_SecurityAccessPolicies, L"", *SchemaNames::Namespace);
        if (!er.IsSuccess())
        {
            return er;
        }
        for (auto i = 0; i < SecurityAccessPolicies.size(); i++)
        {
            er = (SecurityAccessPolicies[i]).WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }//</SecurityAccessPolicies
        er = xmlWriter->WriteEndElement();
    }

    //</policies>
    return xmlWriter->WriteEndElement();
}

void ApplicationPoliciesDescription::clear()
{
    this->DefaultRunAs.clear();
    this->LogCollectionEntries.clear();
    this->HealthPolicy.clear();
}
