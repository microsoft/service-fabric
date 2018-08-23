// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ConfigSection::ConfigSection()
    : Name(),
    Parameters()
{
}

ConfigSection::ConfigSection(std::wstring && name, ParametersMapType && parameters)
    : Name(move(name)),
    Parameters(move(parameters))
{
}

ConfigSection::ConfigSection(ConfigSection const & other)
    : Name(other.Name),
    Parameters(other.Parameters)
{
}

ConfigSection::ConfigSection(ConfigSection && other)
    : Name(move(other.Name)),
    Parameters(move(other.Parameters))
{
}

ConfigSection const & ConfigSection::operator = (ConfigSection const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Parameters = other.Parameters;
    }

    return *this;
}

ConfigSection const & ConfigSection::operator = (ConfigSection && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->Parameters = move(other.Parameters);
    }

    return *this;
}

bool ConfigSection::operator == (ConfigSection const & other) const
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

bool ConfigSection::operator != (ConfigSection const & other) const
{
    return !(*this == other);
}

void ConfigSection::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigSection {");
    w.Write("Name = {0},", Name);

    w.Write("Parameters = {");
    for(auto iter = Parameters.cbegin(); iter != Parameters.cend(); ++ iter)
    {
        w.WriteLine("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void ConfigSection::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_CONFIGURATION_SECTION & publicSection) const
{
    publicSection.Name = heap.AddString(this->Name);

    auto publicParameterList = heap.AddItem<FABRIC_CONFIGURATION_PARAMETER_LIST>();
    publicSection.Parameters = publicParameterList.GetRawPointer();

    auto parameterCount = this->Parameters.size();
    publicParameterList->Count = static_cast<ULONG>(parameterCount);

    if (parameterCount > 0)
    {
        auto publicItems = heap.AddArray<FABRIC_CONFIGURATION_PARAMETER>(parameterCount);
        publicParameterList->Items = publicItems.GetRawArray();

        size_t ix = 0;
        for(auto iter = this->Parameters.cbegin(); iter != this->Parameters.cend(); ++iter)
        {
            iter->second.ToPublicApi(heap, publicItems[ix]);
            ++ix;
        }
    }
}

void ConfigSection::Set(ConfigParameter && parameter)
{
    wstring parameterName(parameter.Name);

    auto iter = this->Parameters.find(parameterName);
    if (iter != this->Parameters.end())
    {
        iter->second = move(parameter);
    }
    else
    {
        this->Parameters.insert(make_pair(move(parameterName), move(parameter)));
    }
}

void ConfigSection::clear()
{
    this->Name.clear();
    this->Parameters.clear();
}

void ConfigSection::Merge(ConfigSection const & other)
{
    for(auto otherParamIter = other.Parameters.cbegin();
        otherParamIter != other.Parameters.cend();
        ++otherParamIter)
    {
        this->Parameters[otherParamIter->first] = otherParamIter->second;
    }
}
