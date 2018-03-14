// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    static const GUID CLSID_SendRequestAsyncOperation =
    { 0xd80af29b, 0xfb0, 0x4c68, { 0x9e, 0xfb, 0xb5, 0x41, 0x58, 0x24, 0x44, 0xf6 } };

    class ComFabricSendRequestAsyncOperation : public  Common::ComAsyncOperationContext
    {
        DENY_COPY(ComFabricSendRequestAsyncOperation);

        COM_INTERFACE_AND_DELEGATE_LIST(
            ComFabricSendRequestAsyncOperation,
            CLSID_SendRequestAsyncOperation,
            ComFabricSendRequestAsyncOperation,
            ComAsyncOperationContext)

    public:
        ComFabricSendRequestAsyncOperation(__in ICommunicationMessageSenderPtr & impl)
            : ComAsyncOperationContext()
            , impl_(impl)
            , request_()
            , reply_()
            , timeout_()
        {
        }

        HRESULT Initialize(
            __in IFabricServiceCommunicationMessage *request,
            __in DWORD timeoutInMilliSeconds,
            __in IFabricAsyncOperationCallback * callback,
            Common::ComponentRootSPtr const & rootSPtr);
        
        static HRESULT End(__in IFabricAsyncOperationContext * context, __out IFabricServiceCommunicationMessage **reply);
      


    protected:
        virtual void OnStart(Common::AsyncOperationSPtr const & proxySPtr);
        

    private:

        void FinishRequest(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
        

        HRESULT ComAsyncOperationContextEnd();
        
        ICommunicationMessageSenderPtr impl_;
        Transport::MessageUPtr request_;
        Transport::MessageUPtr reply_;
        Common::TimeSpan timeout_;
    };
}

