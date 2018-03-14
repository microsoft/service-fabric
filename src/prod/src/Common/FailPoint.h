// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    typedef std::shared_ptr<IFailPointCriteria> FailPointCriteriaSPtr;
    typedef std::shared_ptr<IFailPointAction> FailPointActionSPtr;

    class FailPoint
    {
        DENY_COPY(FailPoint);
    public:
        __declspec(property(get=get_CriteriaSPtr)) FailPointCriteriaSPtr CriteriaSPtr;
        __declspec(property(get=get_Action)) FailPointActionSPtr ActionSPtr;
        __declspec(property(get=get_Name)) std::wstring Name;

        FailPointCriteriaSPtr get_CriteriaSPtr(void) const { return criteriaSPtr_;}
        FailPointActionSPtr get_Action(void) const {return actionSPtr_;}
        std::wstring get_Name(void) const {return name_;}

        FailPoint()
            :criteriaSPtr_(),
            actionSPtr_(),
            priority_(0)
        {
        }

        FailPoint(FailPointActionSPtr action,std::wstring name)
            :criteriaSPtr_(),
            actionSPtr_(action),
            name_(name)
        {
        }

        FailPoint(FailPointCriteriaSPtr criteria,FailPointActionSPtr action, int priority=0,std::wstring name=std::wstring())
            :criteriaSPtr_(criteria),
            actionSPtr_(action),
            priority_(priority),
            name_(name)
        {
        }

        virtual std::wstring ToString()
        {
            return L"name "+name_+L"criteria: "+criteriaSPtr_.get()->ToString()+L"action "+actionSPtr_.get()->ToString();
        }
        
    private:
        std::wstring name_;
        FailPointCriteriaSPtr criteriaSPtr_;
        int priority_;
        FailPointActionSPtr actionSPtr_;
    };
};

