// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestDispatcher;
    class TestHealthTable;

    class TestWatchDog
    {
        DENY_COPY(TestWatchDog);

    public:
        TestWatchDog(FabricTestDispatcher & dispatcher, std::wstring const & sourceId, Common::TimeSpan reportInterval);

        __declspec(property(get=get_SourceId)) std::wstring const& SourceId;
        std::wstring const& get_SourceId() const { return sourceId_; }

        int64 GetSequence();

        int NextInt(int max)
        {
            return random_.Next(max);
        }

        double NextDouble()
        {
            return random_.NextDouble();
        }

        void Report(FABRIC_HEALTH_REPORT const & report);

        void Start(std::shared_ptr<TestWatchDog> const & thisSPtr);

        void Stop();

    private:
        void Run();

        FabricTestDispatcher & dispatcher_;
        std::shared_ptr<TestHealthTable> healthTable_;
        std::wstring sourceId_;
        Common::TimeSpan reportInterval_;
        Random random_;
        bool active_;
        Common::TimerSPtr timer_;
        ComPointer<IFabricHealthClient4> healthClient_;

        static int64 Sequence;
    };

    typedef std::shared_ptr<TestWatchDog> TestWatchDogSPtr;
}
