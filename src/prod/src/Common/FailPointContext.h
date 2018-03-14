// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FailPointContext
    {
        typedef std::map<std::wstring, void*> ContextMap;

    public:
        FailPointContext()
            :properties_()
        {
        }

        bool Contains(std::wstring const & key) const
        {
            return properties_.find(key)!=properties_.end();
        }

        void* GetProperty (std::wstring const & name)
        {
            if(Contains(name))
            {
                return properties_[name];
            }
            else
            {
                return nullptr;
            }
        }

        void AddProperty(std::wstring const & name, void* value)
        {
            properties_[name]=value;
        }
        
    private:
        ContextMap properties_;
    };
};
