// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class NodeEnabledNotification : public Serialization::FabricSerializable
    {
    public:
        NodeEnabledNotification();
        NodeEnabledNotification(Common::ErrorCode error);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }
    private:
        Common::ErrorCode error_;
    };
}
