include_directories("../pch")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_ServiceModelTest}
  # boost.test main
    ../../../test/BoostUnitTest/btest.cpp

  # test code
    ../ApplicationIdentifier.Test.cpp
    ../Parser.Test.cpp
    ../ServiceModel.Test.cpp
    ../ConsistencyUnitId.test.cpp
    #../replicatorsettings.test.cpp
    ../SizeEstimator.test.cpp
    ../NodeIdGenerator.Test.cpp
    ../Serializer.Test.cpp
	../KeyRange.Test.cpp
    ../QueryArgumentMap.Test.cpp
    ../modelV2/modelV2.Test.cpp
)


add_precompiled_header(${exe_ServiceModelTest} ../pch/stdafx.h)

set_target_properties(${exe_ServiceModelTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_ServiceModelTest}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_Common}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ssh2
  minizip
  z
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
  c
)

install(
    FILES 
        ./ApplicationInstance.1.xml
        ./ApplicationInstanceWithDefaultServiceGroup.1.xml
        ./ApplicationPackageWithDiagnostics.1.xml
        ./ApplicationPackage.1.0.xml
        ./ApplicationManifest.1.0.xml
        ./ApplicationManifest.2.0.xml
        ./ApplicationManifest.3.0.xml
        ./ApplicationPackage.1.0.xml
        ./CalculatorSample.Package.Bootstrap.xml
        ./CalculatorSample.Package.WithDiagnostics.xml
        ./CalculatorSample.Manifest.1.0.xml
        ./CalculatorSample.Manifest.2.0.xml
        ./ConfigSettings-BasicTest-1.xml
        ./ConfigSettings-BasicTest-2.xml
        ./ConfigSettings-BasicTest-3.xml
        ./ConfigSettings-BasicTest-4.xml
        ./ServiceGroupSample.Manifest.1.0.xml
        ./ApplicationPackage-HealthPolicy-1.1.0.xml
        ./ApplicationPackage-HealthPolicy-2.1.0.xml
        ./ApplicationPackage-HealthPolicy-3.1.0.xml
        ./ApplicationPackage-HealthPolicy-4.1.0.xml
        ./ApplicationPackage-HealthPolicy-5.1.0.xml
        ./ApplicationPackage-HealthPolicy-6.1.0.xml
        ./ApplicationPackage-Policies-1.1.0.xml
        ./ApplicationPackage-Policies-2.1.0.xml
        ./ApplicationPackage-Policies-3.1.0.xml
        ./TargetInformation.Valid.xml
        ./TargetInformation.NoCurrentInstallation.xml
        ./TargetInformation.NoTargetInstallation.xml
        ./TargetInformation.Empty.xml
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./ServiceModel.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./RunServiceModelTests.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE 
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    DIRECTORY ./../modelV2/modelV2TestFiles
    DESTINATION ${TEST_OUTPUT_DIR}/
)

