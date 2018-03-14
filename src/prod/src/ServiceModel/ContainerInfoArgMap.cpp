// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerInfoArgMap::ContainerInfoArgMap(){ }

ContainerInfoArgMap::~ContainerInfoArgMap(){ }

void ContainerInfoArgMap::Insert(wstring const & key, wstring const & value)
{
    containerInfoArgMap_.insert(make_pair(key, value));
}

std::wstring ContainerInfoArgMap::GetValue(std::wstring const & key)
{
    wstring value;
    auto entry = containerInfoArgMap_.find(key);
    if (entry != containerInfoArgMap_.end())
    {
        value = entry->second;
    }

    return value;
}

bool ContainerInfoArgMap::ContainsKey(wstring const & key)
{
    return containerInfoArgMap_.find(key) != containerInfoArgMap_.end();
}

void ContainerInfoArgMap::Erase(wstring const & key)
{
    containerInfoArgMap_.erase(key);
}
