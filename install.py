#!/usr/bin/python
"""

Copyright (c) 2018 Intel Corporation All Rights Reserved.

THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: VAinstaller.py

Abstract: Complete install for Intel Visual Analytics, including
 * Intel(R) Media SDK
 * Intel(R) Computer Vision SDK or Intel(R) Deep Learning Deployment Toolkit
 * Drivers
 * Prerequisites
"""

import os, sys, platform
import os.path
import argparse
import grp

class diagnostic_colors:
    ERROR   = '\x1b[31;1m'  # Red/bold
    SUCCESS = '\x1b[32;1m'  # green/bold
    RESET   = '\x1b[0m'     # Reset attributes
    INFO    = '\x1b[34;1m'  # info
    OUTPUT  = ''            # command's coutput printing
    STDERR  = '\x1b[36;1m'  # cyan/bold
    SKIPPED = '\x1b[33;1m'  # yellow/bold

class loglevelcode:
    ERROR   = 0
    SUCCESS = 1
    INFO    = 2

GLOBAL_LOGLEVEL=3

def print_info( msg, loglevel ):
    global GLOBAL_LOGLEVEL

    """ printing information """    
    
    if loglevel==loglevelcode.ERROR and GLOBAL_LOGLEVEL>=0:
        color = diagnostic_colors.ERROR
        msgtype=" [ ERROR ] "
        print( color + msgtype + diagnostic_colors.RESET + msg )
    elif loglevel==loglevelcode.SUCCESS and GLOBAL_LOGLEVEL>=1:
        color = diagnostic_colors.SUCCESS
        msgtype=" [ OK ] "
        print( color + msgtype + diagnostic_colors.RESET + msg )
    elif loglevel==loglevelcode.INFO and GLOBAL_LOGLEVEL>=2:
        color = diagnostic_colors.INFO
        msgtype=" [ INFO ] "
        print( color + msgtype + diagnostic_colors.RESET + msg )


    
    return

def run_cmd(cmd):
    output=""
    fin=os.popen(cmd+" 2>&1","r")
    for line in fin:
        output+=line
    fin.close()
    return output

def find_library(libfile):
    search_path=os.environ.get("LD_LIBRARY_PATH","/usr/lib64")
    if not ('/usr/lib64' in search_path):
	search_path+=";/usr/lib64"
    paths=search_path.split(";")

    found=False
    for libpath in paths:
        if os.path.exists(libpath+"/"+libfile):
            found=True
            break
    
    return found

def fnParseCommandline():
    if len(sys.argv) == 1:		
        return "-all"
    elif len(sys.argv) > 3:
        return "not support"
        exit(0)
	
    if sys.argv[1] == "-h":
        print "[%s" % sys.argv[0] + " usage]"
        print "\t -h: display help"
        print "\t -all: install all components"
        print "\t -b BUILT_TARGET"
        print "\t -nomsdk: skip prerequisites for MSDK"
        print "\t -cvsdk: install legacy cvsdk version r3"
        exit(0)

    return sys.argv[1]

if __name__ == "__main__":

    if os.getuid() != 0:
        exit("Must be run as root. Exiting.")

    WORKING_DIR=run_cmd("pwd").strip()
    msg_tmp = "Working directory: " + WORKING_DIR
    print_info(msg_tmp, loglevelcode.INFO)

    BUILD_TARGET=""
    enable_msdk = True
    install_cvsdk = False

    cmd = fnParseCommandline()

    if cmd == "-b":
        BUILD_TARGET=sys.argv[2]
        print "BUILD_TARGET=", BUILD_TARGET
    elif cmd == "-nomsdk":
        print_info("Skip prerequisites installation for MediaSDK interop tutorials", loglevelcode.INFO)
        enable_msdk = False
    elif cmd == "-cvsdk":
        print_info("Install legacy CVSDK R3 package", loglevelcode.INFO)
        install_cvsdk = True

    print ""
    print "************************************************************************"
    print_info("Install required tools and create build environment.", loglevelcode.INFO)
    print "************************************************************************"
   
    # Install the necessary tools
    #$os.system("sudo add-apt-repository -y ppa:teejee2008/ppa")
    #cmd="apt-get update"
    #os.system(cmd)
    
    #cmd ="apt-get -y install git libssl-dev dh-autoreconf cmake libgl1-mesa-dev libpciaccess-dev build-essential curl imagemagick gedit mplayer unzip yasm libjpeg-dev linux-firmware ukuu; "
    #cmd+="apt-get -y install libopencv-dev checkinstall pkg-config libgflags-dev"
    #os.system(cmd)

    if enable_msdk == True:
        # Firmware update  
        cmd = "git clone https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git"
        os.system(cmd);

        if BUILD_TARGET == "BXT":
            cmd = "cp linux-firmware/i915/bxt_dmc_ver1_07.bin /lib/firmware/i915/.; "
            cmd+= "rm -f /lib/firmware/i915/bxt_dmc_ver1.bin; "
            cmd+= "ln -s /lib/firmware/i915/bxt_dmc_ver1_07.bin /lib/firmware/i915/bxt_dmc_ver1.bin"
        else:
            cmd = "cp linux-firmware/i915/skl_dmc_ver1_27.bin /lib/firmware/i915/.; "
            cmd+= "rm -f /lib/firmware/i915/skl_dmc_ver1.bin; "
            cmd+= "ln -s /lib/firmware/i915/skl_dmc_ver1_27.bin /lib/firmware/i915/skl_dmc_ver1.bin"

        os.system(cmd)

    # Install kernel
    os.system("ukuu --install v4.14.16")

    # Create the build directory for UMD
    os.system("mkdir -p %s/build"%(WORKING_DIR))

    if enable_msdk == True:
        print ""
        print "************************************************************************"
        print_info("Pull all the source code.", loglevelcode.INFO)
        print "************************************************************************"

        # Pull all the source code
        print "libva"
        if not os.path.exists("%s/libva"%(WORKING_DIR)):
            cmd = "cd %s; git clone https://github.com/01org/libva.git"%(WORKING_DIR) 
            print cmd
            os.system(cmd);

        print "libva-utils"
        if not os.path.exists("%s/libva-utils"%(WORKING_DIR)):
            cmd = "cd %s; git clone https://github.com/01org/libva-utils.git"%(WORKING_DIR)
            print cmd
            os.system(cmd);

        print "media-driver"
        if not os.path.exists("%s/media-driver"%(WORKING_DIR)): 
            cmd = "cd %s; git clone https://github.com/intel/media-driver.git; "%(WORKING_DIR)
            print cmd
            os.system(cmd);

        print "gmmlib"
        if not os.path.exists("%s/gmmlib"%(WORKING_DIR)): 
            cmd = "cd %s; git clone https://github.com/intel/gmmlib.git; "%(WORKING_DIR)
            print cmd
            os.system(cmd);

        print "MediaSDK"
        if not os.path.exists("%s/MediaSDK"%(WORKING_DIR)): 
            cmd = "cd %s; git clone https://github.com/Intel-Media-SDK/MediaSDK.git; "%(WORKING_DIR)
            print cmd
            os.system(cmd);

        print "tbb"
        if not os.path.exists("%s/tbb"%(WORKING_DIR)): 
            cmd = "cd %s; git clone https://github.com/01org/tbb.git; "%(WORKING_DIR)
            print cmd
            os.system(cmd);

        print ""
        print "************************************************************************"
        print_info("Build and Install libVA", loglevelcode.INFO)
        print "************************************************************************"

        # Build and install libVA including the libVA utils for vainfo.
        # libVA origin:fbf7138389f7d6adb6ca743d0ddf2dbc232895f6 (011118), libVA utils origin: 7b85ff442d99c233fb901a6fe3407d5308971645 (011118)
        cmd ="cd %s/libva; "%(WORKING_DIR)
        #cmd+="git checkout 95aa89b80c0a209bf8be1f7e307e4ca210ef4cf0; " # 030718
        cmd+="./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu; make -j4; make install"
        print cmd
        os.system(cmd)

        cmd ="cd %s/libva-utils; "%(WORKING_DIR)
        #cmd+="git checkout c77a46cd92c9497a6f0d3d48a50de0d1bda584f8; " # 030718
        cmd+="./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu; make -j4; make install"
        print cmd
        os.system(cmd)

        print ""		
        print "************************************************************************"
        print_info("Build and Install media driver", loglevelcode.INFO)
        print "************************************************************************"

        # Build and install media driver
        # gmmlib origin: b1451bbe4c506f17ddc819f12b4b448fc08698c5 (011118), media driver origin: 2eab2e248c5787ceaebd76be791e60e28e56fbf3 (011218)
        cmd = "cd %s/gmmlib; "%(WORKING_DIR)
        #cmd+= "git checkout b8d74ac3fcea0dffa901ce62399d43a937212074" # 022318
        print cmd
        os.system(cmd)

        cmd = "cd %s/media-driver; "%(WORKING_DIR)
        #cmd+= "git checkout 6932fc0ffb8228245052528820adafb6743f7482" # 030918
        print cmd
        os.system(cmd)

        cmd = "cd %s/build; "%(WORKING_DIR)
        cmd+= "cmake ../media-driver -DMEDIA_VERSION=\"2.0.0\" -DBUILD_ALONG_WITH_CMRTLIB=1 -DBS_DIR_GMMLIB=`pwd`/../gmmlib/Source/GmmLib/ -DBS_DIR_COMMON=`pwd`/../gmmlib/Source/Common/ -DBS_DIR_INC=`pwd`/../gmmlib/Source/inc/ -DBS_DIR_MEDIA=`pwd`/../media-driver -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=/usr/lib/x86_64-linux-gnu -DINSTALL_DRIVER_SYSCONF=OFF -DLIBVA_DRIVERS_PATH=/usr/lib/x86_64-linux-gnu/dri; "
        cmd+="make -j4; make install"
        print cmd
        os.system(cmd)

        print ""
        print "************************************************************************"
        print_info("Build and Install Media SDK and samples", loglevelcode.INFO)
        print "************************************************************************"

        # Build and install Media SDK library and samples
        # MediaSDK origin: 22dae397ae17d29b4734adb0e5a998f32a9c25b2 (022218)
        cmd = "cd %s/MediaSDK; "%(WORKING_DIR)
        #cmd+= "git checkout 4c5f8a3167eadcf2f2afd47ac822b6341e6fff52" # 030118
        print cmd
        os.system(cmd)

        cmd ="cd %s/MediaSDK; "%(WORKING_DIR)
        cmd+="export MFX_HOME=%s/MediaSDK; "%(WORKING_DIR)
        cmd+="perl tools/builder/build_mfx.pl --cmake=intel64.make.release --api latest ; "
        if len(BUILD_TARGET)<2:
            cmd+="make -C __cmake/intel64.make.release/ -j4; "
        else:
            cmd+="make -C __cmake/intel64.make.release/ -j4 --target=%s; "%(BUILD_TARGET)
        print cmd
        os.system(cmd)

        cmd ="cd %s/MediaSDK/__cmake/intel64.make.release; make install"%(WORKING_DIR)
        print cmd
        os.system(cmd)

        print ""
        print "************************************************************************"
        print_info("Build and Install tbb", loglevelcode.INFO)
        print "************************************************************************"

        # Build and install tbb.
        # tbb origin: 4c73c3b5d7f78c40f69e0c04fd4afb9f48add1e6 (120617)
        cmd = "cd %s/tbb; "%(WORKING_DIR)
        cmd+= "git checkout 4c73c3b5d7f78c40f69e0c04fd4afb9f48add1e6; " # 120617
        cmd+= "make tbb tbbmalloc"
        print cmd
        os.system(cmd)

        cmd = "cd %s/tbb; "%(WORKING_DIR)	
        cmd+= "export TBBROOT=/opt/intel/tbb; "
        cmd+= "mkdir $TBBROOT; mkdir $TBBROOT/bin; mkdir $TBBROOT/lib; "
        cmd+= "cp -r include $TBBROOT; "
        cmd+= "cp build/linux_*_release/libtbbmalloc* $TBBROOT/lib; "
        cmd+= "cp build/linux_*_release/libtbb.so* $TBBROOT/lib"
        print cmd
        os.system(cmd)

    if install_cvsdk == True:
        print ""
        print "************************************************************************"
        print_info("Install Intel OpenCL driver and CV SDK(option)", loglevelcode.INFO)
        print "************************************************************************"

        #add users to video group.  starting with sudo group members.  TODO: anyone else?
        for username in grp.getgrnam("sudo").gr_mem:
            cmd="usermod -a -G video %s"%(username)
            print cmd
            os.system(cmd)

        if not os.path.exists("intel-opencl"):
            print_info("downloading Intel(R) OpenCL SRB5.0 driver", loglevelcode.INFO) 
            cmd="curl -# -O http://registrationcenter-download.intel.com/akdlm/irc_nas/11396/SRB5.0_linux64.zip"
            print cmd
            os.system(cmd)

        print_info("installing Intel(R) OpenCL SRB5.0 driver", loglevelcode.INFO)
        cmd = "unzip -x SRB5.0_linux64.zip; "
        cmd+= "mkdir -p %s/intel-opencl; "%(WORKING_DIR)
        cmd+= "tar -C intel-opencl -Jxf intel-opencl-cpu-r5*.tar.xz; "
        cmd+= "tar -C intel-opencl -Jxf intel-opencl-devel-r5*.tar.xz; "
        cmd+= "tar -C intel-opencl -Jxf intel-opencl-r5*.tar.xz; "
        cmd+= "cp -R intel-opencl/* /; "
        cmd+= "ldconfig"
        print cmd
        os.system(cmd)
    
        if not os.path.exists("l_intel_cv_sdk_p_2018.0.211"):
            print_info("downloading Intel(R) Computer Vision SDK", loglevelcode.INFO) 
            cmd="curl -# -O http://registrationcenter-download.intel.com/akdlm/irc_nas/12492/l_intel_cv_sdk_p_2018.0.211.tgz"
            print cmd
            os.system(cmd)

        print_info("installing Intel(R) Computer Vision SDK RC2", loglevelcode.INFO) 
        cmd ="tar -xzf l_intel_cv_sdk_p_2018.0.211.tgz;"
        cmd+="cp silent.cfg l_intel_cv_sdk_p_2018.0.211;"
        cmd+="cd l_intel_cv_sdk_p_2018.0.211;"
	print "i'm here1"
        cmd+="./install.sh -s silent.cfg; "
        print "i'm here2"
        print cmd
        os.system(cmd)

    if enable_msdk == True:
        print ""
        print "************************************************************************"
        print_info("Set environment variables. ", loglevelcode.INFO)
        print "************************************************************************"
        cmd = "echo 'export MFX_HOME=/opt/intel/mediasdk' >> ~/.bashrc; "
        cmd+= "echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu:/usr/lib' >> ~/.bashrc; "
        cmd+= "echo 'export LIBVA_DRIVERS_PATH=/usr/lib/x86_64-linux-gnu/dri' >> ~/.bashrc; "
        cmd+= "echo 'export LIBVA_DRIVER_NAME=iHD' >> ~/.bashrc"
        print cmd
        os.system(cmd)

        cmd = "echo '/usr/lib/x86_64-linux-gnu' > /etc/ld.so.conf.d/libdrm-intel.conf; "
        cmd+= "echo '/usr/lib' >> /etc/ld.so.conf.d/libdrm-intel.conf; "
        cmd+= "ldconfig"
        print cmd
        os.system(cmd)

        cmd = "export TBBROOT=/opt/intel/tbb; "
        cmd+= "echo 'export TBBROOT='$TBBROOT > $TBBROOT/bin/tbbvars.sh; "
        cmd+= "echo 'export LIBRARY_PATH='$TBBROOT/lib >> $TBBROOT/bin/tbbvars.sh; "
        cmd+= "echo 'export CPATH='$TBBROOT/include >> $TBBROOT/bin/tbbvars.sh; "
        cmd+= "chmod +x $TBBROOT/bin/tbbvars.sh; "
        cmd+= "echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:'$TBBROOT/lib >> ~/.bashrc;"
        print cmd
        os.system(cmd)

    print "************************************************************************"
    print "    Done, all installation, please reboot system !!! "
    print "************************************************************************"

