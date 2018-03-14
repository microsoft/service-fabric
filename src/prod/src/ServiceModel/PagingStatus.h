// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class PagingStatus;
    typedef std::unique_ptr<PagingStatus> PagingStatusUPtr;

    class PagingStatus
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(PagingStatus)

    public:
        PagingStatus();

        explicit PagingStatus(std::wstring && continuationToken);

        PagingStatus(PagingStatus && other);
        PagingStatus & operator = (PagingStatus && other);

        ~PagingStatus();

        __declspec(property(get = get_ContinuationToken, put = set_ContinuationToken)) std::wstring const & ContinuationToken;
        std::wstring const & get_ContinuationToken() const { return continuationToken_; }
        void set_ContinuationToken(std::wstring && value) { continuationToken_ = std::move(value); }

        std::wstring && TakeContinuationToken() { return std::move(continuationToken_); }

        template <class T>
        static std::wstring CreateContinuationToken(T const & data);

        // Note: This should be used only with non empty continuation tokens
        // Otherwise, it will return an error.
        template <class T>
        static Common::ErrorCode GetContinuationTokenData(std::wstring const & continuationToken, __inout T & data);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_PAGING_STATUS & publicPagingStatus) const;
        Common::ErrorCode FromPublicApi(FABRIC_PAGING_STATUS const & publicPagingStatus);

        FABRIC_FIELDS_01(continuationToken_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ContinuationToken, continuationToken_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring continuationToken_;
    };
}

