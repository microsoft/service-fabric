// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if KTL_USER_MODE

namespace Ktl
{
    namespace Com
    {
        //
        // This class allow derived operations to use HRESULT or ErrorCode instead of NTSTATUS for completion
        //
        class FabricAsyncContextBase : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(FabricAsyncContextBase)

        public:
            const static BOOLEAN IsFabricAsyncContextBase = TRUE;   // Used for compile-time type check

            inline HRESULT Result() { return _Result; }

        protected:
            friend class AsyncCallInAdapter;

            BOOLEAN Complete(__in HRESULT Hr);
            BOOLEAN Complete(__in Common::ErrorCode ErrorCode);
            BOOLEAN Complete(__in Common::ErrorCodeValue::Enum ErrorCode);

            VOID OnReuse() override;

            virtual VOID OnCallbackInvoked();

        private:
            VOID OnUnsafeCompleteCalled(void* ErrorCodeCtx);

        private:
            HRESULT     _Result;
        };
    }
}

#endif
