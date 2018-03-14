// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    namespace OperationState
    {
        enum Enum
        {
            Unknown = 0,
            NotStarted = 1,
            InProgress = 2,
            Completed = 3,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    };

    struct OperationStatus
    {
        OperationStatus();
        OperationStatus(std::wstring const & id);
        OperationStatus(OperationStatus const & other);
        OperationStatus(OperationStatus && other);
        OperationStatus & operator = (OperationStatus const & other);
        OperationStatus & operator = (OperationStatus && other);
            
         __declspec(property(get=get_LastError, put=put_LastError)) Common::ErrorCode LastError;
        Common::ErrorCode const & get_LastError() const { return lastError_; }
        void put_LastError(Common::ErrorCode const & value) { lastError_.ReadValue(); value.ReadValue(); lastError_ = value; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        OperationState::Enum State;
        // FailureCount leads to disable of Service type once it is more than the ServiceTypeDisableFailureThreshold.
        ULONG FailureCount;
        std::wstring Id;
        // InternalFailureCount is not treated as real failure for a max of MaxRetryOnInternalError. After which it is a real failure(FailureCount) and will lead to disable of the service type.
        ULONG InternalFailureCount;

    private:
        Common::ErrorCode lastError_;
    };
}
