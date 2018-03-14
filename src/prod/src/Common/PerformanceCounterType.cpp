// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace PerformanceCounterType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case QueueLength : w << L"QueueLength"; return;
            case LargeQueueLength : w << L"LargeQueueLength"; return;
            case QueueLength100Ns : w << L"QueueLength100Ns"; return;
            case RawData32 : w << L"RawData32"; return;
            case RawData64 : w << L"RawData64"; return;  
            case RawDataHex32 : w << L"RawDataHex32"; return;  
            case RawDataHex64 : w << L"RawDataHex64"; return;  
            case RateOfCountPerSecond32 : w << L"RateOfCountPerSecond32"; return;  
            case RateOfCountPerSecond64 : w << L"RateOfCountPerSecond64"; return;  
            case RawFraction32 : w << L"RawFraction32"; return;  
            case RawFraction64 : w << L"RawFraction64"; return;
            case RawBase32 : w << L"RawBase32"; return;  
            case RawBase64 : w << L"RawBase64"; return;  
            case SampleFraction : w << L"SampleFraction"; return;  
            case SampleCounter : w << L"SampleCounter"; return;  
            case SampleBase : w << L"SampleBase"; return;
            case AverageTimer32 : w << L"AverageTimer32"; return;  
            case AverageBase : w << L"AverageBase"; return;  
            case AverageCount64 : w << L"AverageCount64"; return;  
            case PercentageActive : w << L"PercentageActive"; return;  
            case PercentageNotActive : w << L"PercentageNotActive"; return;  
            case PercentageActive100Ns : w << L"PercentageActive100Ns"; return;
            case PercentageNotActive100Ns : w << L"PercentageNotActive100Ns"; return;  
            case ElapsedTime : w << L"ElapsedTime"; return;  
            case MultiTimerPercentageActive : w << L"MultiTimerPercentageActive"; return;  
            case MultiTimerPercentageNotActive : w << L"MultiTimerPercentageNotActive"; return;  
            case MultiTimerPercentageActive100Ns : w << L"MultiTimerPercentageActive100Ns"; return;  
            case MultiTimerPercentageNotActive100Ns : w << L"MultiTimerPercentageNotActive100Ns"; return;  
            case MultiTimerBase : w << L"MultiTimerBase"; return;
            case Delta32 : w << L"Delta32"; return;  
            case Delta64 : w << L"Delta64"; return;  
            case ObjectSpecificTimer : w << L"ObjectSpecificTimer"; return; 
            case PrecisionSystemTimer : w << L"PrecisionSystemTimer"; return;  
            case PrecisionTimer100Ns : w << L"PrecisionTimer100Ns"; return;  
            case PrecisionObjectSpecificTimer : w << L"PrecisionObjectSpecificTimer"; return;
            }

            w << "PerformanceCounterType(" << static_cast<int>(e) << ')';
        }
    }
}
