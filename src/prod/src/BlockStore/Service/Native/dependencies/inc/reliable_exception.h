// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <stdexcept>
#include <string>
#include "servicefabric.h"

using namespace std;

namespace service_fabric
{
    class reliable_exception : public std::runtime_error
    {
    public:
        reliable_exception(HRESULT status, const char *msg)
            :runtime_error(msg), _status(status)
        {
        }

    public:
        HRESULT code()
        {
            return _status;
        }

        static void throw_error(HRESULT error, const char *msg)
        {
            std::string error_msg = msg + get_error(error);
            throw reliable_exception(error, error_msg.c_str());
        }

        static string get_error(HRESULT error)
        {
            // @TODO - This should be supplied by our C API
            return ": " + std::to_string(error);
        }

    private:
        HRESULT _status;
    };
}

