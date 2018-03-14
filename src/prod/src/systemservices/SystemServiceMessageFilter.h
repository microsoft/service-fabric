// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class SystemServiceMessageFilter : public Federation::IMessageFilter
    {
    public:

        explicit SystemServiceMessageFilter(SystemServiceLocation const &);

        explicit SystemServiceMessageFilter(SystemServiceFilterHeader const &);

        virtual ~SystemServiceMessageFilter();

        bool operator < (SystemServiceMessageFilter const &  other) const;

        virtual bool Match(Transport::Message & message);

    private:
        Common::Guid partitionId_;
        ::FABRIC_REPLICA_ID replicaId_;
        __int64 replicaInstance_;
    };

    typedef std::shared_ptr<SystemServiceMessageFilter> SystemServiceMessageFilterSPtr;
}
