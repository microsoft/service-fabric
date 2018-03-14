// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Store
{
    StringLiteral const TraceComponent("FabricTimeData");
    
    FabricTimeData::FabricTimeData():logicalTimeStamp_(0)
    {
        WriteNoise(TraceComponent,"{0} constructor",this->TraceId);
    }

    FabricTimeData::FabricTimeData(int64 logicalTimeStamp):logicalTimeStamp_(logicalTimeStamp)
    {
        WriteNoise(TraceComponent,"{0} constructor",this->TraceId);
    }

    FabricTimeData::~FabricTimeData()
    {
        WriteNoise(TraceComponent,"{0} ~dtor",this->TraceId);
        inMemoryStopWatch_.Stop();
    }

    void FabricTimeData::Start()
    {
        inMemoryStopWatch_.Start();
    }

    int64 FabricTimeData::RefreshAndGetCurrentStoreTime()
    {
        //update logical timer in memory
        logicalTimeStamp_+=inMemoryStopWatch_.ElapsedTicks;
        inMemoryStopWatch_.Restart();
        return logicalTimeStamp_;
    }

    std::wstring const & FabricTimeData::get_Type() const
    {
        return Constants::FabricTimeDataType;
    }
    std::wstring FabricTimeData::ConstructKey() const
    {
        return Constants::FabricTimeDataKey;
    }

    void FabricTimeData::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.Write("FabricTimeData: logicalTimeStamp:{0}, StopWatch:{1}",logicalTimeStamp_,inMemoryStopWatch_.get_ElapsedTicks());
    }

}
