// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class FabricTimeData : public StoreData
    {
        DENY_COPY(FabricTimeData);
    public:
        FabricTimeData();
        FabricTimeData(int64 logicalTimeStamp);
        void Start();
        virtual ~FabricTimeData();        
        int64 RefreshAndGetCurrentStoreTime();
        virtual std::wstring const & get_Type() const;
        virtual std::wstring ConstructKey() const;
        virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const ;

        FABRIC_FIELDS_01(logicalTimeStamp_);
    private:
        int64 logicalTimeStamp_;
        Common::Stopwatch inMemoryStopWatch_;
    };

    class FabricTimeController :public Common::TextTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(FabricTimeController);
    public:
        FabricTimeController(__in ReplicatedStore & replicatedStore);
        virtual ~FabricTimeController();
        int64 GetCurrentStoreTime();
        void RefreshAndPersist();
    private:
        ReplicatedStore & replicatedStore_;
        FabricTimeData fabricTimeData_;
        Common::TimerSPtr refreshTimer_;
    };
}
