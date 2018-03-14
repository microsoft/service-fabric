// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ULONGLONG TraceKeywords::ParseTestKeyword(wstring const& testKeyword)
{
    if (Common::StringUtility::AreEqualCaseInsensitive(testKeyword, L"Test0"))
    {
        return TraceKeywords::Test0;
    }

    if (Common::StringUtility::AreEqualCaseInsensitive(testKeyword, L"Test1"))
    {
        return TraceKeywords::Test1;
    }

    if (Common::StringUtility::AreEqualCaseInsensitive(testKeyword, L"Test2"))
    {
        return TraceKeywords::Test2;
    }

    if (Common::StringUtility::AreEqualCaseInsensitive(testKeyword, L"Test3"))
    {
        return TraceKeywords::Test3;
    }

    else return 0;
}
