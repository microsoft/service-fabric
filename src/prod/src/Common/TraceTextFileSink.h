// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TraceTextFileSink
    {
        DENY_COPY(TraceTextFileSink);

    public:
        static std::wstring GetPath()
        {
            return Singleton->path_;
        }

        static void SetPath(std::wstring const & path)
        {
            Singleton->PrivateSetPath(path);
        }

        static void SetOption(std::wstring const & option)
        {
            Singleton->PrivateSetOption(option);
        }

        static void Write(StringLiteral taskName, StringLiteral eventName, LogLevel::Enum level, std::wstring const & id, std::wstring const & data)
        {
            Singleton->PrivateWrite(taskName, eventName, level, id, data);
        }

        static bool IsEnabled()
        {
            return Singleton->enabled_;
        }

    private:
        // Helper class to release file handle without destructing TraceTextFileSink::Singleton
        class CleanupHelper
        {
        public:
            CleanupHelper();
            ~CleanupHelper();
        };

        static TraceTextFileSink* Singleton;

        static int64 const DefaultSizeCheckIntervalInMinutes;
        static int const DefaultMaxFilesToKeep;

        TraceTextFileSink();

        DateTime CalculateCheckTime(DateTime now, int64 minutes);
        void CloseFile();
        void OpenFile();
        void PrivateSetPath(std::wstring const & path);
        void PrivateSetOption(std::wstring const & option);
        void PrivateWrite(StringLiteral taskName, StringLiteral eventName, LogLevel::Enum level, std::wstring const & id, std::wstring const & data);
        void Disable();
        static std::wstring TimeToFileNameSuffix(DateTime const & now);

        File file_;
        RwLock lock_;
        std::vector<std::wstring> files_;
        std::wstring path_;
        std::wstring option_;
        DateTime segmentTime_;
        DateTime sizeCheckTime_;
        int64 maxSize_;
        int64 sizeCheckIntervalInMinutes_;
        bool enabled_;

        std::wstring const processNameOption_;
        std::wstring const processIdOption_;
        std::wstring const instanceIdOption_;
        std::wstring const moduleOption_;
        std::wstring const segmentHoursOption_;
        std::wstring const segmentMinutesOption_;
        std::wstring const fileCountOption_;
        std::wstring const segmentSizeOption_;
    };
}
