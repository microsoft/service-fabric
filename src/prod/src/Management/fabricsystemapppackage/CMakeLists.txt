# The system applications manifest. Please see individual manifest files for the ones specific to .prod and .test

set (FABRIC_SYSTEM_APP_SRC_DIR __fabricsystem_app4294967295 CACHE STRING "Fabric System Applications source directory")
set (FABRIC_SYSTEM_APP_DST_DIR __FabricSystem_App4294967295 CACHE STRING "Fabric System Applications destination directory")

# Here, we copy files only to TEST_OUTPUT_DIR. The same files are also copied to FabricDrop/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295
# in script /src/prod/linuxsetup/CMakeLists.txt.
# Why are they not copied here itself to FabricDrop?
# FabricDrop runs as a separate target in the other make file towards the end of the build while this script runs much earlier. This is to run
# FabricDrop target separately independent of mixing building/binplacing operations with FabricDrop. Therefore the dependencies will not be 
# correct if we add copying to FabricDrop in this script.
install(
    FILES
    ${FABRIC_SYSTEM_APP_SRC_DIR}/FAS.Manifest.Current.xml
    ${FABRIC_SYSTEM_APP_SRC_DIR}/FAS.Package.1.0.xml

    ${FABRIC_SYSTEM_APP_SRC_DIR}/FileStoreService.Manifest.Current.xml
    ${FABRIC_SYSTEM_APP_SRC_DIR}/FileStoreService.Package.1.0.xml

    ${FABRIC_SYSTEM_APP_SRC_DIR}/RM.Manifest.Current.xml
    ${FABRIC_SYSTEM_APP_SRC_DIR}/RM.Package.1.0.xml

    ${FABRIC_SYSTEM_APP_SRC_DIR}/TVS.Manifest.Current.xml
    ${FABRIC_SYSTEM_APP_SRC_DIR}/TVS.Package.1.0.xml

    ${FABRIC_SYSTEM_APP_SRC_DIR}/UOS.Manifest.Current.xml
    ${FABRIC_SYSTEM_APP_SRC_DIR}/UOS.Package.1.0.xml

    ${FABRIC_SYSTEM_APP_SRC_DIR}/US.Manifest.Current.xml
    ${FABRIC_SYSTEM_APP_SRC_DIR}/US.Package.1.0.xml

    ${FABRIC_SYSTEM_APP_SRC_DIR}/DnsService.Manifest.Current.xml

    ${FABRIC_SYSTEM_APP_SRC_DIR}/RMS.Manifest.Current.xml

    ${FABRIC_SYSTEM_APP_SRC_DIR}/CSS.Manifest.Current.xml
    ${FABRIC_SYSTEM_APP_SRC_DIR}/CSS.Package.1.0.xml
    
    ${FABRIC_SYSTEM_APP_SRC_DIR}/GRM.Manifest.Current.xml
    ${FABRIC_SYSTEM_APP_SRC_DIR}/GRM.Package.1.0.xml

    DESTINATION ${TEST_OUTPUT_DIR}/${FABRIC_SYSTEM_APP_DST_DIR}
)

# These files are copied separately since the file is renamed while copying
install(
    FILES
    ${FABRIC_SYSTEM_APP_SRC_DIR}/IS.Manifest.Current.linux.xml
    RENAME IS.Manifest.Current.xml
    DESTINATION ${TEST_OUTPUT_DIR}/${FABRIC_SYSTEM_APP_DST_DIR}
)
install(
    FILES
    ${FABRIC_SYSTEM_APP_SRC_DIR}/App.1.0.test.xml
    RENAME App.1.0.xml
    DESTINATION ${TEST_OUTPUT_DIR}/${FABRIC_SYSTEM_APP_DST_DIR}
)
install(
    FILES
    ${FABRIC_SYSTEM_APP_SRC_DIR}/IS.Package.1.0.test.linux.xml
    RENAME IS.Package.1.0.xml
    DESTINATION ${TEST_OUTPUT_DIR}/${FABRIC_SYSTEM_APP_DST_DIR}
)

# exclude FabricIS.sh and copy it like a 'programs' in the next step. Programs also updates the execute permissions on the file.
install(
   DIRECTORY ${WinFab_CoreCLR_Libs_Dir}/FabricIS/IS.Code.Current
   DESTINATION ${TEST_OUTPUT_DIR}/${FABRIC_SYSTEM_APP_DST_DIR}
   PATTERN FabricIS.sh EXCLUDE
)

install(
    PROGRAMS ${WinFab_CoreCLR_Libs_Dir}/FabricIS/IS.Code.Current/FabricIS.sh
    DESTINATION ${TEST_OUTPUT_DIR}/${FABRIC_SYSTEM_APP_DST_DIR}/IS.Code.Current
)

install(
    FILES
    ${FABRIC_SYSTEM_APP_SRC_DIR}/DnsService.Package.1.0.linux.xml
    RENAME DnsService.Package.1.0.xml
    DESTINATION ${TEST_OUTPUT_DIR}/${FABRIC_SYSTEM_APP_DST_DIR}
)

install(
    FILES
    ${FABRIC_SYSTEM_APP_SRC_DIR}/RMS.Package.1.0.test.xml
    RENAME RMS.Package.1.0.xml
    DESTINATION ${TEST_OUTPUT_DIR}/${FABRIC_SYSTEM_APP_DST_DIR}
)
