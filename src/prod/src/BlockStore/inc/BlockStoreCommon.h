// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace std;

namespace BlockStore
{
    class StringUtil
    {
    public:
        // Support T=u16string/wstring/string
        template <class T>
        static string ToString(T stringToProcess, bool removeTrailingNull = false)
        {
            string str(stringToProcess.begin(), stringToProcess.end());
            return removeTrailingNull ? RemoveTrailingNull(str) : str;
        }

    private:
        // Remove possible ending '\0'
        static string RemoveTrailingNull(string str)
        {
            while (str.size() > 0 && str.back() == '\0')
            {
                str.pop_back();
            }
            return str;
        }
    };
}