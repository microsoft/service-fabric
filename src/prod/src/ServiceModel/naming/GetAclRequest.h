// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class GetAclRequest : public ServiceModel::ClientServerMessageBody
    {
    public:
        GetAclRequest();
        GetAclRequest(Common::NamingUri const & fabricUri);

        Common::NamingUri const & FabricUri() const;

        FABRIC_FIELDS_01(fabricUri_);

    private:
        Common::NamingUri fabricUri_;
    };
}
