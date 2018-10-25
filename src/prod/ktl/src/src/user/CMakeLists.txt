#
# Only enable this when you want to enable allocation tracking on retail build
#
#add_definitions(-DTRACK_ALLOCATIONS)

set( CORE_SOURCES
    ../knt.cpp
    ../kevent.cpp
    ../kspinlock.cpp
    ../kallocator.cpp
    ../KtlCoreAllocator.cpp
    ../KtlSystemCoreImp.cpp
    ../kmemchannel.cpp
    ../ktlpal.cpp
    ../wc16.c
   )

set( FULL_SOURCES
    ../kbuffer.cpp
    ../knodetable.cpp
    ../ktlbase.cpp
    ../KtlSystemBase.cpp
    ../KPointer.cpp
    ../kwstring.cpp
    ../kchecksum.cpp
    ../kvariant.cpp
    ../kawipcclient.cpp
    ../kawipcclientpipe.cpp
    ../kawipcclientserverconnection.cpp
    ../kawipceventwaiter.cpp
    ../kawipcheap.cpp
    ../kawipclifolist.cpp
    ../kawipcpipe.cpp
    ../kawipcringconsumer.cpp
    ../kawipcringproducer.cpp
    ../kawipcring.cpp
    ../kawipcserver.cpp
    ../kawipcserverclientconnection.cpp
    ../kawipcserverpipe.cpp
    ../kawipcserverpipeendpoint.cpp
    ../kawipcsharedmemory.cpp

    ../ksynchronizer.cpp
    ../kstringview.cpp
    ../KTrace.cpp
    ../kiobuffer.cpp
    ../KAsyncContext.cpp
    ../knt.cpp
    ../KAsyncEvent.cpp
    ../KAsyncService.cpp
    ../kvolumenamespace.cpp
    ../kuri.cpp
    ../kvariant.cpp
    ../kserial.cpp

    ../KQuotaGate.cpp
    ../kblockfile.cpp
    ../kbitmap.cpp
    ../kthread.cpp
    ../KTimer.cpp
    ../ksemaphore.cpp
    ../KTextFile.cpp
    ../KBlockDevice.cpp
    ../kcachedblockfile.cpp
    ../krangelock.cpp
    ../kreadcache.cpp

    ../kinstrumentop.cpp
    ../kperfcounter.cpp

    ../KXmlParser.cpp
    ../KDom.cpp
    ../kdompath.cpp
    ../KDomPipe.cpp

    ../kresourcelock.cpp
    ../ktask.cpp

    ../KAsyncService.cpp
    ../ProbeAndLock.c

    ../KTpl.cpp
    ../kfilestream.cpp

    # KXM APIs used by kthreadpool
    ../kxm/user/KXMApi.cpp
    ../kxm/user/KXMApiUser.cpp
    ../kxm/user/KXMApiKernel.cpp

    ../kthreadpool.cpp

    # Threadpool
    ../Threadpool/Win32ThreadpoolWrapper.cpp
    ../Threadpool/Win32ThreadpoolConfig.cpp
    ../Threadpool/Threadpool.cpp
    ../Threadpool/ThreadpoolRequest.cpp
    ../Threadpool/HillClimbing.cpp
    ../Threadpool/MinPal.cpp
    ../Threadpool/UnfairSemaphore.cpp

    ../knetworkendpoint.cpp
    ../kregistry.cpp
    ../palio.cpp
)

set( SOURCES
    ../kwebsocket.cpp
    ../khttp.cpp
    ../khttpserver.cpp
    ../khttpclient.cpp
    ../knetwork.cpp
    ../knetfaultinjector.cpp
    ../ksocket.cpp
    ../knetchannel.cpp
    ../kalpc.cpp
    ../kmgmt.cpp
    ../kmgmtserver.cpp
    ../kssl.cpp
    ../kKerberos.cpp
    ../ksspi.cpp
    ../KSparseFile.cpp
)

include_directories("../../inc")
include_directories("../kxm/common/")
include_directories("../kxm/user/inc")

################################### KTL CORE ##################################
add_library(objects_ktlcore OBJECT ${CORE_SOURCES})
target_compile_definitions(objects_ktlcore PUBLIC KTL_CORE_LIB)
add_library(${lib_ktlcore} STATIC $<TARGET_OBJECTS:objects_ktlcore>)

set_target_properties(${lib_ktlcore} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

################################### KTL FULL ##################################
add_library(objects_ktlfull OBJECT ${FULL_SOURCES})
target_compile_definitions(objects_ktlfull PUBLIC KTL_FULL_LIB)
add_library(${lib_ktlfull} STATIC $<TARGET_OBJECTS:objects_ktlcore>
                                  $<TARGET_OBJECTS:objects_ktlfull>)

target_link_libraries(${lib_ktlfull} rt uuid)
set_target_properties(${lib_ktlfull} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

