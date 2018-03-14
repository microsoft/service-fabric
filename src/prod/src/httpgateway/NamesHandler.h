// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class NamesHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(NamesHandler)

    public:
        explicit NamesHandler(__in HttpGatewayImpl & server);
        virtual ~NamesHandler();

        Common::ErrorCode Initialize();

    private:

        void CreateName(Common::AsyncOperationSPtr const & thisSPtr);
        void OnCreateNameComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void DeleteName(Common::AsyncOperationSPtr const & thisSPtr);
        void OnDeleteNameComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void NameExists(Common::AsyncOperationSPtr const & thisSPtr);
        void OnNameExistsComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void EnumerateSubNames(Common::AsyncOperationSPtr const & thisSPtr);
        void OnEnumerateSubNamesComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void PutProperty(Common::AsyncOperationSPtr const & thisSPtr);
        void OnPutPropertyComplete(
                Common::AsyncOperationSPtr const & operation,
                bool expectedCompletedSynchronously,
                bool isCustomProperty);
        
        void DeleteProperty(Common::AsyncOperationSPtr const & thisSPtr);
        void OnDeletePropertyComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void GetProperty(Common::AsyncOperationSPtr const & thisSPtr);
        void OnGetPropertyComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void EnumerateProperties(Common::AsyncOperationSPtr const & thisSPtr);
        void OnEnumeratePropertiesComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously, bool includeValues);

        void SubmitPropertyBatch(Common::AsyncOperationSPtr const & thisSPtr);
        void OnSubmitPropertyBatchComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
    };
}
