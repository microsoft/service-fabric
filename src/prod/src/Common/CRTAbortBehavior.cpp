//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace CRTAbortBehavior
    {
        int CustomReportHook(int reportType, wchar_t *message, int *returnValue)
        {        
            UNREFERENCED_PARAMETER(message);
            UNREFERENCED_PARAMETER(returnValue);
            
        #ifndef PLATFORM_UNIX 
            if (reportType == _CRT_ASSERT)   
            {
                // return nonzero to indicate the issue was unhandled which will cause a crash
                return 1;
            }
        #endif
        
            // do nothing - this may have been called for other nonfatal reportTypes
            return 0;
        }
        
        // By default CRT debug asserts will open a blocking popup and generate no dump.  This
        // disables the behavior.
        void DisableAbortPopup()
        {
        #ifndef PLATFORM_UNIX 
            _CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, CustomReportHook);
        #endif
        } 
    }
}
