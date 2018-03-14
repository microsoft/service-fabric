// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        interface IDisposable
        {
            K_SHARED_INTERFACE(IDisposable)

        public:
  
            virtual void Dispose() = 0;
        };
    }
}
