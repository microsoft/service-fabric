// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    namespace FullCopyMode
    {
        FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE ToPublicApi(
            __in Enum internalMode)
        {
            switch (internalMode)
            {
            case Physical:
                return FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_PHYSICAL;

            case Logical:
                return FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_LOGICAL;

            case Rebuild:
                return FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_REBUILD;

            default:
                return FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_DEFAULT;
            }
        }

        ErrorCode FromPublicApi(
            __in FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE publicMode,
            __out Enum & internalMode)
        {
            switch (publicMode)
            {
            case FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_DEFAULT:
                internalMode = Default;
                return ErrorCodeValue::Success;

            case FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_PHYSICAL:
                internalMode = Physical;
                return ErrorCodeValue::Success;

            case FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_LOGICAL:
                internalMode = Logical;
                return ErrorCodeValue::Success;

            case FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_REBUILD:
                internalMode = Rebuild;
                return ErrorCodeValue::Success;

            default:
                return ErrorCodeValue::InvalidArgument;
            }
        }

        void WriteToTextWriter(__in TextWriter & w, Enum const & e)
        {			
            switch (e)
            {
            case Default: w << "Default"; return;
            case Physical: w << "Physical"; return;
            case Logical: w << "Logical"; return;
            case Rebuild: w << "Rebuild"; return;
            default: w << "FullCopyMode(" << static_cast<ULONG>(e) << ')'; return;
            }
        }

        ENUM_STRUCTURED_TRACE(FullCopyMode, FirstValidEnum, LastValidEnum)
    }
}

