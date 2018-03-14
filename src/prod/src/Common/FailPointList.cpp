// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <stdafx.h>

using namespace std;

namespace Common
{
    void FailPointList::Set(FailPointCriteriaSPtr const & criteria, FailPointActionSPtr const & action, int priority, std::wstring name)
    {
        AcquireExclusiveLock lock(FailPointList::thisLock);
        failpoints_.push_back(make_unique<FailPoint>(criteria,action,priority,name));
    }

    void FailPointList::Remove(wstring name)
    {
        AcquireExclusiveLock lock(thisLock);
        vector<unique_ptr<FailPoint>>::iterator it;
        for(it=failpoints_.begin(); it<failpoints_.end();it++)
        {
            if(name.size()!=0&&StringUtility::AreEqualCaseInsensitive((*it)->Name, name))
            {
                failpoints_.erase(it);
                return;
            }
        }
    }

    void FailPointList::Remove(FailPointCriteriaSPtr const & criteria, wstring name)
    {
        AcquireExclusiveLock lock(thisLock);
        vector<unique_ptr<FailPoint>>::iterator it;
        for(it=failpoints_.begin(); it<failpoints_.end();it++)
        {
            if(name.size()!=0&&StringUtility::AreEqualCaseInsensitive((*it)->Name, name))
            {
                failpoints_.erase(it);
                return;
            }

            if(name.size()==0&&(*it)->CriteriaSPtr==criteria)
            {
                failpoints_.erase(it);
                return;
            }
        }
    }

    bool FailPointList::Check(FailPointContext context, bool invokeAction,wstring name)
    {
        vector<unique_ptr<FailPoint>>::iterator it;
        {
            AcquireExclusiveLock lock(thisLock);
            for(it=failpoints_.begin(); it<failpoints_.end();it++)
            {
                if(name.size()!=0&&StringUtility::AreEqualCaseInsensitive((*it)->Name, name))
                {
                    auto criteria=(*it)->CriteriaSPtr;
                    if(!criteria ||criteria->Match(context))
                    {
                        break;
                    }
                }

                if(name.size()==0&&(*it)->Name.size()==0&&(*it)->CriteriaSPtr->Match(context))
                {
                    break;
                }
            }
        }

        if(it==failpoints_.end())
        {
            return false;
        }

        if(invokeAction)
        {
            return (*it)->ActionSPtr->Invoke(context);
        }

        return true;
    }

    void FailPointList::RemoveAll()
    {
        AcquireExclusiveLock lock(thisLock);
        failpoints_.clear();
    }

    bool FailPointList::CheckAllResults()
    {
        AcquireExclusiveLock lock(thisLock);
        vector<unique_ptr<FailPoint>>::iterator it;
        for(it=failpoints_.begin();it!=failpoints_.end();++it)
        {
            if(!(*it)->get_Action()->CheckResult())
            {
                return false;
            }
        }

        return true;
    }
}
