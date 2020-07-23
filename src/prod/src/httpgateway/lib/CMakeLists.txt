set( LINUX_SOURCES
    ../ApplicationsHandler.cpp             
    ../ApplicationsResourceHandler.cpp
    ../ApplicationTypesHandler.cpp         
    ../ApplicationUpgradeProgress.cpp      
    ../ClusterManagementHandler.cpp        
    ../Constants.cpp                       
    ../ComposeDeploymentsHandler.cpp
    ../VolumesHandler.cpp
    ../GatewaysResourceHandler.cpp
    ../ErrorBody.cpp
    ../FabricClientWrapper.cpp             
    ../FabricUpgradeProgress.cpp         
    ../FabricOrchestrationUpgradeProgress.cpp
    ../GatewayUri.cpp
    ../HandlerUriTemplate.cpp                    
    ../HttpAuthHandler.AccessCheckAsyncOperation.cpp
    ../HttpAuthHandler.cpp                          
    ../HttpCertificateAuthHandler.cpp               
    ../HttpClaimsAuthHandler.cpp                    
    ../HttpGatewayConfig.cpp                        
    #../HttpKerberosAuthHandler.cpp                  
    ../HttpServer.AccessCheckAsyncOperation.cpp     
    ../HttpServer.CloseAsyncOperation.cpp           
    ../HttpServer.cpp                             
    ../HttpServer.CreateAndOpenAsyncOperation.cpp 
    ../imagestorehandler.cpp
    ../HttpServer.OpenAsyncOperation.cpp   
    ../NetworksHandler.cpp     
    ../NodesHandler.cpp
    ../RequestHandlerBase.cpp
    ../RequestHandlerBase.HandlerAsyncOperation.cpp
    ../stdafx.cpp
    ../UriArgumentParser.cpp
    ../SecretsResourceHandler.cpp
    ../InvokeQuorumLossProgress.cpp
    ../InvokeDataLossProgress.cpp
    ../RestartPartitionProgress.cpp
    ../ToolsHandler.cpp
    ../FaultsHandler.cpp
    ../NodeTransitionProgress.cpp
    ../NamesHandler.cpp
    )

include_directories("..")

add_library(objects_HttpGateway OBJECT ${LINUX_SOURCES} )

add_precompiled_header(objects_HttpGateway ../stdafx.h)

add_library(${lib_httpgateway} STATIC $<TARGET_OBJECTS:objects_HttpGateway>)

target_link_libraries( ${lib_httpgateway}
    ssl crypto 
    )

set_target_properties(${lib_httpgateway} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR} )

