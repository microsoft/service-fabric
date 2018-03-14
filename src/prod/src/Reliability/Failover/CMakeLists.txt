add_subdirectory(common)
add_subdirectory(fm)
add_subdirectory(ra)

set( LINUX_SOURCES
    ./CacheMode.cpp
    ./CachedServiceTableEntry.cpp
    ./ChangeNotificationOperation.cpp
    ./FederationWrapper.cpp
    ./LookupVersionStoreProvider.cpp
    ./MatchedServiceTableEntry.cpp
    ./NotificationCacheIndex.cpp
    ./ReliabilitySubsystemWrapper.cpp
    ./ReliabilitySubsystem.cpp
    ./ServiceAdminClient.cpp
    ./ServiceResolver.cpp
    ./ServiceResolverImpl.cpp
    ./ServiceResolverCache.cpp
    ./TestApi.Reliability.cpp
)

add_library(objects_Failover OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_Failover Failover.stdafx.h)

add_library(${lib_Failover} STATIC $<TARGET_OBJECTS:objects_Failover> $<TARGET_OBJECTS:objects_FailoverCommon> $<TARGET_OBJECTS:objects_FailoverFM> $<TARGET_OBJECTS:objects_FailoverRA>)

set_target_properties(${lib_Failover} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

