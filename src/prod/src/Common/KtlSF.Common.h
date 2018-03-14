// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // Common caching SF support accessor methods for default KtlSystem instance
    // used in SF across a common link unit. The assumption is that this default 
    // KtlSystem instance will only be terminated by process exit.
    extern KtlSystem& GetSFDefaultKtlSystem();
    extern KAllocator& GetSFDefaultPagedKAllocator();
    extern KAllocator& GetSFDefaultNonPagedKAllocator();
}
