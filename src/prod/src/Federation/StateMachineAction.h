// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class StateMachineAction
    {
    public:
        virtual ~StateMachineAction() {}

        virtual void Execute(SiteNode & siteNode) = 0;

        virtual std::wstring ToString() = 0;
    };

    typedef std::unique_ptr<StateMachineAction> StateMachineActionUPtr;

    class StateMachineActionCollection
    {
    public:
        StateMachineActionCollection()
            :   actions_(nullptr)
        {
        }

        bool IsEmpty() { return (actions_ == nullptr); }

        void Add(StateMachineActionUPtr && action);

        void Execute(SiteNode & siteNode);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        typedef std::vector<StateMachineActionUPtr> StateMachineActionVector;

        static void ExecuteActions(StateMachineActionVector const & actions, SiteNode & siteNode);

        std::unique_ptr<StateMachineActionVector> actions_;
    };
}
