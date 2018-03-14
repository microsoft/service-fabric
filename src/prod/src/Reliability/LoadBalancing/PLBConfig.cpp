// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLBConfig.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

DEFINE_SINGLETON_COMPONENT_CONFIG(PLBConfig)

PLBConfig::KeyDoubleValueMap PLBConfig::KeyDoubleValueMap::Parse(StringMap const & entries)
{
    KeyDoubleValueMap result;

    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        wstring key = Config::Parse<wstring>(it->first);
        double value = Config::Parse<double>(it->second);
        // remove the entry if the value is invalid
        if (value >= 0)
        {
            result[key] = value;
        }
    }

    return result;
}

void PLBConfig::KeyDoubleValueMap::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    for (auto it = begin(); it != end(); ++it)
    {
        w.WriteLine("\r\n{0}:{1}", it->first, it->second);
    }
}

PLBConfig::KeyBoolValueMap PLBConfig::KeyBoolValueMap::Parse(StringMap const & entries)
{
    KeyBoolValueMap result;

    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        wstring key = Config::Parse<wstring>(it->first);
        bool value = Config::Parse<bool>(it->second);

        result[key] = value;
    }

    return result;
}

void PLBConfig::KeyBoolValueMap::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    for (auto it = begin(); it != end(); ++it)
    {
        w.WriteLine("\r\n{0}:{1}", it->first, it->second);
    }
}
