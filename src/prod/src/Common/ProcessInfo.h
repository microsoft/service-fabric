// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ProcessInfo
    {
        DENY_COPY(ProcessInfo);
    public:
        ProcessInfo();
        ~ProcessInfo();

        void RefreshAll();

        size_t RefreshDataSize();
        size_t DataSize() const { return dataSize_; }
        size_t GetDataSizeAndSummary(std::string & summary);

        size_t Rss() const { return rss_; }
        size_t GetRssAndSummary(std::string & summary);

        size_t VmSize() const { return vmSize_; }

        static ProcessInfo* GetSingleton();

    private:
        void read_proc_self_statm(char* buf, size_t bufSize);
        void read_proc_self_status();

        static std::string GenerateSummaryString(
            uint threadCount,
            size_t dataSize,
            size_t vmSize,
            size_t vmPeak,
            size_t rss,
            size_t rssPeak);

        int proc_self_statm_fd_;
        int pageSize_;
        size_t vmSize_;
        size_t rss_;
        size_t dataSize_;
        size_t vmPeak_; 
        size_t rssPeak_;
        uint threadCount_;
    };
}
