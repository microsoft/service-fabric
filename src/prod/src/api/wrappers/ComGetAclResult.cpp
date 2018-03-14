// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace AccessControl;
using namespace Common;
using namespace std;

ComGetAclResult::ComGetAclResult(FabricAcl const & fabricAcl)
{
    fabricAcl.ToPublic(heap_, fabricAcl_);
}

const FABRIC_SECURITY_ACL * STDMETHODCALLTYPE ComGetAclResult::get_Acl(void)
{
    return &fabricAcl_;
}
