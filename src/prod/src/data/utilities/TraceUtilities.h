// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Data
{
    namespace Utilities
    {
        __inline KActivityId PtrToActivityId(VOID* ptr)
        {
            return static_cast<KActivityId>(reinterpret_cast<ULONGLONG>(ptr));
        }
    }
}
