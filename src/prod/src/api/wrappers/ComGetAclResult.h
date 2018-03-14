// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComGetAclResult
        : public IFabricGetAclResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetAclResult)

        COM_INTERFACE_LIST1(
        ComGetAclResult,
        IID_IFabricGetAclResult,
        IFabricGetAclResult)

    public:
        explicit ComGetAclResult(AccessControl::FabricAcl const & fabricAcl);

        const FABRIC_SECURITY_ACL * STDMETHODCALLTYPE get_Acl(void) override;

    private:
        Common::ScopedHeap heap_;
        FABRIC_SECURITY_ACL fabricAcl_;
    };
}
