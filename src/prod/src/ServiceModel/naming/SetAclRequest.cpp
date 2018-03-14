// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Naming;
using namespace Common;

SetAclRequest::SetAclRequest()
{
}

SetAclRequest::SetAclRequest(Common::NamingUri const & fabricUri, AccessControl::FabricAcl const & fabricAcl)
: fabricUri_(fabricUri), fabricAcl_(fabricAcl)
{
}

NamingUri const & SetAclRequest::FabricUri() const
{
    return fabricUri_;
}

AccessControl::FabricAcl const & SetAclRequest::FabricAcl() const
{
    return fabricAcl_;
}
