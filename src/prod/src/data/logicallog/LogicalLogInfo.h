// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class LogicalLogInfo
        {

        public:

            __declspec(property(get = get_LogicalLog, put = put_LogicalLog)) KWeakRef<LogicalLog>::SPtr LogicalLog;
            KWeakRef<Data::Log::LogicalLog>::SPtr get_LogicalLog() const { return logicalLog_; }
            void put_LogicalLog(__in KWeakRef<Data::Log::LogicalLog>::SPtr logicalLog) { logicalLog_ = logicalLog; }
            
            __declspec(property(get = get_UnderlyingStream)) KtlLogStream::SPtr  UnderlyingStream;
            KtlLogStream::SPtr  get_UnderlyingStream() const { return underlyingStream_; }

            LogicalLogInfo()
                : logicalLog_(nullptr)
                , underlyingStream_(nullptr)
            {
            }

            LogicalLogInfo(Data::Log::LogicalLog& logicalLog, KtlLogStream& underlyingStream)
                : logicalLog_(logicalLog.GetWeakRef())
                , underlyingStream_(&underlyingStream)
            {
            }

        private:

            KWeakRef<Data::Log::LogicalLog>::SPtr logicalLog_;
            KtlLogStream::SPtr underlyingStream_;
        };
    }
}
