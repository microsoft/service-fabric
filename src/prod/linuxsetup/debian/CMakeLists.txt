add_custom_target(runtimedeb

    COMMAND mkdir -p ${Runtime_Deb_Folder}
    COMMAND mkdir -p ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin
    COMMAND mkdir -p ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/tools/trace
    COMMAND mkdir -p ${Runtime_Deb_Folder}/usr/share/servicefabric/scripts
    COMMAND cp -R ${FABRIC_DROP_DIR}/bin/. ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin
    COMMAND cp -R ${FABRIC_DROP_DIR}/tools/. ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/tools/.
    COMMAND cp -R ${Linux_Setup_Dir}/runtime/scripts/servicefabric.service ${Runtime_Deb_Folder}/usr/share/servicefabric/scripts/
    COMMAND cp -R ${Linux_Setup_Dir}/runtime/scripts/servicefabricupdater.service ${Runtime_Deb_Folder}/usr/share/servicefabric/scripts/
    COMMAND mkdir -p ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/AddonServices
    COMMAND cp -R ${FABRIC_DROP_DIR}/../coreclr/bin/BlockStore/VolumeDriver/ubuntu.16.04-x64 ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/AddonServices/ServiceFabricVolumeDriver
    COMMAND cp -R ${FABRIC_DROP_DIR}/../coreclr/bin/VolumePluginDrop/ubuntu.16.04-x64/AzureFiles ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/AddonServices/AzureFilesVolumePlugin    

    COMMAND strip ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/FabricHost
    COMMAND strip ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/Fabric/Fabric.Code/Fabric
    COMMAND strip ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/Fabric/Fabric.Code/FabricGateway.exe
    COMMAND strip ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/FileStoreService.Code.Current/FileStoreService.exe
    COMMAND strip ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RM.Code.Current/FabricRM.exe
    COMMAND strip ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/CSS.Code.Current/FabricCSS.exe
    COMMAND strip ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/Fabric/Fabric.Code/*.so
    COMMAND strip ${Runtime_Deb_Folder}/${Runtime_Deb_Install_Path}/bin/Fabric/Fabric.Code/__FabricSystem_App4294967295/RMS.Code.Current/ResourceMonitor.exe
    COMMAND mkdir ${Runtime_Deb_Folder}/DEBIAN

    COMMAND mkdir -p ${Runtime_Deb_Folder}/usr/share/doc/servicefabric
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/runtime/copyright_runtime_deb ${Runtime_Deb_Folder}/usr/share/doc/servicefabric/copyright_runtime

    COMMAND cp -r ${Linux_Setup_Dir}/debian/runtime/control/. ${Runtime_Deb_Folder}/DEBIAN
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/runtime/preinst ${Runtime_Deb_Folder}/DEBIAN/
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/runtime/postinst ${Runtime_Deb_Folder}/DEBIAN/
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/runtime/prerm ${Runtime_Deb_Folder}/DEBIAN/
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/runtime/postrm ${Runtime_Deb_Folder}/DEBIAN/
    COMMAND mv -f ${Runtime_Deb_Folder}/DEBIAN/control.template ${Runtime_Deb_Folder}/DEBIAN/control
    COMMAND sed -i "s%REPLACE_VERSION%${Cluster_Version}%g" ${Runtime_Deb_Folder}/DEBIAN/control

    COMMAND ${Linux_Setup_Dir}/debian/helpers/writeeula_to_templates.pl ${Runtime_Deb_Folder}/DEBIAN/templates.template ${Linux_Setup_Dir}/crossplatsetup/runtime/copyright_runtime_deb REPLACE_COPYRIGHT_BUILDTIME > ${Runtime_Deb_Folder}/DEBIAN/templates
    COMMAND rm -f ${Runtime_Deb_Folder}/DEBIAN/templates.template

    #[[ Write upgrade script to debian control data so it can be promptly extracted (perf optimization) as part of upgrade. This enables single-phase upgrade. ]]
    #[[ Old version naming formats have to be duplicated in the new deb such that new version can be found using old Hosting code. ]]
    #[[ New version format is added in cases of breaking changes in the upgrade process where we need to use the new logic for downgrade/rollback. ]]
    COMMAND cp ${FABRIC_DROP_DIR}/bin/doupgrade.sh ${Runtime_Deb_Folder}/DEBIAN/meta_doupgrade.sh
    COMMAND cp ${FABRIC_DROP_DIR}/bin/doupgrade.sh ${Runtime_Deb_Folder}/DEBIAN/meta_doupgrade.v2.sh

    COMMAND chmod 0755 ${Runtime_Deb_Folder}/DEBIAN/postinst
    COMMAND chmod 0755 ${Runtime_Deb_Folder}/DEBIAN/preinst
    COMMAND chmod 0755 ${Runtime_Deb_Folder}/DEBIAN/postrm
    COMMAND chmod 0755 ${Runtime_Deb_Folder}/DEBIAN/prerm
    COMMAND chmod 0755 ${Runtime_Deb_Folder}/DEBIAN/config
    COMMAND chmod 0755 ${Runtime_Deb_Folder}/DEBIAN/meta_doupgrade.sh
    COMMAND chmod 0755 ${Runtime_Deb_Folder}/DEBIAN/meta_doupgrade.v2.sh
    COMMAND dpkg-deb --build ${Runtime_Deb_Folder}
    )

add_custom_target(upgradetestdeb

    COMMAND mkdir -p ${Runtime_Deb_Folder_UpgradeTest}
    COMMAND cp -R ${Runtime_Deb_Folder}/. ${Runtime_Deb_Folder_UpgradeTest}/.
    COMMAND echo -n ${Cluster_Version_UpgradeTest} > ${Runtime_Deb_Folder_UpgradeTest}/${Runtime_Deb_Install_Path}/bin/Fabric/Fabric.Code/ClusterVersion

    COMMAND cp -f ${Linux_Setup_Dir}/debian/runtime/control/control.template ${Runtime_Deb_Folder_UpgradeTest}/DEBIAN/control
    COMMAND sed -i "s%REPLACE_VERSION%${Cluster_Version_UpgradeTest}%g" ${Runtime_Deb_Folder_UpgradeTest}/DEBIAN/control
    COMMAND dpkg-deb --build ${Runtime_Deb_Folder_UpgradeTest}
    )
    
add_custom_target(sdkdebcommon
    COMMAND mkdir -p ${Sdk_Common_Deb_Folder}
    COMMAND mkdir -p ${Sdk_Common_Deb_Folder}/${Sdk_Common_Deb_Install_Path}
    COMMAND mkdir -p ${Sdk_Common_Deb_Folder}/${Sdk_Common_Deb_Install_Path}/clustersetup
    COMMAND cp -R -p ${Linux_Setup_Dir}/sdkcommon/clustersetup/. ${Sdk_Common_Deb_Folder}/${Sdk_Common_Deb_Install_Path}/clustersetup/.

    COMMAND mkdir -p ${Sdk_Common_Deb_Folder}/usr/share/doc/servicefabric
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/common/copyright_common_deb ${Sdk_Common_Deb_Folder}/usr/share/doc/servicefabric/copyright_common

    COMMAND cp -r ${Linux_Setup_Dir}/debian/common/control/. ${Sdk_Common_Deb_Folder}/DEBIAN
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/common/preinst ${Sdk_Common_Deb_Folder}/DEBIAN/
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/common/postinst ${Sdk_Common_Deb_Folder}/DEBIAN/
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/common/prerm ${Sdk_Common_Deb_Folder}/DEBIAN/
    COMMAND cp ${Linux_Setup_Dir}/crossplatsetup/common/postrm ${Sdk_Common_Deb_Folder}/DEBIAN/
    COMMAND mv -f ${Sdk_Common_Deb_Folder}/DEBIAN/control.template ${Sdk_Common_Deb_Folder}/DEBIAN/control
    COMMAND sed -i "s%REPLACE_RUNTIME_VERSION%${Cluster_Version}%g" ${Sdk_Common_Deb_Folder}/DEBIAN/control
    COMMAND sed -i "s%REPLACE_VERSION%${SDKCommon_Version}%g" ${Sdk_Common_Deb_Folder}/DEBIAN/control	
    COMMAND sed -i "s%GEN_GUESTBIN_SF%${GuestBinary_Gen_Name}%g" ${Sdk_Common_Deb_Folder}/DEBIAN/postrm
    COMMAND sed -i "s%GEN_CONTAINER_SF%${Container_Gen_Name}%g" ${Sdk_Common_Deb_Folder}/DEBIAN/postrm
    COMMAND sed -i "s%SFSDK_PATH%/${SFSdk_Path}%g" ${Sdk_Common_Deb_Folder}/DEBIAN/postrm
    
    COMMAND ${Linux_Setup_Dir}/debian/helpers/writeeula_to_templates.pl ${Sdk_Common_Deb_Folder}/DEBIAN/templates.template ${Linux_Setup_Dir}/crossplatsetup/common/copyright_common_deb REPLACE_COPYRIGHT_BUILDTIME > ${Sdk_Common_Deb_Folder}/DEBIAN/templates
    COMMAND rm -f ${Sdk_Common_Deb_Folder}/DEBIAN/templates.template

    COMMAND chmod 0755 ${Sdk_Common_Deb_Folder}/DEBIAN/postinst
    COMMAND chmod 0755 ${Sdk_Common_Deb_Folder}/DEBIAN/preinst
    COMMAND chmod 0755 ${Sdk_Common_Deb_Folder}/DEBIAN/postrm
    COMMAND chmod 0755 ${Sdk_Common_Deb_Folder}/DEBIAN/prerm
    COMMAND chmod 0755 ${Sdk_Common_Deb_Folder}/DEBIAN/config
    COMMAND dpkg-deb --build ${Sdk_Common_Deb_Folder}
    )

