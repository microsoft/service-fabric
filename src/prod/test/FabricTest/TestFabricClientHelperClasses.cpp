// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;  


TestNamingEntry::TestNamingEntry(Common::NamingUri name, ULONG propertyCount)
    : name_(name), isCreated_(false), properties_()
{
    for(ULONG i = 0; i < propertyCount; i++)
    {
        wstring propertyName = name.Name + StringUtility::ToWString(i);
        properties_[propertyName] = TestFabricClient::EmptyValue;
    }
}

void TestNamingEntry::put_IsCreated(bool value)
{ 
    isCreated_ = value; 
    if(!isCreated_)
    {
        for (auto it = properties_.begin() ; it != properties_.end(); it++ )
        {
            (*it).second = TestFabricClient::EmptyValue;
        }
    }
}
