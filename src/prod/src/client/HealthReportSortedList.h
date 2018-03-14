// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // List that keeps health reports sorted by sequence number and,
    // for reports with same sn, by EntityPropertyId.
    class HealthReportSortedList
    {
        DENY_COPY(HealthReportSortedList)

    public:
        HealthReportSortedList();

        HealthReportSortedList(HealthReportSortedList && other);
        HealthReportSortedList & operator= (HealthReportSortedList && other);

        ~HealthReportSortedList();

        bool IsEmpty() const;

        size_t Count() const;

        HealthReportWrapperPtr GetLastReport() const;

        Common::ErrorCode Add(HealthReportWrapperPtr const report);

        Common::ErrorCode Remove(HealthReportWrapperPtr const report);

        void GetContentUpTo(
            size_t maxNumberOfReports,
            FABRIC_SEQUENCE_NUMBER lastAcceptedSN,
            __out std::vector<ServiceModel::HealthReport> & reports,
            __out size_t & currentReportCount,
            __out FABRIC_SEQUENCE_NUMBER & lastLsn) const;

        void Clear();

        std::wstring ToString() const;

    private:
        void CheckInvariants() const;

        struct Comparator
        {
            inline bool operator()(HealthReportWrapperPtr const left, HealthReportWrapperPtr const right) const
            {
                ASSERT_IF(left == nullptr || right == nullptr, "Comparator: null pointers are not allowed");
                return left->operator<(*right);
            }
        };

    private:
        std::set<HealthReportWrapperPtr, Comparator> list_;
    };
}
