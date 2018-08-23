set( SOURCES
  ../InternalKtlLogger.cpp
  ../KtlLogManager.cpp
  ../KtlLogManager.AsyncEnumerateLogs.cpp
  ../KtlLogManager.AsyncCreateLog.cpp
  ../KtlLogManager.AsyncQueryLogId.cpp
  ../KtlLogManager.AsyncOpenLog.cpp
  ../KtlLogManager.AsyncDeleteLog.cpp
  ../KtlLog.cpp
  ../KtlLog.AsyncMountLog.cpp
  ../KtlLog.AsyncDeleteLog.cpp
  ../KtlLog.AsyncCreateLog.cpp
  ../KtlLog.AsyncCreateLogStream.cpp
  ../KtlLog.AsyncOpenLogStream.cpp
  ../KtlLog.AsyncDeleteLogStream.cpp
  ../KtlLogStream.cpp
  ../KtlLogStream.AsyncReadStream.cpp
  ../KtlLogStream.AsyncWriteStream.cpp
  ../KtlLogRecovery.cpp
  ../KtlLogStreamRecovery.cpp
)

include_directories("../../inc")

add_library(objects_KtlLoggerUser OBJECT ${SOURCES})
add_library(${lib_KtlLoggerUser} STATIC $<TARGET_OBJECTS:objects_KtlLoggerUser>)

set_target_properties(${lib_KtlLoggerUser} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

