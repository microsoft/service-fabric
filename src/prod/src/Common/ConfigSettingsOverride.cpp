// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ConfigSettingsOverride::ConfigSettingsOverride()
    : Sections()
{
}

ConfigSettingsOverride::ConfigSettingsOverride(SectionMapType && sections)
    : Sections(move(sections))
{
}

ConfigSettingsOverride::ConfigSettingsOverride(ConfigSettingsOverride const & other)
    : Sections(other.Sections)
{
}

ConfigSettingsOverride::ConfigSettingsOverride(ConfigSettingsOverride && other)
    : Sections(move(other.Sections))
{
}

ConfigSettingsOverride const & ConfigSettingsOverride::operator = (ConfigSettingsOverride const & other)
{
    if (this != &other)
    {
        this->Sections = other.Sections;
    }

    return *this;
}

ConfigSettingsOverride const & ConfigSettingsOverride::operator = (ConfigSettingsOverride && other)
{
    if (this != &other)
    {
        this->Sections = move(other.Sections);
    }

    return *this;
}

bool ConfigSettingsOverride::operator == (ConfigSettingsOverride const & other) const
{
    bool equals = this->Sections.size() == other.Sections.size();
    if (!equals) { return equals; }

    for(auto iter1 = this->Sections.cbegin(); iter1 != this->Sections.cend(); ++iter1)
    {
        auto iter2 = other.Sections.find(iter1->first);
        if (iter2 == other.Sections.end()) { return false; }

        equals = iter1->second == iter2->second;
        if (!equals) { return equals; }
    }

    return equals;
}

bool ConfigSettingsOverride::operator != (ConfigSettingsOverride const & other) const
{
    return !(*this == other);
}

void ConfigSettingsOverride::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("TODO {");

    w.Write("Sections = {");
    for(auto iter = Sections.cbegin(); iter != Sections.cend(); ++ iter)
    {
        w.WriteLine("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void ConfigSettingsOverride::clear()
{
    this->Sections.clear();
}
