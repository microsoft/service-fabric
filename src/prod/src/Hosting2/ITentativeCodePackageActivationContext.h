// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ITentativeCodePackageActivationContext : public IFabricCodePackageActivationContext6
    {
    public:
        virtual ULONG STDMETHODCALLTYPE TryAddRef() = 0;
    };
}
