// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

InfrastructureDescription::InfrastructureDescription()
    : NodeList()
{
}

InfrastructureDescription::InfrastructureDescription(InfrastructureDescription const & other)
    : NodeList(other.NodeList)
{
}

InfrastructureDescription::InfrastructureDescription(InfrastructureDescription && other) 
    : NodeList(move(other.NodeList))
{
}

InfrastructureDescription const & InfrastructureDescription::operator = (InfrastructureDescription const & other)
{
    if (this != & other)
    {
        this->NodeList = other.NodeList;
    }

    return *this;
}

InfrastructureDescription const & InfrastructureDescription::operator = (InfrastructureDescription && other)
{
    if (this != & other)
    {
        this->NodeList = move(other.NodeList);
    }

    return *this;
}

bool InfrastructureDescription::operator == (InfrastructureDescription const & other) const
{
    bool equals = true;

    equals = this->NodeList.size() == other.NodeList.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->NodeList.size(); ++ix)
    {
        auto it = find(other.NodeList.begin(), other.NodeList.end(), this->NodeList[ix]);
        equals = it != other.NodeList.end();
        if (!equals) { return equals; }
    }

    return equals;
}

bool InfrastructureDescription::operator != (InfrastructureDescription const & other) const
{
    return !(*this == other);
}

void InfrastructureDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("InfrastructureDescription { ");
    w.Write("NodeList = {");
    for(auto iter = NodeList.cbegin(); iter != NodeList.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

ErrorCode InfrastructureDescription::FromXml(wstring const & fileName)
{
    return Parser::ParseInfrastructureDescription(fileName, *this);
}

void InfrastructureDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <InfrastructureInformation ..
    xmlReader->StartElement(
        *SchemaNames::Element_InfrastructureInformation,
        *SchemaNames::Namespace,
        false);

    // <InfrastructureInformation ...>
    xmlReader->ReadStartElement();

    this->ParseNodeList(xmlReader);

    // </InfrastructureInformation>
    xmlReader->ReadEndElement();
}

void InfrastructureDescription::ParseNodeList(
        XmlReaderUPtr const & xmlReader)
{
    // <NodeList ..>
    xmlReader->StartElement(
        *SchemaNames::Element_NodeList,
        *SchemaNames::Namespace,
        false);
    xmlReader->ReadStartElement();

   
    bool done = false;
    while(!done)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_Node,
            *SchemaNames::Namespace,
            false))
        {
            InfrastructureNodeDescription node;
            node.ReadFromXml(xmlReader);
            this->NodeList.push_back(move(node));
        }
        else
        {
            done = true;
        }
    } 

    // </NodeList>
    xmlReader->ReadEndElement();
}

void InfrastructureDescription::clear()
{
    this->NodeList.clear();
}
