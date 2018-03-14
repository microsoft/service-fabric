// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Contains the token owned by sender and shutdown nodes that it knows about.
    class ChangeNotificationMessageBody : public Serialization::FabricSerializable
    {
    public:

        ChangeNotificationMessageBody() 
        {}

        ChangeNotificationMessageBody(
            Federation::RoutingToken const & token,
            std::vector<Federation::NodeInstance> && shutdownNodes)
            : token_(token),
              shutdownNodes_(std::move(shutdownNodes))
        {}

        __declspec (property(get=get_Token)) Federation::RoutingToken const & Token;
        Federation::RoutingToken const& get_Token() const { return token_; }

        __declspec (property(get=get_ShutdownNodes)) std::vector<Federation::NodeInstance> const & ShutdownNodes;
        std::vector<Federation::NodeInstance> const & get_ShutdownNodes() const { return shutdownNodes_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        FABRIC_FIELDS_02(token_, shutdownNodes_);

    private:
        Federation::RoutingToken token_;
        std::vector<Federation::NodeInstance> shutdownNodes_;
    };
}
