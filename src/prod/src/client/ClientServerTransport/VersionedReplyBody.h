// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class VersionedReplyBody : public Serialization::FabricSerializable
    {
    public:
        VersionedReplyBody()
            : fabricVersion_(Common::FabricVersion::Baseline)
        {
        }

        __declspec(property(get=get_FabricVersion)) Common::FabricVersion const FabricVersion;
        Common::FabricVersion const & get_FabricVersion() const { return fabricVersion_; }

        FABRIC_FIELDS_01(fabricVersion_);

    private:
        Common::FabricVersion fabricVersion_;
    };
}
