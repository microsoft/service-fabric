// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{    
    class StoreService::ProcessEnumeratePropertiesRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessEnumeratePropertiesRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:

        DEFINE_PERF_COUNTERS ( EnumerateProperties )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);
        bool AllowNameFragment() override;

    private:
        Common::ErrorCode InitializeEnumeration(
            TransactionSPtr const &,
            __out EnumerationSPtr &);

        Common::ErrorCode EnumerateProperties(
            EnumerationSPtr const &);

        bool FitsInReplyMessage(NamePropertyResult const &);

        EnumeratePropertiesRequest body_;
        std::wstring storeKeyPrefix_;
        std::vector<NamePropertyResult> propertyResults_;
        std::wstring lastEnumeratedPropertyName_;
        __int64 propertiesVersion_;
        FABRIC_ENUMERATION_STATUS enumerationStatus_;
        size_t replySizeThreshold_;
        size_t replyItemsSizeEstimate_;
        size_t replyTotalSizeEstimate_;
    };
}
