// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "../inc/public/leaselayerinc.h"

#include <ws2tcpip.h> // GetAddrInfoW and FreeAddrInfoW
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LeaseLayerTestCommon
{    
    const Common::StringLiteral TraceType("LeaseLayerTestCommon");

    // ETCM currently cannot make an ETCM case pass/fail determination based on TAEF VERIFY results.
    // This class is a helper for a workaround until the issue is fixed.
    class EtcmResult
    {
        DENY_COPY(EtcmResult);
    public:
        EtcmResult(Common::atomic_long & passCount, Common::atomic_long & failCount)
           : passCount_(passCount),
             failCount_(failCount),
             pass_(false)
        {}        
      
        ~EtcmResult()
        {
            if(pass_)
            {
                ++passCount_;                   
                Trace.WriteInfo(TraceType, "passCount_ is now:{0}", passCount_.load());
            }
            else
            {
                ++failCount_;                
                Trace.WriteInfo(TraceType, "failCount_ is now:{0}", failCount_.load());
            }
        }
       
        void MarkPass() 
        {
            pass_ = true;
        }
        
    private:
        Common::atomic_long & passCount_;
        Common::atomic_long & failCount_;
        bool pass_;
    };

    void PrintLeaseTaefTestSummary(LONG passCount, LONG failCount);      
    
    bool GetAddressesHelper(
        TRANSPORT_LISTEN_ENDPOINT & socketAddress1, 
        TRANSPORT_LISTEN_ENDPOINT & socketAddress2, 
        TRANSPORT_LISTEN_ENDPOINT & socketAddress6_1, 
        TRANSPORT_LISTEN_ENDPOINT & socketAddress6_2,
        bool & foundV4,
        bool & foundV6
        ) ;
  
}
