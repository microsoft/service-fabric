add_subdirectory(user)

install(
    FILES XmlTestInput.xml
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES TestFile0.xml
    DESTINATION ${TEST_OUTPUT_DIR}
)

