// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTypeHost
{
    class StatelessServiceTypeHost :
        public Common::ComponentRoot,
        public Common::FabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::FabricTypeHost>
    {
        DENY_COPY(StatelessServiceTypeHost)

    public:
        StatelessServiceTypeHost(std::vector<std::wstring> && typesToHost);
        virtual ~StatelessServiceTypeHost();

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        Common::ComPointer<IFabricRuntime> runtime_;
        std::vector<std::wstring> const typesToHost_;
        std::wstring traceId_;
    };
}
