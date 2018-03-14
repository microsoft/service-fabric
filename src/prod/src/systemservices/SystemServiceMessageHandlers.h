// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    template <class TMessageHandler>
    class SystemServiceMessageHandlers
    {
    public:
        SystemServiceMessageHandlers() : handlers_(), handlersLock_() { }

        virtual ~SystemServiceMessageHandlers() { }

        void Clear();

        void SetHandler(SystemServiceLocation const &, TMessageHandler const &);

        void RemoveHandler(SystemServiceLocation const &);

        TMessageHandler GetHandler(__in Transport::MessageUPtr &);

    private:
        std::map<SystemServiceMessageFilter, TMessageHandler> handlers_;
        Common::RwLock handlersLock_;
    };
}
