// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class IConfigUpdateSink
    {
    public:
        virtual ~IConfigUpdateSink() = 0 {}

        virtual const std::wstring & GetTraceId() const = 0;
        virtual bool OnUpdate(std::wstring const & section, std::wstring const & key) = 0;
        virtual bool CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted) = 0;
    };

    typedef std::shared_ptr<IConfigUpdateSink> IConfigUpdateSinkSPtr;
    typedef std::weak_ptr<IConfigUpdateSink> IConfigUpdateSinkWPtr;
    typedef std::vector<IConfigUpdateSinkWPtr> IConfigUpdateSinkList;
}
