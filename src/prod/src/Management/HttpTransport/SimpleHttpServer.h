// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if defined(PLATFORM_UNIX)
#include <asio/ssl/verify_context.hpp>
#endif
namespace web
{
	namespace http
    {
		class http_request;
		class http_response;
		namespace experimental
        { 
			namespace listener
            { 
				class http_listener; 
			}
		}
	}
}

class LHttpServer : public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>

{
public:
    typedef std::function<void(web::http::http_request &)> RequestHandler; 
    typedef std::shared_ptr<LHttpServer> SPtr; 

    LHttpServer();
    static void Create(LHttpServer::SPtr& server);   
    void OnInitialize(); 
    
    bool StartOpen(std::wstring const & listenUri, RequestHandler const & handler);
    bool Open();  
		
    bool Close();
    bool StartClose();
#if defined(PLATFORM_UNIX)
    bool VerifyCertificateCallback(bool preverified, boost::asio::ssl::verify_context& ctx);
#endif
private:
  	std::string address_;  
   	std::wstring listenUri_;
    std::unique_ptr<web::http::experimental::listener::http_listener> m_listenerUptr;
   	RequestHandler m_all_requests;

	std::string serverCertPath;
	std::string serverPkPath;
};
