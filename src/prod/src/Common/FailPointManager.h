// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FailPointManager
    {
    public: 

        static void Set(FailPointCriteriaSPtr const & criteria, FailPointActionSPtr const & action, int priority=0, std::wstring name=std::wstring())
        {
            FailPointManager::list_.Set(criteria,action,priority,name);
        }

        static void Remove(std::wstring name)
        {
            FailPointManager::list_.Remove(name);
        }

        static void Remove(FailPointCriteriaSPtr const & criteria,std::wstring name=std::wstring())
        {
            FailPointManager::list_.Remove(criteria,name);
        }

        static void Remove(const FailPoint& failpoint)
        {
            FailPointManager::Remove(failpoint.CriteriaSPtr);
        }

        static bool Check(FailPointContext context, bool invokeAction,std::wstring name=std::wstring())
        {
            return FailPointManager::list_.Check(context,invokeAction,name);
        }

        static bool Check(FailPointContext context,std::wstring name=std::wstring())
        {
            return FailPointManager::Check(context,true,name);
        }

        static bool CheckAllResults()
        {
            return FailPointManager::list_.CheckAllResults();
        }

        static void RemoveAll()
        {
            return FailPointManager::list_.RemoveAll();
        }
        
    private:
        static FailPointList list_;
    };
};
