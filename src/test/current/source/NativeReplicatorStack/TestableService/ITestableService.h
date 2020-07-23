// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestableService
{
    class ITestableService
    {
    public:
        //
        // Start doing work. If previous work is still in progress, this request will be ignored.
        // Derived class needs to provide the implementation of this method. This method will be
        // called by the TestableServiceBase class when it receives 'DoWork' request.
        //
        virtual ktl::Awaitable<void> DoWorkAsync() = 0;

        //
        // Get the progress of current work. Derived class needs to provide implementation of this
        // method. This method will be called by the TestableServiceBase class when it receives
        // 'GetProgress' request
        //
        virtual ktl::Awaitable<TestableServiceProgress> GetProgressAsync() = 0;
    };
}
