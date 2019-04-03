include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_CommonTest}
  # boost.test main
  ../../../test/BoostUnitTest/btest.cpp

  # test code
  ../AccountHelper.Test.cpp
  ../ApiMonitoring.MonitoringComponent.Test.cpp
  ../Assert.Test.cpp
  ../AsyncExclusiveLock.test.cpp
  ../AsyncFabricComponent.Test.cpp
  ../AsyncOperation.Test.cpp
  ../AsyncWaitHandle.test.cpp
  ../AsyncWorkJobQueue.Test.cpp
  ../Atomics.Test.cpp
  ../Bique.Test.cpp
  ../CachedFile.Test.cpp
  ../CertDirMonitor.Test.cpp
  ../ComPointer.Test.cpp
  ../ComUnknownBase.Test.cpp
  ../ComponentRoot.Test.cpp
  ../Config.Test.cpp
  ../crc.test.cpp
  ../CryptoUtility.Test.cpp
  ../DateTime.Test.cpp
  ../Directory.test.cpp
  ../EmbeddedList.test.cpp
  ../Endpoint.Test.cpp
  ../Environment.Test.cpp
  ../ErrorCode.Test.cpp
  ../EventHandler.Test.cpp
  ../ExclusiveFile.Test.cpp
  ../ExpiringSet.Test.cpp
  ../Expression.cpp
  ../ExpressionParser.Test.cpp
  ../FabricNodeConfig.Test.cpp
  ../FabricSerializationHelper.test.cpp
  ../File.test.cpp
  ../FileChangeMonitor.Test.cpp
  ../FileConfigStore.Test.cpp
  ../FileLock.Test.cpp
  ../Guid.Test.cpp
  ../IntrusiveList.Test.cpp
  ../IntrusivePtr.Test.cpp
  ../IpUtility.Test.cpp
  ../JobQueue.Test.cpp
  ../JsonSerialization.Test.cpp  #Enable after data type fixes
  ../LargeInteger.Test.cpp
  ../LinkableAsyncOperation.Test.cpp
  ../LongPath.Test.cpp
  ../LruCache.test.cpp
  ../LruCacheWaiterList.test.cpp
  ../Math.Test.cpp
  ../MovePointer.test.cpp
  ../MutexHandle.test.cpp
  ../NamingUri.Test.cpp
  ../Parse.Test.cpp
  ../Path.Test.cpp
  ../PerformanceCounter.Test.cpp
  ../ProcessInfo.test.cpp
  ../ReaderQueue.Test.cpp
  ../ScopedHeap.Test.cpp
  ../StackTrace.Test.cpp
  ../StateMachine.Test.cpp
  ../StringResource.Test.cpp
  ../StringUtility.Test.cpp
  ../TextWriter.Test.cpp
  ../Threadpool.Test.cpp
  ../ProcessWait.Test.cpp
  ../Timer.Test.cpp
  ../TimeSpan.Test.cpp
  ../Uri.Test.cpp
  ../VersionRangeCollection.test.cpp
  ../WaitHandle.Test.cpp

#Not yet migrated
# ../Directory.test.cpp
# ../FabricEnvironment.Test.cpp
# ../KtlProxyAsyncOperation.Test.cpp
# ../asyncfile.test.cpp
  )

add_precompiled_header(${exe_CommonTest} ../stdafx.h)

set_target_properties(${exe_CommonTest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_CommonTest}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_ServiceModel}
  ${BoostTest2}
  ${lib_Pal}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricResources}
  ssh2
  ssl
  crypto
  minizip
  z
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
)

install(
    FILES ./Common.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./RunCommonTests.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./UnsupportedUris.txt
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./SupportedUris.txt
    DESTINATION ${TEST_OUTPUT_DIR}
)
