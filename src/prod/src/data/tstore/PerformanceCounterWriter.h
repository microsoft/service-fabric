// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class PerformanceCounterWriter
        {
        protected:
            PerformanceCounterWriter(__in Common::PerformanceCounterData * counterData);

            __declspec(property(get = get_Value, put = set_Value)) Common::PerformanceCounterValue Value;
            Common::PerformanceCounterValue get_Value() const { return counterData_->Value; }
            void set_Value(Common::PerformanceCounterValue value) { counterData_->Value = value; }

            bool IsEnabled();

        private:
            Common::PerformanceCounterData * counterData_;
        };
    }
}