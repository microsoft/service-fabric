// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // A simple wrapper type to Windows HANDLE type to support RAII and 
    // sharing of the handles between multiple parts of the program
    class Handle :
        Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
        DENY_COPY(Handle);

    public:
            struct DUPLICATE{};

            explicit Handle(HANDLE hValue) : hValue_(hValue) { }

            explicit Handle(Handle && other) : hValue_(other.hValue_) 
            {
                other.hValue_ = nullptr;
            }

            Handle(Handle const & other, DUPLICATE)
                : hValue_(nullptr)
            {
                if (other.Value)
                {
#if !defined(PLATFORM_UNIX)
                    if(!::DuplicateHandle(
                            ::GetCurrentProcess(),
                            other.Value,
                            ::GetCurrentProcess(),
                            &hValue_,
                            0,
                            false,
                            DUPLICATE_SAME_ACCESS))
                    {
                        WriteWarning(
                            "Handle",
                            "DuplicateHandle failed for {0}. Reason: {1}",
                            other.Value,
                            ::GetLastError());
                    }
#else
                    hValue_ = other.Value;
#endif
                }
            }

            Handle(HANDLE hValue, DUPLICATE)
                : hValue_(nullptr)
            {
                if (hValue)
                {
#if !defined(PLATFORM_UNIX)
                    if(!::DuplicateHandle(
                            ::GetCurrentProcess(),
                            hValue,
                            ::GetCurrentProcess(),
                            &hValue_,
                            0,
                            false,
                            DUPLICATE_SAME_ACCESS))
                    {
                        WriteWarning(
                            "Handle",
                            "DuplicateHandle failed for {0}. Reason: {1}",
                            hValue,
                            ::GetLastError());
                    }
#else
                    hValue_ = hValue;
#endif
                }
            }

            ~Handle()
            {
                if (hValue_)
                {
                    if (!::CloseHandle(hValue_))
                    {
                        WriteWarning(
                            "Handle",
                            "CloseHandle failed for {0}. Reason: {1}",
                            hValue_,
                            ::GetLastError());
                    }
                }
            }
        
            __declspec(property(get=get_Value)) HANDLE Value;
            HANDLE get_Value() const { return hValue_; }

            void clear_Value() { hValue_ = nullptr; }

        private:
            HANDLE hValue_;
    };

    typedef std::unique_ptr<Handle> HandleUPtr;
    typedef std::shared_ptr<Handle> HandleSPtr;
}
