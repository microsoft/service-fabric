// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class TestVerifier
    {
    public:
        TestVerifier() :
          expectationResetEvent_(true)
        {
        }

        void Expect(std::wstring const & message);
        bool Validate(std::wstring const & message, bool ignoreUnmatched = false);
        bool WaitForExpectationEquilibrium(Common::TimeSpan timeout);
        void Clear(int);
        void Clear();

    private:
        std::vector<std::wstring> expecting_;
        std::wstring errors_;
        Common::ExclusiveLock lock_;
        Common::ManualResetEvent expectationResetEvent_;

        bool CheckPendingEvents();
    };
}
