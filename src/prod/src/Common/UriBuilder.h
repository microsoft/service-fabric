// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class UriBuilder
    {
    public:
        UriBuilder();

        UriBuilder(Uri const & uri);

        void ToUri(Uri & uri);

        __declspec(property(get=get_Scheme, put=set_Scheme)) std::wstring const & Scheme;
        __declspec(property(get=get_Host, put=set_Host)) std::wstring const & Host;
        __declspec(property(get=get_Port, put=set_Port)) int Port;
        __declspec(property(get=get_Path, put=set_Path)) std::wstring const & Path;
        __declspec(property(get=get_Query, put=set_Query)) std::wstring const & Query;
        __declspec(property(get=get_Fragment, put=set_Fragment)) std::wstring const & Fragment;

        std::wstring const & get_Scheme() const { return scheme_; }
        std::wstring const & get_Host() const { return host_; }
        int get_Port() const { return port_; }
        std::wstring const & get_Path() const { return path_; }
        std::wstring const & get_Query() const { return query_; }
        std::wstring const & get_Fragment() const { return fragment_; }

        UriBuilder & set_Scheme(std::wstring const & scheme) { scheme_ = scheme; return *this; }
        UriBuilder & set_Host(std::wstring const & host) { host_ = host; return *this; }
        UriBuilder & set_Port(int port) { port_ = port; return *this; }
        UriBuilder & set_Path(std::wstring const & path) { path_ = path; return *this; }
        UriBuilder & set_Query(std::wstring const & query) { query_ = query; return *this; }
        UriBuilder & set_Fragment(std::wstring const & fragment) { fragment_ = fragment; return *this; }

    private:
        std::wstring scheme_;
        std::wstring host_;
        int port_;
        std::wstring path_;
        std::wstring query_;
        std::wstring fragment_;
    };
}
