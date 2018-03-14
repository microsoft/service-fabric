// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include <malloc.h>

using namespace std;

namespace Common
{
    BOOST_AUTO_TEST_SUITE2(ProcessInfoTest)

    BOOST_AUTO_TEST_CASE(Basic)
    {
        ENTER;

        auto pmi = ProcessInfo::GetSingleton();
        auto dataSize1 = pmi->RefreshDataSize();
        string summary;
        auto rss1 = pmi->GetRssAndSummary(summary);
        Trace.WriteInfo(TraceType, "{0}", summary);

        void* bs1 = malloc(32);
        VERIFY_IS_TRUE(bs1 != nullptr);
        KFinally([bs1] { free(bs1); });

        void* bs2 = malloc(256);
        VERIFY_IS_TRUE(bs2 != nullptr);
        KFinally([bs2] { free(bs2); });

        void* bs3 = malloc(1024);
        VERIFY_IS_TRUE(bs3 != nullptr);
        KFinally([bs3] { free(bs3); });

        void* bs4 = malloc(4096);
        VERIFY_IS_TRUE(bs4 != nullptr);
        KFinally([bs4] { free(bs4); });

        auto mallInfo = mallinfo();
        size_t largeBufSize = mallInfo.fordblks/* free bytes in heap*/ + 32*1024*1024;
        void* bl = malloc(largeBufSize);
        VERIFY_IS_TRUE(bl != nullptr);
        KFinally([bl] { free(bl); });

        // touch every page to trigger rss/workingSet increase
        char* str = (char*)bl;
        auto pageSize = getpagesize();
        for(uint i = 0; i < largeBufSize; )
        {
            str[i] = (char)i;
            i += pageSize; 
        }

        auto dataSize2 = pmi->RefreshDataSize();
        auto rss2 = pmi->GetRssAndSummary(summary);
        Trace.WriteInfo(TraceType, "rss1= {0}, rss2= {1}", rss1, rss2); 
        Trace.WriteInfo(TraceType, "dataSize1 = {0}, dataSize2 = {1}", dataSize1, dataSize2);

        VERIFY_IS_TRUE(dataSize1 <= dataSize2);

        VERIFY_IS_TRUE(rss1 <= rss2); //assuming swapping is not happening

        Trace.WriteInfo(TraceType, "{0}", summary); 

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()
}
