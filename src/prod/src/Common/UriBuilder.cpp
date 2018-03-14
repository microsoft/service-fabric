// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <string>
#include <sstream>

using namespace std;

namespace Common
{
    UriBuilder::UriBuilder()
        : scheme_()
        , host_()
        , port_(-1)
        , path_()
        , query_()
        , fragment_()
    {
    }

    UriBuilder::UriBuilder(Uri const & uri)
        : scheme_(uri.Scheme)
        , host_(uri.Host)
        , port_(uri.Port)
        , path_(uri.Path)
        , query_(uri.Query)
        , fragment_(uri.Fragment)
    {
    }

    void UriBuilder::ToUri(Uri & uri)
    {
        if (-1 == port_)
        {
            uri = Uri(
                scheme_,
                host_,
                path_,
                query_,
                fragment_);
        }
        else
        {
            wstringstream portString;
            portString << port_;
            
            uri = Uri(
                scheme_,
                host_ + L":" + portString.str(),
                path_,
                query_,
                fragment_);
        }
    }
}
