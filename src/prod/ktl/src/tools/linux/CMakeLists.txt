set (NUGET_OUTPUT_DIR ${PROJECT_BINARY_DIR}/nuget CACHE STRING "Nuget Output directory")
set (NUGET_CONTENT_DIR ${NUGET_OUTPUT_DIR}/content CACHE STRING "Nuget Content directory")
set (NUGET_CONTENT_INC_DIR ${NUGET_CONTENT_DIR}/inc CACHE STRING "Nuget Content Inc directory")
set (NUGET_CONTENT_KXM_DIR ${NUGET_CONTENT_DIR}/kxm CACHE STRING "Nuget Content Kxm directory")
set (KXM_MODULE_SOURCE  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/kxm CACHE STRING "KXM source code")


FILE(GLOB IncHeaderFiles ../../inc/*.h)
install(
    FILES ${IncHeaderFiles}
    DESTINATION ${NUGET_CONTENT_INC_DIR}
)
install(
    FILES ../../linuxinc/kstatus.h ../../linuxinc/paldef.h ../../linuxinc/palio.h ../../linuxinc/palundef.h
    DESTINATION ${NUGET_CONTENT_INC_DIR}
)
install(
    FILES ../../logger/src/InternalKtlLogger.h  ../../logger/src/KtlLogRecovery.h
    DESTINATION ${NUGET_CONTENT_INC_DIR}/
)
install(
    FILES ../../linuxinc/experimental/resumable
    DESTINATION ${NUGET_CONTENT_INC_DIR}/linux-coroutine/experimental/
)
install(
    FILES ${LIBRARY_OUTPUT_DIR}/libKmUnitUser.a
          ${LIBRARY_OUTPUT_DIR}/libktllttng.a
          ${LIBRARY_OUTPUT_DIR}/libktlcore.a
          ${LIBRARY_OUTPUT_DIR}/libktlfull.a
          ${LIBRARY_OUTPUT_DIR}/libKtlLoggerUser.a
    DESTINATION ${NUGET_CONTENT_DIR}/
)

#KXM Source code
#FILE(GLOB KxmSrcFiles ${KXM_MODULE_SOURCE}/driver/src/*.c)
#install(
    #FILES ${KxmSrcFiles}
    #DESTINATION ${NUGET_CONTENT_KXM_DIR}/src/
#)

#FILE(GLOB KxmIncFiles ${KXM_MODULE_SOURCE}/driver/inc/*.h)
#install(
    #FILES ${KxmIncFiles}
    #DESTINATION ${NUGET_CONTENT_KXM_DIR}/inc/
#)

#FILE(GLOB KxmIncCommonFiles ${KXM_MODULE_SOURCE}/common/*.h)
#install(
#    FILES ${KxmIncCommonFiles}
#    DESTINATION ${NUGET_CONTENT_KXM_DIR}/common/
#)

#install(
#    FILES ${KXM_MODULE_SOURCE}/driver/Makefile 
#    DESTINATION ${NUGET_CONTENT_KXM_DIR}/
#)

#KXM user header files.
#install(
#    FILES ${KXM_MODULE_SOURCE}/common/KUShared.h ${KXM_MODULE_SOURCE}/user/inc/KXMApi.h
#    DESTINATION ${NUGET_CONTENT_INC_DIR}
#)

#install(
#    FILES ${KXM_MODULE_SOURCE}/driver/InstallKxm.sh
#    DESTINATION ${NUGET_CONTENT_KXM_DIR}/
#    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
#)

#install(
#    FILES ./WinFab.Ktl.Linux.nuspec
#    DESTINATION ${NUGET_OUTPUT_DIR}
#)

#install(
#    CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_nugetspec.sh ${CMAKE_CURRENT_SOURCE_DIR}/../../../.config/.inc/versions.xml ${CMAKE_BUILD_TYPE} ${NUGET_OUTPUT_DIR}/WinFab.Ktl.Linux.nuspec)"
#)

#install(
#    CODE "execute_process(COMMAND nuget pack ${NUGET_OUTPUT_DIR}/WinFab.Ktl.Linux.nuspec WORKING_DIRECTORY ${NUGET_OUTPUT_DIR})"
#)
