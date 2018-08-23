// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class UnregisterApplicationHostRequest : public Serialization::FabricSerializable
    {
    public:
        UnregisterApplicationHostRequest();
        UnregisterApplicationHostRequest(Common::ActivityDescription const & activityDescription, std::wstring const & hostId);

        __declspec(property(get=get_ActivityDescription)) Common::ActivityDescription const & ActivityDescription;
        Common::ActivityDescription const & get_ActivityDescription() const { return activityDescription_; }

        __declspec(property(get=get_hostId)) std::wstring const & Id;
        std::wstring const & get_hostId() const { return id_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(activityDescription_, id_);

    private:
        Common::ActivityDescription activityDescription_;
        std::wstring id_;
    };
}
