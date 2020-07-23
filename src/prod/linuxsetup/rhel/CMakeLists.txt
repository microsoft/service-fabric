add_custom_target(runtimerpm

    COMMAND mkdir -p ${Base_Rpm_Folder}
    COMMAND mkdir -p ${Runtime_Rpm_DropFolder}
    COMMAND mkdir -p ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin
    COMMAND mkdir -p ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/tools/trace
    COMMAND mkdir -p ${Runtime_Rpm_DropFolder}/usr/share/servicefabric/scripts
    COMMAND cp -R ${FABRIC_DROP_DIR}/bin/. ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin
    COMMAND cp -R ${FABRIC_DROP_DIR}/tools/. ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/tools/.
    COMMAND cp -R ${Linux_Setup_Dir}/runtime/scripts/servicefabric.service ${Runtime_Rpm_DropFolder}/usr/share/servicefabric/scripts/
    COMMAND cp -R ${Linux_Setup_Dir}/runtime/scripts/servicefabricupdater.service ${Runtime_Rpm_DropFolder}/usr/share/servicefabric/scripts/

    #[[ Service scripts have to be nonexecutable ]]
    COMMAND chmod -x `find "${Runtime_Rpm_DropFolder}/usr/share/servicefabric/scripts" -regextype posix-basic -regex ".*service" -type f`

    #[[ Enable execute privilege on libs. Required for rpmbuild dependencies to be recognized. ]]
    COMMAND chmod +x `find ${Runtime_Rpm_DropFolder} -regextype posix-basic -regex ".*[.]so.*" -type f`
    COMMAND chmod +x `find ${Runtime_Rpm_DropFolder} -regextype posix-basic -regex ".*[.]a.*" -type f`
    #[[ Remove execute privilege on coreclr libs to avoid unresolvable dependencies. Note: Commented temporarily since these dlls are not present in latest build. ]]
    #[[ COMMAND chmod -x `find ${Runtime_Rpm_DropFolder} -regextype posix-basic -regex ".*System.*[.]so.*" -type f` ]]

    COMMAND strip ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin/FabricHost
    COMMAND strip ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin/Fabric/Fabric.Code/Fabric
    COMMAND strip ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin/Fabric/Fabric.Code/FabricGateway.exe
    COMMAND strip ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/FileStoreService.Code.Current/FileStoreService.exe
    COMMAND strip ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RM.Code.Current/FabricRM.exe
    COMMAND strip ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/CSS.Code.Current/FabricCSS.exe
    COMMAND strip ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin/Fabric/Fabric.Code/*.so
    COMMAND strip ${Runtime_Rpm_DropFolder}/${Runtime_Rpm_Install_Path}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RMS.Code.Current/ResourceMonitor.exe

    COMMAND mkdir -p ${Runtime_Rpm_DropFolder}/usr/share/doc/servicefabric
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/runtime/copyright_runtime_rpm ${Runtime_Rpm_DropFolder}/usr/share/doc/servicefabric/copyright_runtime

    COMMAND mkdir -p ${Runtime_Rpm_Folder}
    COMMAND cp -R --preserve ${Runtime_Rpm_DropFolder}/. ${Runtime_Rpm_Folder}/.

    COMMAND mkdir -p ${Base_Rpm_Folder}/generated
    COMMAND ${Linux_Setup_Dir}/rhel/helpers/generaterpmspecfile.pl ${Linux_Setup_Dir}/rhel/runtime/servicefabricruntime.spec ${Cluster_Version} ${Linux_Setup_Dir}/crossplatsetup/runtime/ > ${Base_Rpm_Folder}/generated/servicefabricruntime.generated.spec
    COMMAND ${Linux_Setup_Dir}/rhel/helpers/writerpmspecmetapayloads.pl ${Base_Rpm_Folder}/generated/servicefabricruntime.generated.spec ${FABRIC_DROP_DIR}/bin/doupgrade.sh

    COMMAND mkdir -p ${Rpmbuild_Runtime_Path}
    COMMAND rpmbuild -vv -bb ${Base_Rpm_Folder}/generated/servicefabricruntime.generated.spec --buildroot=${Runtime_Rpm_Folder} --define "_topdir ${Rpmbuild_Runtime_Path}" --define "__mono_requires test 0" --define "__mono_provides test 0"

    COMMAND mv `find ${Rpmbuild_Runtime_Path}/RPMS -regextype posix-basic -regex ".*servicefabric_[0-9].*rpm" -type f` ${Base_Rpm_Folder}/.
    )

add_custom_target(upgradetestrpm

    COMMAND mkdir -p ${Runtime_Rpm_Folder_UpgradeTest}
    COMMAND cp -R --preserve ${Runtime_Rpm_DropFolder}/. ${Runtime_Rpm_Folder_UpgradeTest}/.
    COMMAND echo -n ${Cluster_Version_UpgradeTest} > ${Runtime_Rpm_Folder_UpgradeTest}/${Runtime_Rpm_Install_Path}/bin/Fabric/Fabric.Code/ClusterVersion

    COMMAND mkdir -p ${Base_Rpm_Folder}/generated
    COMMAND ${Linux_Setup_Dir}/rhel/helpers/generaterpmspecfile.pl ${Linux_Setup_Dir}/rhel/runtime/servicefabricruntime.spec ${Cluster_Version_UpgradeTest} ${Linux_Setup_Dir}/crossplatsetup/runtime/ > ${Base_Rpm_Folder}/generated/servicefabricruntime_upgradetest.generated.spec
    COMMAND ${Linux_Setup_Dir}/rhel/helpers/writerpmspecmetapayloads.pl ${Base_Rpm_Folder}/generated/servicefabricruntime_upgradetest.generated.spec ${FABRIC_DROP_DIR}/bin/doupgrade.sh

    COMMAND mkdir -p ${Rpmbuild_Runtime_UpgradeTest_Path}
    COMMAND rpmbuild -vv -bb ${Base_Rpm_Folder}/generated/servicefabricruntime_upgradetest.generated.spec --buildroot=${Runtime_Rpm_Folder_UpgradeTest} --define "_topdir ${Rpmbuild_Runtime_UpgradeTest_Path}" --define "__mono_requires test 0" --define "__mono_provides test 0"

    COMMAND mv `find ${Rpmbuild_Runtime_UpgradeTest_Path}/RPMS -regextype posix-basic -regex ".*servicefabric_[0-9].*rpm" -type f` ${Base_Rpm_Folder}/.
    )
    
add_custom_target(sdkrpmcommon
    COMMAND mkdir -p ${Base_Rpm_Folder}
    COMMAND mkdir -p ${Sdk_Common_Rpm_Folder}
    COMMAND mkdir -p ${Sdk_Common_Rpm_Folder}/${Sdk_Common_Rpm_Install_Path}
    COMMAND mkdir -p ${Sdk_Common_Rpm_Folder}/${Sdk_Common_Rpm_Install_Path}/clustersetup
    COMMAND cp -R -p ${Linux_Setup_Dir}/sdkcommon/clustersetup/. ${Sdk_Common_Rpm_Folder}/${Sdk_Common_Rpm_Install_Path}/clustersetup/.

    COMMAND mkdir -p ${Sdk_Common_Rpm_Folder}/usr/share/doc/servicefabric
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/common/copyright_common_rpm ${Sdk_Common_Rpm_Folder}/usr/share/doc/servicefabric/copyright_common

    COMMAND mkdir -p ${Base_Rpm_Folder}/generated
    COMMAND ${Linux_Setup_Dir}/rhel/helpers/generaterpmspecfile.pl ${Linux_Setup_Dir}/rhel/common/servicefabric_sdkcommon.spec ${SDKCommon_Version} ${Linux_Setup_Dir}/crossplatsetup/common/ > ${Base_Rpm_Folder}/generated/servicefabric_sdkcommon.generated.spec
    COMMAND sed -i "s%REPLACE_RUNTIME_VERSION%${Cluster_Version}%g" ${Base_Rpm_Folder}/generated/servicefabric_sdkcommon.generated.spec

    COMMAND mkdir -p ${Rpmbuild_SdkCommon_Path}
    COMMAND rpmbuild -v -bb ${Base_Rpm_Folder}/generated/servicefabric_sdkcommon.generated.spec --buildroot=${Sdk_Common_Rpm_Folder} --define "_topdir ${Rpmbuild_SdkCommon_Path}" --define "__mono_requires test 0" --define "__mono_provides test 0"

    COMMAND mv `find ${Rpmbuild_SdkCommon_Path}/RPMS -regextype posix-basic -regex ".*servicefabric_sdkcommon_[0-9].*rpm" -type f` ${Base_Rpm_Folder}/.
    )
