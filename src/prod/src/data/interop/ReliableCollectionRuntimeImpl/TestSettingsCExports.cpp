// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Common;
using namespace Data::Interop;

TestSettings Data::Interop::testSettings;

extern "C" void Test_UseEnv(BOOL enable)
{
    testSettings.enabled_ = false;
    if (enable)
    {
        wstring value;
        srand((unsigned int)time(NULL));
        testSettings.asyncMode_ = AsyncMode::None;
        if(Environment::GetEnvironmentVariable(L"SF_RELIABLECOLLECTION_ASYNCMODE", value, NOTHROW()))
        {
            switch (value[0])
            {
            case L'0':
                break;
            case L'1':
                testSettings.asyncMode_ = AsyncMode::Async;
                break;
            case L'2':
                testSettings.asyncMode_ = AsyncMode::Random;
            }
        }
        testSettings.enabled_ = true;
    }
}
