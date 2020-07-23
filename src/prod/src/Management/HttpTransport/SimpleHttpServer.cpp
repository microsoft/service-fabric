// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/CryptoUtility.Linux.h"
#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#include <boost/bind.hpp>
#include <iomanip>
#include <src/Transport/TransportSecurity.Linux.h>

using namespace std;
using namespace web;
using namespace utility;
using namespace http;
using namespace web::http::experimental::listener;
using namespace HttpServer;
using namespace Common;

static const StringLiteral TraceType("HttpGateway");

LHttpServer::LHttpServer()
{
}

void LHttpServer::Create(LHttpServer::SPtr& server)
{
    server = std::make_shared<LHttpServer>();
}

bool LHttpServer::VerifyCertificateCallback(bool preverified, boost::asio::ssl::verify_context& ctx)
{
    // this call back will always pass and give cert to application to verify role.
    return true;
}

void LHttpServer::OnInitialize()
{
    StringUtility::Utf16ToUtf8(listenUri_, address_);
    while (listenUri_.find(L"https") == 0)
    {
        FabricNodeConfigSPtr nodeConfig = std::make_shared<FabricNodeConfig>();
        SecurityConfig &securityConfig = SecurityConfig::GetConfig();

        shared_ptr<X509FindValue> findValue;
        auto error = X509FindValue::Create(
            nodeConfig->ServerAuthX509FindType,
            nodeConfig->ServerAuthX509FindValue,
            nodeConfig->ServerAuthX509FindValueSecondary,
            findValue);

        if (!error.IsSuccess()) break;

        CertContextUPtr certContext;
        error = CryptoUtility::GetCertificate(X509Default::StoreLocation(), nodeConfig->ServerAuthX509StoreName, findValue, certContext);
        if (!error.IsSuccess()) break;

        serverCertPath = certContext.FilePath();
        Invariant(!serverCertPath.empty());

        PrivKeyContext privKey;
        error = LinuxCryptUtil().ReadPrivateKey(certContext, privKey);
        if (!error.IsSuccess()) break;

        serverPkPath = privKey.FilePath();
        Invariant(!serverPkPath.empty());

        http_listener_config conf;
        conf.set_ssl_context_callback(
            [this](boost::asio::ssl::context &ctx) {
            try
            {
                ctx.set_options(boost::asio::ssl::context::default_workarounds);
                ctx.use_certificate_chain_file(this->serverCertPath);
                ctx.use_private_key_file(this->serverPkPath, boost::asio::ssl::context::pem);

                if (SecurityConfig::GetConfig().ClientClaimAuthEnabled)
                {
                    ctx.set_verify_mode(boost::asio::ssl::verify_none);
                }
                else
                {
                    ctx.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
                    ctx.set_verify_callback([this](bool p, boost::asio::ssl::verify_context& context)
                    {
                        return this->VerifyCertificateCallback(p, context);
                    });
                }
            }
            catch (boost::system::system_error ec)
            {
                // boost::asio::ssl::context apis can throw boost::system::system_error.
                // Due to a bug in CppRestSdk 2.8, this unobserved exception can terminate the thread and prevent the listener from starting
                // any further async_accepts.
                WriteWarning(TraceType, "LHttpServer Exception while setting ssl context, error code : {0}", ec.code().value());
            }
        });

        m_listenerUptr = make_unique<web::http::experimental::listener::http_listener>(address_, conf);
        m_listenerUptr->support([this](http_request message) { return this->m_all_requests(message); });
        return;
    }
    m_listenerUptr = make_unique<web::http::experimental::listener::http_listener>(address_);
    m_listenerUptr->support([this](http_request message) { return this->m_all_requests(message); });
}

bool LHttpServer::StartOpen(
    const std::wstring& listenUri,
    const RequestHandler &handler)
{
    listenUri_ = listenUri;
    m_all_requests = handler;

    OnInitialize();

    return Open();
}

bool LHttpServer::Open()
{
    m_listenerUptr->open().wait();
    return true;
}

bool LHttpServer::StartClose()
{
    return Close();
}

bool LHttpServer::Close()
{
    m_listenerUptr->close().wait();
    return true;
}
