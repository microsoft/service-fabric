//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Common
{
    namespace CRTAbortBehavior
    {
        int CustomReportHook(int reportType, wchar_t *message, int *returnValue);
        void DisableAbortPopup(); 
    }
}
