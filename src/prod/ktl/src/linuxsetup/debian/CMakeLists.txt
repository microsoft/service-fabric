add_custom_target(kxmdriverdeb
    COMMAND mkdir -p ${KXM_Driver_Deb_Folder}
    COMMAND mkdir -p ${KXM_Driver_Deb_Folder}/${KXM_Driver_Install_Path}
    COMMAND cp -R ${KXM_DRIVER_SOURCE}/src/ ${KXM_Driver_Deb_Folder}/${KXM_Driver_Install_Path}/
    COMMAND cp -R ${KXM_DRIVER_SOURCE}/inc/ ${KXM_Driver_Deb_Folder}/${KXM_Driver_Install_Path}/
    COMMAND cp -R ${KXM_DRIVER_SOURCE}/../common ${KXM_Driver_Deb_Folder}/${KXM_Driver_Install_Path}/
    COMMAND cp -R ${KXM_DRIVER_SOURCE}/Makefile ${KXM_Driver_Deb_Folder}/${KXM_Driver_Install_Path}/

    COMMAND mkdir ${KXM_Driver_Deb_Folder}/DEBIAN

    COMMAND cp -r ${KTL_Linux_Setup_Dir}/debian/kxm/control/. ${KXM_Driver_Deb_Folder}/DEBIAN
    COMMAND mv -f ${KXM_Driver_Deb_Folder}/DEBIAN/control.template ${KXM_Driver_Deb_Folder}/DEBIAN/control
    COMMAND sed -i "s%REPLACE_VERSION%${DRIVER_VERSION}%g" ${KXM_Driver_Deb_Folder}/DEBIAN/control
    COMMAND sed -i "s%REPLACE_MAJOR%${MAJOR_NUMBER}%g" ${KXM_Driver_Deb_Folder}/DEBIAN/control
    COMMAND chmod 0755 ${KXM_Driver_Deb_Folder}/DEBIAN/postinst
    COMMAND chmod 0755 ${KXM_Driver_Deb_Folder}/DEBIAN/preinst
    COMMAND chmod 0755 ${KXM_Driver_Deb_Folder}/DEBIAN/postrm
    COMMAND chmod 0755 ${KXM_Driver_Deb_Folder}/DEBIAN/prerm
    COMMAND dpkg-deb --build ${KXM_Driver_Deb_Folder}
    )
