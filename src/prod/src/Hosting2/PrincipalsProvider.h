// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class PrincipalsProvider : 
        public Common::RootedObject,
        public Common::FabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(PrincipalsProvider)

    public:
        explicit PrincipalsProvider(Common::ComponentRoot const & root, std::wstring const & nodeId);
        virtual ~PrincipalsProvider();

        static Common::GlobalWString GlobalMutexName;

    protected:
        // ****************************************************
        // FabricComponent specific methods
        // ****************************************************
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:

        std::wstring nodeId_;
    };
}
