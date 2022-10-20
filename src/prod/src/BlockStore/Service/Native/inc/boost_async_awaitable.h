// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "common.h"

using namespace service_fabric;
using boost::asio::ip::tcp;

//
// awaitable wrapper for boost
// @TODO - Move this into reliable_collections.h
//
template <typename T>
class BoostAsyncAwaitable : public async_awaitable<T>
{
public:
    BoostAsyncAwaitable() = default;

protected:
    void set_error(boost::system::error_code err)
    {
        error_ = err;
    }

    virtual void throw_if_error() const
    {
        if (error_)
        {
            throw boost::system::system_error(error_);
        }
    }

    boost::system::error_code get_error()
    {
        return error_;
    }

private:
    boost::system::error_code error_;
    std::string msg_;
};

