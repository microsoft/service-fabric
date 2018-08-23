// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IGuestServiceReplica : public IFabricStatefulServiceReplica
    {
    public:
        virtual void STDMETHODCALLTYPE OnActivatedCodePackageTerminated(CodePackageEventDescription && eventDesc) = 0;
    };
}
