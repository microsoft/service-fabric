// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IClientConnection : public ICommunicationMessageSender
    {
    public:
        virtual std::wstring const & Get_ClientId() const = 0;
        virtual ~IClientConnection() {};
    };

}
