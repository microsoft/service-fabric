// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "JsonSerializableTestBase.h"
#include "QuerySerializationInteropTest.h"
#include "HealthSerializationInteropTest.h"
#include "DescriptionSerializationInteropTest.h"

namespace NativeAndManagedSerializationInteropTest
{
    using namespace std;
    using namespace Common;

    ErrorCode DeserializeAndSerialize(wstring itemKind, wstring filepath1, wstring filepath2);
    
    template<typename T>
    ErrorCode DeserializeAndSerializeItem(T &item, wstring filepath1, wstring filepath2);
    
    template<typename T>
    ErrorCode DeserializeFromFile(T &item, const wstring &fileIn);

    template<typename T>
    ErrorCode SerializeToFile(T &item, const wstring &fileOut);
}
