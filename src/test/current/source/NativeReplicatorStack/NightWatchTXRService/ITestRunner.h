// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace NightWatchTXRService
{
    interface ITestRunner
    {
        K_SHARED_INTERFACE(ITestRunner);

    public:
        virtual ktl::Awaitable<PerfResult::SPtr> Run() = 0;

        __declspec(property(get = get_exception)) Data::Utilities::SharedException::CSPtr Exception;
        Data::Utilities::SharedException::CSPtr get_exception() const
        {
            return exceptionSptr_;
        }

        __declspec(property(get = get_exceptionThrown, put = set_exceptionThrown)) bool ExceptionThrown;
        bool get_exceptionThrown() const
        {
            return exceptionThrown_.load();
        }
        void set_exceptionThrown(bool value)
        {
            exceptionThrown_.store(value);
        }

    protected:
        Data::Utilities::SharedException::CSPtr exceptionSptr_;
        Common::atomic_bool exceptionThrown_;
    };
}