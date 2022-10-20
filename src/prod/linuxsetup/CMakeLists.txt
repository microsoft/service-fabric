add_custom_target(fabricdrop
    COMMAND ln ${TEST_OUTPUT_DIR}/ ${TEST_OUTPUT_DIR}/../FabricUnitTests -sf

    # After managed code is build, restructure managed bits
    COMMAND ${PROJECT_SOURCE_DIR}/src/managed/restructure_managed_bits.sh ${Managed_Libs_Dir} ${PROJECT_SOURCE_DIR}/../ ${PACKAGE_LOCAL_BUILD_MANAGED}

    COMMAND cp -r ${Package_Managed_Libs_Dir}/Fabric.Code/. ${TEST_OUTPUT_DIR}/
    COMMAND chmod +x ${TEST_OUTPUT_DIR}/ImageBuilder.sh
    COMMAND chmod +x ${TEST_OUTPUT_DIR}/FabricCAS.sh

    COMMAND rm -rf ${FABRIC_DROP_DIR}
    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Data
    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code
    COMMAND mkdir -p ${FABRIC_DROP_DIR}/tools/trace

    COMMAND cp -r ${Package_Managed_Libs_Dir}/Fabric.Code/. ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/azurecliproxy/CoreCLR/azurecliproxy/AzureCliProxy.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/ImageBuilderExe/CoreCLR/ImageBuilder/ImageBuilder.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/FabricCAS/CoreCLR/FabricCAS.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/tools/CoreCLRUtilites/SfTrace/lttng_handler.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/AzureCliProxy.sh
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/ImageBuilder.sh
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/FabricCAS.sh
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/lttng_handler.sh

    COMMAND cp -r ${Package_Managed_Libs_Dir}/DCA.Code/. ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code
    COMMAND cp ${WinFab_Babeltrace_Dir}/bin/sftrace ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code
    COMMAND cp ${WinFab_Babeltrace_Dir}/bin/sftrace ${TEST_OUTPUT_DIR}
    COMMAND cp ${WinFab_Babeltrace_Dir}/bin/sftrace ${CLUSTER_DEPLOYER_DIR}
    COMMAND cp ${WinFab_Babeltrace_Dir}/lib/lib* ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code
    COMMAND cp ${WinFab_Babeltrace_Dir}/lib/lib* ${TEST_OUTPUT_DIR}
    COMMAND cp ${WinFab_Babeltrace_Dir}/lib/lib* ${CLUSTER_DEPLOYER_DIR}
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/azurecliproxy/CoreCLR/azurecliproxy/AzureCliProxy.sh ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/.
    COMMAND cp ${LIBRARY_OUTPUT_DIR}/libLttngReader.so ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/DCA/product/host/CoreClrDll/FabricDCA/FabricDCA.sh ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/tools/CoreCLRUtilites/SfTrace/lttng_handler.sh ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/manifests/components/Microsoft-ServiceFabric-Events.man ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/manifests/components/Microsoft-ServiceFabric-KtlEvents.man ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/manifests/components/Microsoft-ServiceFabric-LeaseEvents.man ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/.


    COMMAND cp ${WinFab_Linux_Libs_Dir}/Networking/azure-cns ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/azure-cns

    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/AzureCliProxy.sh
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/FabricDCA.sh
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/sftrace
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/lttng_handler.sh

    COMMAND cp -r ${TEST_OUTPUT_DIR}/FabricHost                 ${FABRIC_DROP_DIR}/bin/.
    COMMAND cp -r ${TEST_OUTPUT_DIR}/FabricGateway              ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/FabricGateway.exe
    COMMAND cp -r ${TEST_OUTPUT_DIR}/Fabric                     ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/Fabric
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricRuntime.so     ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricCommon.so      ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricResources.so   ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricClient.so      ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricImageStore.so  ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libKPhysicalLog.so  ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricTokenValidationService.so  ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricContainerActivatorService.so  ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricInfrastructureService.so  ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${WinFab_Linux_Libs_Dir}/ClrPal/libcoreclr.so ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${PROJECT_BINARY_DIR}/FabricTypeHost.exe      ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricServiceCommunication.so ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libReliableCollectionRuntime.so     ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricTransport.so ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricFaultAnalysisService.so      ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.

    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libReliableCollectionService.so      ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -r ${LIBRARY_OUTPUT_DIR}/libFabricGatewayResourceManager.so  ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.

    COMMAND echo Cluster Version ${Cluster_Version}
    COMMAND echo -n ${Cluster_Version} > ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/ClusterVersion
    COMMAND echo -n ${Cluster_Version} > ${FABRIC_DROP_DIR}/bin/Fabric/DCA.Code/ClusterVersion
    COMMAND echo -n ${Cluster_Version} > ${TEST_OUTPUT_DIR}/ClusterVersion

    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/FabricHost
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/Fabric
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/FabricGateway.exe
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/libFabricImageStore.so
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/libFabricRuntime.so

    COMMAND cp -a ${THIRD_PARTY_LIB_DIR}/casablanca_v_2/Binaries/.      ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    COMMAND cp -a ${THIRD_PARTY_LIB_DIR}/Boost_1_61_0/lib/.         ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.

    COMMAND cp -a ${Linux_Setup_Dir}/runtime/scripts/deployfabric.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/deployfabric.sh
    COMMAND cp -a ${Linux_Setup_Dir}/runtime/scripts/starthost.sh    ${FABRIC_DROP_DIR}/bin/starthost.sh
    COMMAND cp -a ${Linux_Setup_Dir}/runtime/scripts/FaclBlkDev.sh    ${FABRIC_DROP_DIR}/bin/FaclBlkDev.sh
    COMMAND cp -a ${Linux_Setup_Dir}/runtime/scripts/FabricDeployer.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/FabricDeployer.sh
    COMMAND cp -a ${Linux_Setup_Dir}/runtime/scripts/startservicefabricupdater.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/startservicefabricupdater.sh
    COMMAND cp -a ${Linux_Setup_Dir}/runtime/scripts/doupgrade.sh ${FABRIC_DROP_DIR}/bin/doupgrade.sh
    COMMAND cp -a ${Linux_Setup_Dir}/runtime/scripts/cleancgroup.sh ${FABRIC_DROP_DIR}/bin/cleancgroup.sh

    COMMAND chmod 0755 ${FABRIC_DROP_DIR}/bin/doupgrade.sh

    COMMAND cp ${PROJECT_SOURCE_DIR}/src/ServiceModel/xsd/ServiceFabricServiceModel.xsd ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/ServiceFabricServiceModel.xsd

    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Data/html/Explorer
    COMMAND cp -R ${EXTERNAL_DEPS_DIR}/ServiceFabric.Explorer/. ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Data/html/Explorer/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/ServiceFabricExplorer/ServiceFabric.html ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Data/html/ServiceFabric.html

    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/FileStoreService.Code.Current
    COMMAND cp ${TEST_OUTPUT_DIR}/FileStoreService ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/FileStoreService.Code.Current/FileStoreService.exe
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/App.1.0.prod.linux.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/App.1.0.xml
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/FileStoreService.Manifest.Current.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/FileStoreService.Package.1.0.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/FileStoreService.Code.Current/FileStoreService.exe

    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RM.Code.Current
    COMMAND cp ${TEST_OUTPUT_DIR}/FabricRM ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RM.Code.Current/FabricRM.exe
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/RM.Manifest.Current.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/RM.Package.1.0.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RM.Code.Current/FabricRM.exe

    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/CSS.Code.Current
    COMMAND cp ${TEST_OUTPUT_DIR}/FabricCSS ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/CSS.Code.Current/FabricCSS.exe
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/CSS.Manifest.Current.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/CSS.Package.1.0.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/CSS.Code.Current/FabricCSS.exe

    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/DnsService.Code.Current
    COMMAND cp ${TEST_OUTPUT_DIR}/FabricDnsService ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/DnsService.Code.Current/FabricDnsService.exe
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/DnsService.Manifest.Current.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/DnsService.Package.1.0.linux.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/DnsService.Package.1.0.xml
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/DnsService.Code.Current/FabricDnsService.exe

    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RMS.Code.Current
    COMMAND cp ${TEST_OUTPUT_DIR}/ResourceMonitorService ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RMS.Code.Current/ResourceMonitor.exe
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/RMS.Manifest.Current.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RMS.Manifest.Current.xml
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/RMS.Package.1.0.prod.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RMS.Package.1.0.xml
    COMMAND chrpath -d ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RMS.Code.Current/ResourceMonitor.exe

    COMMAND cp -r ${Package_Managed_Libs_Dir}/FabricUS/. ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/FabricUS/CoreCLR/FabricUS/FabricUS.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/US.Code.Current/.
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/US.Code.Current/FabricUS.sh

    # Let FabricIS come from winfab coreclr libs
    COMMAND cp -r ${WinFab_CoreCLR_Libs_Dir}/FabricIS/IS.Code.Current ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/IS.Code.Current
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/fabricis/host/CoreCLR/FabricIS/FabricIS.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/IS.Code.Current/.
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/IS.Code.Current/FabricIS.sh
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/fabricis/parallel/manualtool/CoreCLR/FabricInfrastructureManualControl/FabricInfrastructureManualControl.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/IS.Code.Current/.
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/IS.Code.Current/FabricInfrastructureManualControl.sh

    COMMAND cp -r ${Package_Managed_Libs_Dir}/FabricFAS/. ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/FabricFAS/CoreCLRDlls/exe/FabricFAS.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/FAS.Code.Current/.
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/FAS.Code.Current/FabricFAS.sh

    COMMAND cp -r ${Package_Managed_Libs_Dir}/FabricTVS/. ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/TokenValidationService/CoreCLR/FabricTVS/FabricTVS.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/TVS.Code.Current/.
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/TVS.Code.Current/FabricTVS.sh

    COMMAND cp -r ${Package_Managed_Libs_Dir}/FabricGRM/. ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/.
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/FabricGRM/CoreCLR/FabricGRM/FabricGRM.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/GRM.Code.Current/.
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/GRM.Code.Current/FabricGRM.sh

    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/App.1.0.prod.linux.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/App.1.0.xml
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/IS.Package.1.0.prod.linux.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/IS.Package.1.0.xml
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Management/fabricsystemapppackage/__fabricsystem_app4294967295/IS.Manifest.Current.linux.xml ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/IS.Manifest.Current.xml

    COMMAND cp -R ${WinFab_Babeltrace_Dir}/bin/sftrace ${FABRIC_DROP_DIR}/tools/trace/.
    COMMAND cp -R ${WinFab_Babeltrace_Dir}/lib/libbabel* ${FABRIC_DROP_DIR}/tools/trace/.

    COMMAND cp -R ${Linux_Setup_Dir}/runtime/scripts/collecttraces.sh  ${FABRIC_DROP_DIR}/tools/trace/.
    COMMAND chmod +x ${FABRIC_DROP_DIR}/tools/trace/sftrace

    COMMAND cp ${PROJECT_SOURCE_DIR}/src/linux/scripts/sfcli.sh ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/sfcli.sh

    COMMAND cp -r ${PROJECT_SOURCE_DIR}/src/linux/dockers ${PROJECT_BINARY_DIR}
    COMMAND cp -r ${PROJECT_SOURCE_DIR}/src/linux/dockers/.dockerignore ${PROJECT_BINARY_DIR}

    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Hosting2/crio.cc.sh   ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/crio.cc.sh
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Hosting2/crio.kata.sh   ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/crio.kata.sh
    COMMAND cp ${Linux_Setup_Dir}/crio.cc.conf    ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND cp ${Linux_Setup_Dir}/crio.kata.conf    ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code

    COMMAND cp ${PROJECT_SOURCE_DIR}/src/Hosting2/containerlogrotate.sh   ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/containerlogrotate.sh

    COMMAND cp ${THIRD_PARTY_LIB_DIR}/cri-o/cni/bridge  ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/bridge
    COMMAND cp ${THIRD_PARTY_LIB_DIR}/cri-o/cni/host-local  ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code
    COMMAND chmod +x ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/host-local

    COMMAND cp -r ${PROJECT_SOURCE_DIR}/../test/ClusterDeployer ${PROJECT_BINARY_DIR}/ClusterDeployerTest

    # BlockStore Service
    COMMAND mkdir -p ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/BlockStore/

    # Copy Blockstore driver.

    # Copy Volume Drive Manifests and update version
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/BlockStore/Service/VolumeDriver/Service/Manifests/Linux/ApplicationManifest.xml ${PROJECT_BINARY_DIR}/coreclr/bin/BlockStore/VolumeDriver/ubuntu.16.04-x64/
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/BlockStore/Service/VolumeDriver/Service/Manifests/Linux/ServiceManifest.xml ${PROJECT_BINARY_DIR}/coreclr/bin/BlockStore/VolumeDriver/ubuntu.16.04-x64/ServiceFabricVolumeDriverPkg/
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/Container/VolumePlugin/AzureFiles/Service/Manifests/Linux/ApplicationManifest.xml ${PROJECT_BINARY_DIR}/coreclr/bin/VolumePluginDrop/ubuntu.16.04-x64/AzureFiles/
    COMMAND cp ${PROJECT_SOURCE_DIR}/src/managed/Container/VolumePlugin/AzureFiles/Service/Manifests/Linux/ServiceManifest.xml ${PROJECT_BINARY_DIR}/coreclr/bin/VolumePluginDrop/ubuntu.16.04-x64/AzureFiles/AzureFilesVolumePluginPkg/
    # Simulate SpecializeFabricVersion target to replace !FABRIC_VERSION! with build version
    COMMAND sed -i 's/!FABRIC_VERSION!/${BUILD_VERSION}/g' ${PROJECT_BINARY_DIR}/coreclr/bin/BlockStore/VolumeDriver/ubuntu.16.04-x64/ServiceFabricVolumeDriverPkg/ServiceManifest.xml
    COMMAND sed -i 's/!FABRIC_VERSION!/${BUILD_VERSION}/g' ${PROJECT_BINARY_DIR}/coreclr/bin/BlockStore/VolumeDriver/ubuntu.16.04-x64/ApplicationManifest.xml
    COMMAND sed -i 's/!FABRIC_VERSION!/${BUILD_VERSION}/g' ${PROJECT_BINARY_DIR}/coreclr/bin/VolumePluginDrop/ubuntu.16.04-x64/AzureFiles/AzureFilesVolumePluginPkg/ServiceManifest.xml
    COMMAND sed -i 's/!FABRIC_VERSION!/${BUILD_VERSION}/g' ${PROJECT_BINARY_DIR}/coreclr/bin/VolumePluginDrop/ubuntu.16.04-x64/AzureFiles/ApplicationManifest.xml
    COMMAND cp -r ${TEST_OUTPUT_DIR}/CheckLinuxLog.exe     ${FABRIC_DROP_DIR}/bin/Fabric/Fabric.Code/.
    )

add_custom_target(symsplit
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/FabricHost                                ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/Fabric                 ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/FabricGateway.exe      ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/FabricTypeHost.exe     ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricRuntime.so    ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricCommon.so     ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricResources.so  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricClient.so     ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricImageStore.so ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricInfrastructureService.so   ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricTokenValidationService.so  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricGatewayResourceManager.so   ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RM.Code.Current/FabricRM.exe  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/CSS.Code.Current/FabricCSS.exe  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/FileStoreService.Code.Current/FileStoreService.exe  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libReliableCollectionRuntime.so    ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricFaultAnalysisService.so     ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libKPhysicalLog.so     ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricContainerActivatorService.so  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RMS.Code.Current/ResourceMonitor.exe  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/SFBlockStoreService  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libSFBDCtlLib.so  ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${FABRIC_DROP_DIR}  bin/Fabric/Fabric.Code/libFabricGatewayResourceManager.so     ${FABRIC_DROP_DIR}/symbols ${CMAKE_STRIP}
    )

add_custom_target(symsplitfortestdir
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${TEST_OUTPUT_DIR}  Fabric ${TEST_OUTPUT_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${TEST_OUTPUT_DIR}  FabricRM ${TEST_OUTPUT_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${TEST_OUTPUT_DIR}  FabricCSS ${TEST_OUTPUT_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${TEST_OUTPUT_DIR}  FileStoreService ${TEST_OUTPUT_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${TEST_OUTPUT_DIR}  FabricGateway ${TEST_OUTPUT_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/symsplit.sh ${TEST_OUTPUT_DIR}  FabricHost  ${TEST_OUTPUT_DIR}/symbols ${CMAKE_STRIP}
    COMMAND ${Linux_Setup_Dir}/scripts/splittestdir.sh ${TEST_OUTPUT_DIR} ${TEST_OUTPUT_DIR}/symbols ${CMAKE_STRIP}
)

if(LINUX_DISTRIBUTION STREQUAL "DEBIAN")
    add_subdirectory (debian)
elseif(LINUX_DISTRIBUTION STREQUAL "REDHAT")
    add_subdirectory (rhel)
endif()
