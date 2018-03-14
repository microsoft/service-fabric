// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetChaosReportDescription
            : public Serialization::FabricSerializable
        {
        public:
            GetChaosReportDescription();
            GetChaosReportDescription(
                std::shared_ptr<ChaosReportFilter> &,
                std::wstring &,
                std::wstring &);

            explicit GetChaosReportDescription(GetChaosReportDescription const &);
            explicit GetChaosReportDescription(GetChaosReportDescription &&);

            __declspec(property(get = get_Filter)) std::shared_ptr<ChaosReportFilter> const & FilterSPtr;
            std::shared_ptr<ChaosReportFilter> const & get_Filter() const { return filterSPtr_; }

            __declspec(property(get = get_Token)) std::wstring const& ContinuationToken;
            std::wstring const& get_Token() const { return continuationToken_; }

            Common::ErrorCode FromPublicApi(FABRIC_GET_CHAOS_REPORT_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_GET_CHAOS_REPORT_DESCRIPTION &) const;

            FABRIC_FIELDS_03(
                filterSPtr_,
                continuationToken_,
                clientType_);

        private:
            std::shared_ptr<ChaosReportFilter> filterSPtr_;
            std::wstring continuationToken_;
            std::wstring clientType_;
        };
    }
}
