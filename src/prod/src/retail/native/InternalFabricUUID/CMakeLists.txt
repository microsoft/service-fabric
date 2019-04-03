set( LINUX_SOURCES
    ../../../prebuilt/internal/FabricCommon__i.c
    ../../../prebuilt/internal/FabricClient__i.c
    ../../../prebuilt/internal/FabricFaultAnalysisService__i.c
    ../../../prebuilt/internal/FabricImageStore__i.c
    ../../../prebuilt/internal/FabricInfrastructureService__i.c
    ../../../prebuilt/internal/FabricServiceCommunication__i.c
	../../../prebuilt/internal/FabricTransport__i.c
    ../../../prebuilt/internal/FabricTokenValidationService__i.c
    ../../../prebuilt/internal/FabricRuntime__i.c
    ../../../prebuilt/internal/FabricReliableMessaging__i.c
    ../../../prebuilt/internal/KPhysicalLog__i.c
    ../../../prebuilt/internal/FabricContainerActivatorService__i.c
    ../../../prebuilt/internal/FabricGatewayResourceManager__i.c
)

add_library(objects_InternalFabricUUID OBJECT ${LINUX_SOURCES})

add_library(${lib_InternalFabricUUID} STATIC $<TARGET_OBJECTS:objects_InternalFabricUUID>)

set_target_properties(${lib_InternalFabricUUID} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

