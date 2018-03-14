// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    template <class THandlerInterface>
    class PackageChangeHandler
    {
        public:
            PackageChangeHandler(
                LONGLONG id, 
                Common::ComPointer<THandlerInterface> const & handler,
                std::wstring const & sourceId)
                : id_(id)
                , handler_(handler)
                , sourceId_(sourceId)
            {
            }

            __declspec(property(get=get_HandleId)) LONGLONG Id;
            LONGLONG get_HandleId() { return id_; }

            __declspec(property(get=get_Handler)) Common::ComPointer<THandlerInterface> const & Handler;
            Common::ComPointer<THandlerInterface> const & get_Handler() { return handler_; }

            __declspec(property(get=get_SourceId)) std::wstring const& SourceId;
            std::wstring const& get_SourceId() { return sourceId_; }

        private:
            LONGLONG id_;
            Common::ComPointer<THandlerInterface> handler_;
            std::wstring sourceId_;
    };
}

