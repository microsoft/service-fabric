// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class SetAclRequest : public ServiceModel::ClientServerMessageBody
    {
    public:
        SetAclRequest();
        SetAclRequest(Common::NamingUri const & fabricUri, AccessControl::FabricAcl const & fabricAcl);

        Common::NamingUri const & FabricUri() const;
        AccessControl::FabricAcl const & FabricAcl() const;

        FABRIC_FIELDS_02(fabricUri_, fabricAcl_);

    private:
        Common::NamingUri fabricUri_;
        AccessControl::FabricAcl fabricAcl_;
    };
}
