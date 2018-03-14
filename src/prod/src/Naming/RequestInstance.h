// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class RequestInstance : public Common::TextTraceComponent<Common::TraceTaskCodes::NamingStoreService>
    {
        DENY_COPY(RequestInstance);

    public:
        RequestInstance();

        explicit RequestInstance(std::wstring const & traceId);

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        void SetTraceId(std::wstring const &);

        __int64 GetNextInstance();

        void UpdateToHigherInstance(__int64 higherInstance);

    private:
        std::wstring traceId_;
        __int64 instance_;
        Common::RwLock lock_;
    };

    typedef std::unique_ptr<RequestInstance> RequestInstanceUPtr;
}
