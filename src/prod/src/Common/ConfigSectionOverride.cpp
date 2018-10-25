// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ConfigSectionOverride::ConfigSectionOverride()
    : Name(),
    Parameters()
{
}

ConfigSectionOverride::ConfigSectionOverride(std::wstring && name, ParametersMapType && parameters) :
    Name(move(name)),
    Parameters(move(parameters))
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

void ConfigSectionOverride::clear()
{
    this->Name.clear();
    this->Parameters.clear();
}
