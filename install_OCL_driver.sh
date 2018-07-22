#!/bin/bash
#
# Installs the Graphics Driver for OpenCL on Linux.
#
# Usage: sudo -E ./install_OCL_driver.sh
#
# Supported platforms:
#     6th, 7th or 8th generation Intel® processor with Intel(R)
#     Processor Graphics Technology not previously disabled by the BIOS
#     or motherboard settings
#
RELEASE_NUM="5.0"
BUILD_ID="r5.0-63503"
EXIT_FAILURE=1
PKGS=
CENTOS_MINOR=
UBUNTU_VERSION=
DISTRO=

_install_prerequisites_centos()
{
    # yum doesn't accept timeout in seconds as parameter
    echo
    echo "Note: if yum becomes non-responsive, try aborting the script and run:"
    echo "      sudo -E $0"
    echo

    CMDS=("yum -y install tar libpciaccess numactl-libs"
          "yum -y groupinstall 'Development Tools'"
          "yum -y install rpmdevtools openssl openssl-devel bc numactl ocl-icd ocl-icd-devel")

    for cmd in "${CMDS[@]}"; do
        echo $cmd
        eval $cmd
        if [[ $? -ne 0 ]]; then
            echo ERROR: failed to run $cmd >&2
            echo Problem \(or disk space\)? >&2
            echo . Verify that you have enough disk space, and run the script again. >&2
            exit $EXIT_FAIULRE
        fi
    done

}

_install_prerequisites_ubuntu()
{
    CMDS=("apt-get -y update"
          "apt-get -y install libnuma1 ocl-icd-libopencl1")

    for cmd in "${CMDS[@]}"; do
        echo $cmd
        eval $cmd
        if [[ $? -ne 0 ]]; then
            echo ERROR: failed to run $cmd >&2
            echo Problem \(or disk space\)? >&2
            echo "                sudo -E $0" >&2
            echo 2. Verify that you have enough disk space, and run the script again. >&2
            exit $EXIT_FAIULRE
        fi
    done
}

install_prerequisites()
{
    if [[ $DISTRO == "centos" ]]; then
        echo Installing prerequisites...
        _install_prerequisites_centos
    elif [[ $DISTRO == "ubuntu" ]]; then
        echo Installing prerequisites...
        _install_prerequisites_ubuntu
    else
        echo Unknown OS
    fi
}

_deploy_rpm()
{
    # On a CentOS 7.2 machine with Intel Parallel Composer XE 2017
    # installed we got conflicts when trying to deploy these rpms.
    # If that happens to you too, try again with:
    # IGFX_RPM_FLAGS="--force" sudo -E ./install_OCL_driver.sh install
    #
    cmd="rpm $IGFX_RPM_FLAGS -ivh --force $1"
    echo $cmd
    eval $cmd
}

_deploy_deb()
{
    cmd="dpkg -i $1"
    echo $cmd
    eval $cmd
}

_install_user_mode_centos()
{
    _deploy_rpm "*.rpm"
    if [[ $? -ne 0 ]]; then
        echo ERROR: failed to install rpms $cmd erroe  >&2
        echo Make sure you have enough disk space or fix the problem manually and try again. >&2
        exit $EXIT_FAIULRE
    fi
}

_install_user_mode_ubuntu()
{
    _deploy_deb "*.deb"
    if [[ $? -ne 0 ]]; then
        echo ERROR: failed to install rpms $cmd erroe  >&2
        echo Make sure you have enough disk space or fix the problem manually and try again. >&2
        exit $EXIT_FAIULRE
    fi
}

install_user_mode()
{
    echo Installing user mode driver...

    if [[ $DISTRO == "centos" ]]; then
        _install_user_mode_centos
    else
        _install_user_mode_ubuntu
    fi

}

_uninstall_user_mode_centos()
{
    echo Looking for previously installed user-mode driver...
    echo "rpm -qa | grep intel-opencl"
    rpm -qa | grep intel-opencl
    if [[ $? -eq 0 ]]; then
        echo Found installed user-mode driver, performing uninstall...
        cmd='rpm -e --nodeps $(rpm -qa | grep intel-opencl)'
        echo $cmd
        eval $cmd
        if [[ $? -ne 0 ]]; then
            echo ERROR: failed to uninstall existing user-mode driver. >&2
            echo Please try again manually and run the script again. >&2
            exit $EXIT_FAIULRE
        fi
    fi
}

_uninstall_user_mode_ubuntu()
{
    echo Looking for previously installed user-mode driver...

    FILES=("/etc/ld.so.conf.d/libintelopencl.conf"
           "/etc/OpenCL/vendors/intel.icd"
           "/etc/profile.d/libintelopencl.sh"
           "/opt/intel/opencl")

    for file_ in "${FILES[@]}"; do
        echo rm -rf $file_
        rm -rf $file_
        if [[ $? -ne 0 ]]; then
            echo ERROR: failed to remove existing user-mode driver. >&2
            echo Please try again manually and run the script again. >&2
            exit $EXIT_FAIULRE
        fi
    done

    pkgs=$(dpkg-query -W -f='${binary:Package}\n' 'intel-opencl*')
    if [[ $? -eq 0 ]]; then
        echo Found $pkgs installed, uninstalling...
        dpkg --purge $pkgs
        if [[ $? -ne 0 ]]; then
            echo "ERROR: unable to remove $pkgs" >&2
            echo "       please resolve it manually and try to launch the script again." >&2
            exit $EXIT_FAILURE
        fi
    fi
}

uninstall_user_mode()
{
    if [[ $DISTRO == "centos" ]]; then
        _uninstall_user_mode_centos
    else
        _uninstall_user_mode_ubuntu
    fi
}

_is_package_installed()
{
    if [[ $DISTRO == "centos" ]]; then
        cmd="rpm -qa | grep $1"
    else
        cmd="dpkg-query -W -f='${binary:Package}\n' $pkg"
    fi
    echo $cmd
    eval $cmd
}

summary()
{
    echo
    echo Installation completed successfully.
    echo
    echo Next steps:
    echo "Add OpenCL users to the video group: 'sudo usermod -a -G video USERNAME'"
    echo "   e.g. if the user running OpenCL host applications is foo, run: sudo usermod -a -G video foo"
    echo

    if ! uname -a | grep -q -E "4.14"; then
        echo "Install 4.14 kernel using install_4_14_kernel.sh script and reboot into this kernel"
        echo
    fi

    echo "If you use 8th Generation Intel® Core™ processor, you will need to add:"
    echo "   i915.alpha_support=1"
    echo "   to the 4.14 kernel command line, in order to enable OpenCL functionality for this platform."
    echo
 
}

check_root_access()
{
    if [[ $EUID -ne 0 ]]; then
        echo "ERROR: you must run this script as root." >&2
        echo "Please try again with "sudo -E $0", or as root." >&2
        exit $EXIT_FAILURE
    fi
}

add_user_to_video_group()
{
    local real_user=$(logname 2>/dev/null || echo ${SUDO_USER:-${USER}})
    echo
    echo Adding $real_user to the video group...
    usermod -a -G video $real_user
    if [[ $? -ne 0 ]]; then
        echo WARNING: unable to add $real_user to the video group >&2
    fi
}

_check_distro_version()
{
    if [[ $DISTRO == centos ]]; then
        CENTOS_MINOR=$(sed 's/CentOS Linux release 7\.\([[:digit:]]\+\).\+/\1/' /etc/centos-release)
        if [[ $? -ne 0 ]]; then
            echo ERROR: failed to obtain CentOS version minor. >&2
            echo This script is supported only on CentOS 7 and above. >&2
            exit $EXIT_FAIULRE
        fi
    elif [[ $DISTRO == ubuntu ]]; then
        grep -q -E "16.04" /etc/lsb-release && UBUNTU_VERSION="16.04"
        if [[ -z $UBUNTU_VERSION ]]; then
            echo "ERROR: this script is supported only on 16.04." >&2
            exit $EXIT_FAILURE
        fi
    fi
}

distro_init()
{
    if [[ -f /etc/centos-release ]]; then
        DISTRO="centos"
    elif [[ -f /etc/lsb-release ]]; then
        DISTRO="ubuntu"
    fi

    _check_distro_version
}

install()
{
    uninstall_user_mode
    install_prerequisites
    install_user_mode
    add_user_to_video_group
}

main()
{
    echo "Intel OpenCL graphics driver installer"
    distro_init
    check_root_access
    install
    summary
}

[[ "$0" == "$BASH_SOURCE" ]] && main "$@"
