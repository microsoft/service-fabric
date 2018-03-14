// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class OperationStatusMap
    {
        DENY_COPY(OperationStatusMap)

    public:
        OperationStatusMap();

        void Initialize(std::wstring const & id);
        bool TryInitialize(std::wstring const & id, __out OperationStatus & initializedStatus);

        OperationStatus Get(std::wstring const & id) const;
        bool TryGet(std::wstring const & id, __out OperationStatus & status) const;

        void SetState(std::wstring const & id, OperationState::Enum const & state);
        void SetFailureCount(std::wstring const & id, ULONG const failureCount);
        void SetError(std::wstring const & id, Common::ErrorCode const & error);
        void Set( std::wstring const & id, 
            Common::ErrorCode const & error, 
            OperationState::Enum const & state = OperationState::Completed, 
            ULONG const failureCount = 0,
            ULONG const internalFailureCount = 0);
        void Set( std::wstring const & id, OperationStatus const status);

        bool TryRemove(std::wstring const & id, __out OperationStatus & status);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        // TODO: Convert this to Structured Tracing completely
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {            
            std::string format = "OperationStatusMap: {0}";

            size_t index = 0;
            traceEvent.AddEventField<std::wstring>(format, name + ".info1", index);
            
            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {           
            context.WriteCopy<std::wstring>(Common::wformatString(*this));
        }

    private:
        mutable Common::RwLock lock_;
        std::map<std::wstring, OperationStatus, Common::IsLessCaseInsensitiveComparer<std::wstring>> map_;
    };
}
