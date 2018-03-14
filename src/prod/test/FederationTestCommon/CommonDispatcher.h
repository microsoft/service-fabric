// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTestCommon
{
    class CommonDispatcher: public TestCommon::TestDispatcher
    {
        DENY_COPY(CommonDispatcher)
    public:
        CommonDispatcher();

        bool ExecuteCommand(std::wstring command);

        virtual void PrintHelp(std::wstring const & command) = 0;

        static Federation::NodeId ParseNodeId(std::wstring const & nodeIdString);

    protected:
        static Common::LargeInteger ParseLargeInteger(std::wstring const & value);

    private:
        bool DeclareVotes(Common::StringCollection const & params);

        bool ListBehaviors();
        bool AddUnreliableTransportBehavior(Common::StringCollection const & params);
        bool RemoveUnreliableTransportBehavior(Common::StringCollection const & params);
    };
}
