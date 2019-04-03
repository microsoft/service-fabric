install(
    FILES
    UnsupportedTests.list
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./RunTestsManagedLinux.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    DIRECTORY ./../../../test/JsonTests
    DESTINATION ${TEST_OUTPUT_DIR}/..
)

install(
    FILES ./../../src/ServiceModel/xsd/ServiceFabricServiceModel.xsd
    DESTINATION ${TEST_OUTPUT_DIR}/
)

install(
    FILES ./../../ktl/src/test/JsonTests/LinuxKtlUnitTests.json 
    DESTINATION ${TEST_OUTPUT_DIR}/../JsonTests
)


install(CODE "EXECUTE_PROCESS(COMMAND ln ${TEST_OUTPUT_DIR}/ ${TEST_OUTPUT_DIR}/../FabricUnitTests -fs)")
install(CODE "EXECUTE_PROCESS(COMMAND cp -a ${EXTERNAL_DEPS_DIR}/WinFab.Linux.Libs/RunTests/. ${TEST_OUTPUT_DIR}/../RunTests)")
install(CODE "EXECUTE_PROCESS(COMMAND cp -a ${EXTERNAL_DEPS_DIR}/WinFab.CoreCLR.Libs/test/. ${TEST_OUTPUT_DIR}/CoreCLRDlls)")
install(CODE "EXECUTE_PROCESS(COMMAND cp -a ${EXTERNAL_DEPS_DIR}/WinFab.Linux.Libs/RunTests/. ${TEST_OUTPUT_DIR})")
# install(CODE "EXECUTE_PROCESS(COMMAND find ${TEST_OUTPUT_DIR}/dotnet -name dotnet -exec chmod +x {} \;)")
# install(CODE "EXECUTE_PROCESS(COMMAND find ${TEST_OUTPUT_DIR}/dotnet -name corehost -exec chmod +x {} \;)")
# install(CODE "EXECUTE_PROCESS(COMMAND find ${TEST_OUTPUT_DIR}/dotnet -name *.so -exec chmod +x {} \;)")
install(CODE "EXECUTE_PROCESS(COMMAND chmod +x ${TEST_OUTPUT_DIR}/ImageBuilder.sh)")

install(
    FILES
    UnsupportedTests.list
    DESTINATION ${TEST_OUTPUT_DIR}/../RunTests/
)

install(
    FILES ./RunTests.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}/../RunTests/
)

install(
    FILES 
    ./RunTestsV2Settings.json
    DESTINATION ${TEST_OUTPUT_DIR}/../RunTests/
)

install(
    FILES
    ./UploadBuildSettings.json
    DESTINATION ${TEST_OUTPUT_DIR}/../RunTests/
)

install(
    DIRECTORY ./../../../test/JsonTestTypes
    DESTINATION ${TEST_OUTPUT_DIR}/../RunTests
)

install(
     FILES 
        ./../../ktl/src/test/JsonTestTypes/SF/Linux.KConsTest.json
        ./../../ktl/src/test/JsonTestTypes/SF/Linux.KConsTest_Sequential.json
        ./../../ktl/src/test/JsonTestTypes/SF/Linux.KConsTest_Isolated.json
        ./../../ktl/src/test/JsonTestTypes/SF/Linux.KConsLongTest.json

    DESTINATION ${TEST_OUTPUT_DIR}/../RunTests/JsonTestTypes/
)

install(
    FILES ./configuredumps.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}/
)

install(
    FILES ./testrunsetup.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}/
)

install(
    FILES ./testruncleanup.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}/
)

