//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Common
{
    /*
        Counts the number of threads that are running code in FabricCommon

        The functions in this class OnThreadEnter/OnThreadExit are called 
        from a DllMain and must respect those semantics
    */
    class ThreadCounter
    {
    public:
        __declspec(property(get = get_ThreadCount)) int ThreadCount;
        int get_ThreadCount() const { return count_; }

        // Sets the number of threads after 
        // which any thread would cause the process to immediately fail fast
        void SetThreadTestLimit(int limit);
        
        void OnThreadEnter();

        void OnThreadExit();

    private:
        void FailFastIfThreadLimitHit();

        void InitializeThreadCount();

        int count_ = -1;
        int limit_ = -1;
    };
}