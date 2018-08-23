install(
    FILES
    UnsupportedTests.list
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./RunTests.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES LinuxRunTestsV2Settings.json
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME RunTestsV2Settings.json
)

install(
    FILES ../JsonTests/LinuxKtlUnitTests.json 
    DESTINATION ${TEST_OUTPUT_DIR}/JsonTests/
    RENAME KtlUnitTests.json)

install(
    FILES 
        ../JsonTestTypes/Linux.KConsTest.json
        ../JsonTestTypes/Linux.KConsTest_Sequential.json
        ../JsonTestTypes/Linux.KConsTest_Isolated.json
        ../JsonTestTypes/Linux.KConsLongTest.json

    DESTINATION ${TEST_OUTPUT_DIR}/JsonTestTypes/ 
)


install(CODE "EXECUTE_PROCESS(COMMAND cp -a ${EXTERNAL_LIB_PATH}/RunTests/. ${TEST_OUTPUT_DIR})")
