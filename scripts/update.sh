#!/bin/bash
#[root@localhost ~]# rpm -qa | wc -l
#456
SOURCEDIR=/usr/src/PBX-1.8-RL94/
RELEASE=`rpm -q rocky-release`

function exec {
  "$@"
  local EXIT_CODE=$?
  if [ $EXIT_CODE -ne 0 ]; then
    echo "$* failed with exit code $EXIT_CODE." >&2
    exit
  fi
}

clear

#Modify the file permissions
chmod -R 755 $SOURCEDIR/*

if [[ $RELEASE = "rocky-release-9.4-1.5.el9.noarch" ]]; then
  cd $SOURCEDIR/packages/x86_64/5.14.0-427.13.1.el9_4.x86_64

  #Update system
  echo "Updating system..."
  #yum update
  exec rpm -Uvh --force kernel-modules-5.14.0-427.18.1.el9_4.x86_64.rpm \
                        kernel-core-5.14.0-427.18.1.el9_4.x86_64.rpm \
                        kernel-5.14.0-427.18.1.el9_4.x86_64.rpm \
                        kernel-modules-core-5.14.0-427.18.1.el9_4.x86_64.rpm \
                        selinux-policy-targeted-38.1.35-2.el9_4.0.2.noarch.rpm \
                        selinux-policy-38.1.35-2.el9_4.0.2.noarch.rpm \
                        libnghttp2-1.43.0-5.el9_4.3.x86_64.rpm \
                        less-590-4.el9_4.x86_64.rpm \
                        kernel-tools-libs-5.14.0-427.18.1.el9_4.x86_64.rpm \
                        kernel-tools-5.14.0-427.18.1.el9_4.x86_64.rpm \
                        glibc-langpack-en-2.34-100.el9_4.2.x86_64.rpm \
                        glibc-gconv-extra-2.34-100.el9_4.2.x86_64.rpm \
                        glibc-common-2.34-100.el9_4.2.x86_64.rpm \
                        glibc-2.34-100.el9_4.2.x86_64.rpm


  #Install software dependencies
  echo "Installing software dependencies..."
  #yum install kernel-devel
  exec rpm -Uvh --force pkgconf-pkg-config-1.7.3-10.el9.x86_64.rpm \
                        pkgconf-m4-1.7.3-10.el9.noarch.rpm \
                        pkgconf-1.7.3-10.el9.x86_64.rpm \
                        libpkgconf-1.7.3-10.el9.x86_64.rpm \
                        make-4.3-8.el9.x86_64.rpm \
                        perl-Text-ParseWords-3.30-460.el9.noarch.rpm \
                        perl-Exporter-5.74-461.el9.noarch.rpm \
                        perl-Pod-Simple-3.42-4.el9.noarch.rpm \
                        perl-File-Path-2.18-4.el9.noarch.rpm \
                        perl-Pod-Perldoc-3.28.01-461.el9.noarch.rpm \
                        perl-Getopt-Long-2.52-4.el9.noarch.rpm \
                        perl-Pod-Usage-2.01-4.el9.noarch.rpm \
                        perl-IO-Socket-IP-0.41-5.el9.noarch.rpm \
                        perl-Time-Local-1.300-7.el9.noarch.rpm \
                        perl-libnet-3.13-4.el9.noarch.rpm \
                        flex-2.6.4-9.el9.x86_64.rpm \
                        perl-Carp-1.50-460.el9.noarch.rpm \
                        perl-podlators-4.14-460.el9.noarch.rpm \
                        perl-Term-Cap-1.17-460.el9.noarch.rpm \
                        perl-Term-ANSIColor-5.01-461.el9.noarch.rpm \
                        perl-HTTP-Tiny-0.076-462.el9.noarch.rpm \
                        perl-Digest-1.19-4.el9.noarch.rpm \
                        perl-URI-5.09-3.el9.noarch.rpm \
                        perl-constant-1.33-461.el9.noarch.rpm \
                        perl-Encode-3.08-462.el9.x86_64.rpm \
                        perl-Data-Dumper-2.174-462.el9.x86_64.rpm \
                        perl-Scalar-List-Utils-1.56-461.el9.x86_64.rpm \
                        perl-IO-Socket-SSL-2.073-1.el9.noarch.rpm \
                        perl-Storable-3.21-460.el9.x86_64.rpm \
                        perl-PathTools-3.78-461.el9.x86_64.rpm \
                        perl-Mozilla-CA-20200520-6.el9.noarch.rpm \
                        perl-Text-Tabs+Wrap-2013.0523-460.el9.noarch.rpm \
                        perl-Digest-MD5-2.58-4.el9.x86_64.rpm \
                        perl-Pod-Escapes-1.07-460.el9.noarch.rpm \
                        openssl-devel-3.0.7-27.el9.x86_64.rpm \
                        perl-MIME-Base64-3.16-4.el9.x86_64.rpm \
                        perl-File-Temp-0.231.100-4.el9.noarch.rpm \
                        bison-3.7.4-5.el9.x86_64.rpm \
                        m4-1.4.19-1.el9.x86_64.rpm \
                        libmpc-1.2.1-4.el9.x86_64.rpm \
                        libxcrypt-devel-4.4.18-3.el9.x86_64.rpm \
                        perl-Socket-2.031-4.el9.x86_64.rpm \
                        perl-mro-1.23-481.el9.x86_64.rpm \
                        perl-libs-5.32.1-481.el9.x86_64.rpm \
                        perl-interpreter-5.32.1-481.el9.x86_64.rpm \
                        perl-POSIX-1.94-481.el9.x86_64.rpm \
                        perl-NDBM_File-1.15-481.el9.x86_64.rpm \
                        perl-IO-1.43-481.el9.x86_64.rpm \
                        perl-Fcntl-1.13-481.el9.x86_64.rpm \
                        perl-Errno-1.30-481.el9.x86_64.rpm \
                        perl-B-1.80-481.el9.x86_64.rpm \
                        perl-vars-1.05-481.el9.noarch.rpm \
                        perl-subs-1.03-481.el9.noarch.rpm \
                        perl-overloading-0.02-481.el9.noarch.rpm \
                        perl-overload-1.31-481.el9.noarch.rpm \
                        perl-if-0.60.800-481.el9.noarch.rpm \
                        perl-base-2.27-481.el9.noarch.rpm \
                        perl-Symbol-1.08-481.el9.noarch.rpm \
                        perl-SelectSaver-1.02-481.el9.noarch.rpm \
                        perl-IPC-Open3-1.21-481.el9.noarch.rpm \
                        perl-Getopt-Std-1.12-481.el9.noarch.rpm \
                        perl-FileHandle-2.03-481.el9.noarch.rpm \
                        perl-File-stat-1.09-481.el9.noarch.rpm \
                        perl-File-Basename-2.85-481.el9.noarch.rpm \
                        perl-Class-Struct-0.66-481.el9.noarch.rpm \
                        perl-AutoLoader-5.74-481.el9.noarch.rpm \
                        elfutils-libelf-devel-0.190-2.el9.x86_64.rpm \
                        perl-Net-SSLeay-1.92-2.el9.x86_64.rpm \
                        kernel-headers-5.14.0-427.18.1.el9_4.x86_64.rpm \
                        kernel-devel-5.14.0-427.18.1.el9_4.x86_64.rpm \
                        perl-parent-0.238-460.el9.noarch.rpm \
                        libzstd-devel-1.5.1-2.el9.x86_64.rpm \
                        zlib-devel-1.2.11-40.el9.x86_64.rpm \
                        gcc-11.4.1-3.el9.x86_64.rpm \
                        cpp-11.4.1-3.el9.x86_64.rpm \
                        glibc-headers-2.34-100.el9_4.2.x86_64.rpm \
                        glibc-devel-2.34-100.el9_4.2.x86_64.rpm

  #/contrib/scripts/install_prereq install
  exec rpm -Uvh --force unixODBC-devel-2.3.9-4.el9.x86_64.rpm \
                        libogg-devel-1.3.4-6.el9.x86_64.rpm \
                        bluez-libs-5.64-2.el9.x86_64.rpm \
                        zip-3.0-35.el9.x86_64.rpm \
                        python3-pyparsing-2.4.7-9.el9.noarch.rpm \
                        unzip-6.0-56.el9.x86_64.rpm \
                        libicu-67.1-9.el9.x86_64.rpm \
                        cyrus-sasl-2.1.27-21.el9.x86_64.rpm \
                        wget-1.21.1-7.el9.x86_64.rpm \
                        libtool-ltdl-2.4.6-45.el9.x86_64.rpm \
                        unixODBC-2.3.9-4.el9.x86_64.rpm \
                        libogg-1.3.4-6.el9.x86_64.rpm \
                        automake-1.16.2-8.el9.noarch.rpm \
                        autoconf-2.69-38.el9.noarch.rpm \
                        perl-Thread-Queue-3.14-460.el9.noarch.rpm \
                        perl-threads-shared-1.61-460.el9.0.1.x86_64.rpm \
                        perl-threads-2.25-460.el9.x86_64.rpm \
                        perl-DynaLoader-1.47-481.el9.x86_64.rpm \
                        perl-File-Find-1.37-481.el9.noarch.rpm \
                        perl-File-Copy-2.34-481.el9.noarch.rpm \
                        perl-File-Compare-1.100.600-481.el9.noarch.rpm \
                        emacs-filesystem-27.2-9.el9.noarch.rpm \
                        libvorbis-1.3.7-5.el9.x86_64.rpm \
                        speex-1.2.0-11.el9.x86_64.rpm \
                        lua-5.4.4-4.el9.x86_64.rpm \
                        expat-devel-2.5.0-2.el9_4.x86_64.rpm \
                        neon-0.31.2-11.el9.x86_64.rpm \
                        sqlite-devel-3.34.1-7.el9_3.x86_64.rpm \
                        sqlite-3.34.1-7.el9_3.x86_64.rpm \
                        mariadb-connector-c-config-3.2.6-1.el9_0.noarch.rpm \
                        mysql-common-8.0.36-1.el9_3.x86_64.rpm \
                        perl-Test-Harness-3.42-461.el9.noarch.rpm \
                        perl-ExtUtils-ParseXS-3.40-460.el9.noarch.rpm \
                        perl-CPAN-Meta-YAML-0.018-461.el9.noarch.rpm \
                        ghc-srpm-macros-1.5.0-6.el9.noarch.rpm \
                        perl-Math-BigInt-1.9998.18-460.el9.noarch.rpm \
                        python-srpm-macros-3.9-53.el9.noarch.rpm \
                        rust-srpm-macros-17-4.el9.noarch.rpm \
                        perl-Encode-Locale-1.05-21.el9.noarch.rpm \
                        perl-JSON-PP-4.06-4.el9.noarch.rpm \
                        efi-srpm-macros-6-2.el9_0.noarch.rpm \
                        fonts-srpm-macros-2.0.5-7.el9.1.noarch.rpm \
                        go-srpm-macros-3.2.0-3.el9.noarch.rpm \
                        kernel-srpm-macros-1.0-13.el9.noarch.rpm \
                        lua-srpm-macros-1-6.el9.noarch.rpm \
                        openblas-srpm-macros-2-11.el9.noarch.rpm \
                        libsamplerate-0.1.9-10.el9.x86_64.rpm \
                        libpq-devel-13.11-1.el9.x86_64.rpm \
                        libpq-13.11-1.el9.x86_64.rpm \
                        lm_sensors-libs-3.6.0-10.el9.x86_64.rpm \
                        lm_sensors-devel-3.6.0-10.el9.x86_64.rpm \
                        libxml2-devel-2.9.13-6.el9_4.x86_64.rpm \
                        mariadb-connector-c-3.2.6-1.el9_0.x86_64.rpm \
                        llvm-libs-17.0.6-5.el9.x86_64.rpm \
                        perl-ExtUtils-Manifest-1.73-4.el9.noarch.rpm \
                        perl-ExtUtils-Install-2.20-4.el9.noarch.rpm \
                        redhat-rpm-config-207-1.el9.noarch.rpm \
                        perl-CPAN-Meta-2.150010-460.el9.noarch.rpm \
                        perl-CPAN-Meta-Requirements-2.140-461.el9.noarch.rpm \
                        popt-devel-1.18-8.el9.x86_64.rpm \
                        perl-srpm-macros-1-41.el9.noarch.rpm \
                        rpm-devel-4.16.1.3-29.el9.x86_64.rpm \
                        perl-ExtUtils-MakeMaker-7.60-3.el9.noarch.rpm \
                        perl-ExtUtils-Command-7.60-3.el9.noarch.rpm \
                        perl-Devel-PPPort-3.62-4.el9.x86_64.rpm \
                        xz-devel-5.2.5-8.el9_0.x86_64.rpm \
                        ocaml-srpm-macros-6-6.el9.noarch.rpm \
                        cmake-filesystem-3.26.5-2.el9.x86_64.rpm \
                        net-snmp-libs-5.9.1-13.el9.x86_64.rpm \
                        net-snmp-devel-5.9.1-13.el9.x86_64.rpm \
                        net-snmp-agent-libs-5.9.1-13.el9.x86_64.rpm \
                        gsm-1.0.19-6.el9.x86_64.rpm \
                        annobin-12.31-2.el9.x86_64.rpm \
                        flac-libs-1.3.3-10.el9_2.1.x86_64.rpm \
                        libical-devel-3.0.14-1.el9.x86_64.rpm \
                        libical-3.0.14-1.el9.x86_64.rpm \
                        libsndfile-1.0.31-8.el9.x86_64.rpm \
                        libicu-devel-67.1-9.el9.x86_64.rpm \
                        libjpeg-turbo-2.0.90-7.el9.x86_64.rpm \
                        jbigkit-libs-2.1-23.el9.x86_64.rpm \
                        libtiff-devel-4.4.0-12.el9.x86_64.rpm \
                        libtiff-4.4.0-12.el9.x86_64.rpm \
                        libwebp-1.2.0-8.el9.x86_64.rpm \
                        dwz-0.14-3.el9.x86_64.rpm \
                        alsa-lib-devel-1.2.10-2.el9.x86_64.rpm \
                        alsa-lib-1.2.10-2.el9.x86_64.rpm \
                        newt-devel-0.52.21-11.el9.x86_64.rpm \
                        opus-1.3.1-10.el9.x86_64.rpm \
                        perl-lib-0.65-481.el9.x86_64.rpm \
                        perl-devel-5.32.1-481.el9.x86_64.rpm \
                        perl-I18N-Langinfo-0.19-481.el9.x86_64.rpm \
                        perl-locale-1.09-481.el9.noarch.rpm \
                        perl-doc-5.32.1-481.el9.noarch.rpm \
                        perl-Math-Complex-1.59-481.el9.noarch.rpm \
                        perl-ExtUtils-Constant-0.25-481.el9.noarch.rpm \
                        perl-Benchmark-1.23-481.el9.noarch.rpm \
                        perl-AutoSplit-5.74-481.el9.noarch.rpm \
                        slang-devel-2.3.2-11.el9.x86_64.rpm \
                        elfutils-devel-0.190-2.el9.x86_64.rpm \
                        libcurl-devel-7.76.1-29.el9_4.x86_64.rpm \
                        openldap-devel-2.6.6-3.el9.x86_64.rpm \
                        perl-version-0.99.28-4.el9.x86_64.rpm \
                        ncurses-devel-6.2-10.20210508.el9.x86_64.rpm \
                        ncurses-c++-libs-6.2-10.20210508.el9.x86_64.rpm \
                        systemtap-sdt-devel-5.0-4.el9.x86_64.rpm \
                        cyrus-sasl-devel-2.1.27-21.el9.x86_64.rpm \
                        perl-Time-HiRes-1.9764-462.el9.x86_64.rpm \
                        libstdc++-devel-11.4.1-3.el9.x86_64.rpm \
                        gcc-plugin-annobin-11.4.1-3.el9.x86_64.rpm \
                        gcc-c++-11.4.1-3.el9.x86_64.rpm \
                        qt5-srpm-macros-5.15.9-1.el9.noarch.rpm \
                        pyproject-srpm-macros-1.12.0-1.el9.noarch.rpm \
                        epel-release-9-7.el9.noarch.rpm \
                        freetds-1.3.3-1.el9.x86_64.rpm \
                        freetds-devel-1.3.3-1.el9.x86_64.rpm \
                        freetds-libs-1.3.3-1.el9.x86_64.rpm \
                        jack-audio-connection-kit-1.9.21-1.el9.x86_64.rpm \
                        jack-audio-connection-kit-devel-1.9.21-1.el9.x86_64.rpm \
                        libresample-0.1.3-39.el9.x86_64.rpm \
                        libresample-devel-0.1.3-39.el9.x86_64.rpm \
                        libsqlite3x-20071018-31.el9.x86_64.rpm \
                        libsqlite3x-devel-20071018-31.el9.x86_64.rpm \
                        portaudio-19-38.el9.x86_64.rpm \
                        portaudio-devel-19-38.el9.x86_64.rpm \
                        spandsp-0.0.6-13.el9.x86_64.rpm \
                        spandsp-devel-0.0.6-13.el9.x86_64.rpm \
                        libvorbis-devel-1.3.7-5.el9.x86_64.rpm \
                        speex-devel-1.2.0-11.el9.x86_64.rpm \
                        libtool-ltdl-devel-2.4.6-45.el9.x86_64.rpm \
                        lua-devel-5.4.4-4.el9.x86_64.rpm \
                        neon-devel-0.31.2-11.el9.x86_64.rpm \
                        mysql-libs-8.0.36-1.el9_3.x86_64.rpm \
                        mysql-devel-8.0.36-1.el9_3.x86_64.rpm \
                        bluez-libs-devel-5.64-2.el9.x86_64.rpm
  
  #yum install libresample libresample-devel (codec_resample)
  #exec rpm -Uvh --force libresample-0.1.3-20.gf.el7.x86_64.rpm \
  #                      libresample-devel-0.1.3-20.gf.el7.x86_64.rpm
  #yum install libtiff-devel (res_fax)
  #exec rpm -Uvh --force libtiff-devel-4.0.3-35.el7.x86_64.rpm
  #yum install verbio-clients (app_verbio_speech)
  exec rpm -Uvh --force verbio-clients-9.02-0.x86_64.rpm

  #Install software utilities
  echo "Installing software utilities..."
  #yum install pciutils
  exec rpm -Uvh --force pciutils-3.7.0-5.el9.x86_64.rpm
  #yum install lsof
  exec rpm -Uvh --force libtirpc-1.3.3-8.el9_4.x86_64.rpm \
                        lsof-4.94.0-3.el9.x86_64.rpm
  #yum install smartmontools
  exec rpm -Uvh --force smartmontools-7.2-9.el9.x86_64.rpm  
  #yum install sysstat
  exec rpm -Uvh --force avahi-libs-0.8-20.el9.x86_64.rpm  \
                        libuv-1.42.0-1.el9.x86_64.rpm     \
                        sysstat-12.5.4-7.el9.x86_64.rpm   \
                        pcp-conf-6.2.0-2.el9_4.x86_64.rpm \
                        pcp-libs-6.2.0-2.el9_4.x86_64.rpm
                        
  #yum install dos2unix
  #exec rpm -Uvh --force dos2unix-6.0.3-7.el7.x86_64.rpm
  #yum install nano
  exec rpm -Uvh --force nano-5.6.1-5.el9.x86_64.rpm 
  #yum install gdb
  exec rpm -Uvh --force libipt-2.0.4-5.el9.x86_64.rpm \
                        libbabeltrace-1.5.8-10.el9.x86_64.rpm \
                        source-highlight-3.1.9-11.el9.x86_64.rpm \
                        boost-regex-1.75.0-8.el9.x86_64.rpm \
                        gdb-10.2-13.el9.x86_64.rpm \
                        gdb-headless-10.2-13.el9.x86_64.rpm

  #yum install rpm-build
  exec rpm -Uvh --force   bzip2-1.0.8-8.el9.x86_64.rpm \
                          ed-1.14.2-12.el9.x86_64.rpm \
                          info-6.7-15.el9.x86_64.rpm \
                          elfutils-0.190-2.el9.x86_64.rpm \
                          zstd-1.5.1-2.el9.x86_64.rpm \
                          tar-1.34-6.el9_1.x86_64.rpm \
                          rpm-build-4.16.1.3-29.el9.x86_64.rpm \
                          lua-rpm-macros-1-6.el9.noarch.rpm \
                          debugedit-5.0-5.el9.x86_64.rpm \
                          patch-2.7.6-16.el9.x86_64.rpm


  #yum install iptables-services
  exec rpm -Uvh --force iptables-services-1.8.10-2.2.el9.noarch.rpm  
  #yum install net-tools
  exec rpm -Uvh --force net-tools-2.0-0.62.20160912git.el9.x86_64.rpm
  #yum install traceroute
  exec rpm -Uvh --force traceroute-2.1.0-18.el9.x86_64.rpm
  #yum install bind-utils
  exec rpm -Uvh --force bind-utils-9.16.23-18.el9_4.1.x86_64.rpm \
                        bind-license-9.16.23-18.el9_4.1.noarch.rpm \
                        bind-libs-9.16.23-18.el9_4.1.x86_64.rpm \
                        libmaxminddb-1.5.2-3.el9.x86_64.rpm \
                        fstrm-0.6.1-3.el9.x86_64.rpm \
                        protobuf-c-1.3.3-13.el9.x86_64.rpm
  #yum install ifplugd
  #rpmbuild --rebuild ifplugd-0.28-1.2.rf.src.rpm
  #/root/rpmbuild/RPMS/x86_64/ifplugd-0.28-1.2.el7.centos.x86_64.rpm
  #exec rpm -Uvh --force ifplugd-0.28-1.2.el7.centos.x86_64.rpm
  #yum install tcpdump
  exec rpm -Uvh --force libpcap-1.10.0-4.el9.x86_64.rpm  \
                        libibverbs-48.0-1.el9.x86_64.rpm \
                        tcpdump-4.99.0-9.el9.x86_64.rpm
  #yum install ngrep
  exec rpm -Uvh --force libnet-1.2-6.el9.x86_64.rpm \
                        ngrep-1.47-8.1.20180101git9b59468.el9.x86_64.rpm
  #yum install inotify-tools
  exec rpm -Uvh --force inotify-tools-3.22.1.0-1.el9.x86_64.rpm \
                        inotify-tools-devel-3.22.1.0-1.el9.x86_64.rpm
  #yum install ssmtp
  exec rpm -Uvh --force ssmtp-2.64-36.el9.x86_64.rpm
  #yum install vsftpd
  exec rpm -Uvh --force vsftpd-3.0.5-5.el9.x86_64.rpm 
  #yum install httpd
  exec rpm -Uvh --force mailcap-2.1.49-5.el9.noarch.rpm \
                        rocky-logos-httpd-90.15-2.el9.noarch.rpm \
                        mod_lua-2.4.57-8.el9.x86_64.rpm \
                        httpd-2.4.57-8.el9.x86_64.rpm \
                        httpd-tools-2.4.57-8.el9.x86_64.rpm \
                        httpd-filesystem-2.4.57-8.el9.noarch.rpm \
                        apr-util-openssl-1.6.1-23.el9.x86_64.rpm \
                        apr-util-bdb-1.6.1-23.el9.x86_64.rpm \
                        apr-util-1.6.1-23.el9.x86_64.rpm \
                        mod_http2-2.0.26-2.el9_4.x86_64.rpm \
                        apr-1.7.0-12.el9_3.x86_64.rpm \
                        httpd-core-2.4.57-8.el9.x86_64.rpm
  #yum install php
  exec rpm -Uvh --force nginx-filesystem-1.20.1-14.el9_2.1.noarch.rpm \
                        php-xml-8.0.30-1.el9_2.x86_64.rpm \
                        php-pdo-8.0.30-1.el9_2.x86_64.rpm \
                        libxslt-1.1.34-9.el9.x86_64.rpm \
                        php-opcache-8.0.30-1.el9_2.x86_64.rpm \
                        php-mbstring-8.0.30-1.el9_2.x86_64.rpm \
                        php-fpm-8.0.30-1.el9_2.x86_64.rpm \
                        php-common-8.0.30-1.el9_2.x86_64.rpm \
                        php-8.0.30-1.el9_2.x86_64.rpm \
                        php-cli-8.0.30-1.el9_2.x86_64.rpm
  #yum install ntp
  #exec rpm -Uvh --force autogen-libopts-5.18-5.el7.x86_64.rpm \
  #                      ntp-4.2.6p5-29.el7.centos.2.x86_64.rpm \
  #                      ntpdate-4.2.6p5-29.el7.centos.2.x86_64.rpm
  #yum install lame
  exec rpm -Uvh --force lame-3.100-12.el9.x86_64.rpm \
                        lame-libs-3.100-12.el9.x86_64.rpm
  #yum install jq
  #exec rpm -Uvh --force jq-1.6-7.el8.x86_64.rpm \
  #                      oniguruma-6.8.2-2.1.el8_9.x86_64.rpm

  #dnf module list postgresql
  #dnf module enable postgresql:9.6
  #dnf install postgresql-server
  exec rpm -Uvh --force postgresql-private-libs-13.14-1.el9_3.x86_64.rpm \
                        postgresql-13.14-1.el9_3.x86_64.rpm \
                        postgresql-server-13.14-1.el9_3.x86_64.rpm

  exec rpm -Uvh --force chkconfig-1.24-1.el9.x86_64.rpm
  
else
  echo "Release $RELEASE not supported"
  exit
fi

#rpm -iUvh http://dl.fedoraproject.org/pub/epel/7/x86_64/e/epel-release-7-2.noarch.rpm
#rpm -iUvh http://pkgs.repoforge.org/rpmforge-release/rpmforge-release-0.5.3-1.el7.rf.x86_64.rpm

#Convert scripts
#dos2unix $SOURCEDIR/*.sh
#dos2unix $SOURCEDIR/*.txt
#dos2unix $SOURCEDIR/*.conf

#Create ramdisk
echo "Creating ramdisk..."
mkdir /mnt/ramdisk
sed -e "s/none                    \/mnt\/ramdisk            tmpfs   defaults,size=2500M      0 0//" /etc/fstab > /etc/fstab.tmp && mv -f /etc/fstab.tmp /etc/fstab
echo "none                    /mnt/ramdisk            tmpfs   defaults,size=2500M      0 0" >> /etc/fstab

#Disable selinux
echo "Disabling selinux..."
cp -f /etc/selinux/config /etc/selinux/config.bak
sed -e "s/SELINUX=enforcing/SELINUX=disabled/" /etc/selinux/config > /etc/selinux/config.tmp && mv -f /etc/selinux/config.tmp /etc/selinux/config

#Configure services
echo "Configuring services..."
#List all services
#systemctl list-units -t service --all
#Start a service
#systemctl start dahdi.service
#Stop a service
#systemctl stop dahdi.service
#Restart a service
#systemctl restart dahdi.service
#Show service status
#systemctl status dahdi.service
#Enable service to be started on bootup
#systemctl enable dahdi.service
#Disable service to not start during bootup
#systemctl disable dahdi.service
#Check whether service is already enabled or not
#systemctl is-enabled dahdi.service; echo $?
systemctl mask firewalld #chkconfig firewalld off
systemctl enable iptables #chkconfig iptables on
systemctl enable ip6tables #chkconfig ip6tables on
#systemctl enable ntpd #chkconfig ntpd on
systemctl enable vsftpd #chkconfig vsftpd on
systemctl enable postgresql #chkconfig postgresql on

#Configure sshd service
echo "Configuring sshd service..."
cp -f /etc/ssh/sshd_config /etc/ssh/sshd_config.bak
sed -e "s/#UseDNS yes/UseDNS no/g" /etc/ssh/sshd_config > /etc/ssh/sshd_config.tmp && mv -f /etc/ssh/sshd_config.tmp /etc/ssh/sshd_config

#Configure PostgreSQL
postgresql-setup --initdb
systemctl start postgresql
systemctl enable postgresql
cp -f /var/lib/pgsql/data/postgresql.conf /var/lib/pgsql/data/postgresql.conf.bak
sed -e "s/#listen_addresses = 'localhost'/listen_addresses = '*'/g" /var/lib/pgsql/data/postgresql.conf > /var/lib/pgsql/data/postgresql.conf.tmp && mv -f /var/lib/pgsql/data/postgresql.conf.tmp /var/lib/pgsql/data/postgresql.conf
cp -f /var/lib/pgsql/data/pg_hba.conf /var/lib/pgsql/data/pg_hba.conf.bak
sed -e "s/local   all             all                                     peer/local   all             all                                     trust/g" /var/lib/pgsql/data/pg_hba.conf > /var/lib/pgsql/data/pg_hba.conf.tmp && mv -f /var/lib/pgsql/data/pg_hba.conf.tmp /var/lib/pgsql/data/pg_hba.conf
sed -e "s/host    all             all             127.0.0.1\/32            ident/host    all             all             127.0.0.1\/32            trust/g" /var/lib/pgsql/data/pg_hba.conf > /var/lib/pgsql/data/pg_hba.conf.tmp && mv -f /var/lib/pgsql/data/pg_hba.conf.tmp /var/lib/pgsql/data/pg_hba.conf
sed -e "s/host    all             all             ::1\/128                 ident/host    all             all             ::1\/128                 trust/g" /var/lib/pgsql/data/pg_hba.conf > /var/lib/pgsql/data/pg_hba.conf.tmp && mv -f /var/lib/pgsql/data/pg_hba.conf.tmp /var/lib/pgsql/data/pg_hba.conf
echo "host    all             all             10.88.8.0/24            trust" >> /var/lib/pgsql/data/pg_hba.conf
service postgresql restart

#Configure vsftpd service
#echo "Configuring vsftpd service..."
cp -f /etc/vsftpd/vsftpd.conf /etc/vsftpd/vsftpd.conf.bak
cp -f /etc/vsftpd/ftpusers /etc/vsftpd/ftpusers.bak
cp -f /etc/vsftpd/user_list /etc/vsftpd/user_list.bak
sed -e "s/anonymous_enable=YES/anonymous_enable=NO/g" /etc/vsftpd/vsftpd.conf > /etc/vsftpd/vsftpd.conf.tmp && mv -f /etc/vsftpd/vsftpd.conf.tmp /etc/vsftpd/vsftpd.conf
sed -e "s/#chroot_local_user=YES/chroot_local_user=NO/g" /etc/vsftpd/vsftpd.conf > /etc/vsftpd/vsftpd.conf.tmp && mv -f /etc/vsftpd/vsftpd.conf.tmp /etc/vsftpd/vsftpd.conf
sed -e "s/userlist_deny=YES//" /etc/vsftpd/vsftpd.conf > /etc/vsftpd/vsftpd.conf.tmp && mv -f /etc/vsftpd/vsftpd.conf.tmp /etc/vsftpd/vsftpd.conf
echo "userlist_deny=YES" >> /etc/vsftpd/vsftpd.conf
sed -e "s/root/#root/g" /etc/vsftpd/ftpusers > /etc/vsftpd/ftpusers.tmp && mv -f /etc/vsftpd/ftpusers.tmp /etc/vsftpd/ftpusers
sed -e "s/root/#root/g" /etc/vsftpd/user_list > /etc/vsftpd/user_list.tmp && mv -f /etc/vsftpd/user_list.tmp /etc/vsftpd/user_list
sed -e "s/reverse_lookup_enable=NO//" /etc/vsftpd/vsftpd.conf > /etc/vsftpd/vsftpd.conf.tmp && mv -f /etc/vsftpd/vsftpd.conf.tmp /etc/vsftpd/vsftpd.conf
echo "reverse_lookup_enable=NO" >> /etc/vsftpd/vsftpd.conf

#Configure smtp
echo "Configuring smtp..."
SMTPSERVER=smtp.gmail.com:587
SMTPUSER=voicemail@neotel.com.ar
SMTPPASSWORD=qndi39sn2A
SMTPTESTUSER=mgines@neotel.com.ar

sed -e "s/root=postmaster/#root=postmaster/" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
sed -e "s/mailhub=mail/#mailhub=mail/" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
sed -e "s/root=$SMTPUSER//" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
echo "root=$SMTPUSER" >> /etc/ssmtp/ssmtp.conf
sed -e "s/mailhub=$SMTPSERVER//" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
echo "mailhub=$SMTPSERVER" >> /etc/ssmtp/ssmtp.conf
sed -e "s/FromLineOverride=YES//" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
echo "FromLineOverride=YES" >> /etc/ssmtp/ssmtp.conf
sed -e "s/UseTLS=YES//" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
echo "UseTLS=YES" >> /etc/ssmtp/ssmtp.conf
sed -e "s/UseSTARTTLS=YES//" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
echo "UseSTARTTLS=YES" >> /etc/ssmtp/ssmtp.conf
sed -e "s/TLS_CA_File=\/etc\/pki\/tls\/certs\/ca-bundle.crt//" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
echo "TLS_CA_File=/etc/pki/tls/certs/ca-bundle.crt" >> /etc/ssmtp/ssmtp.conf
sed -e "s/AuthUser=$SMTPUSER//" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
echo "AuthUser=$SMTPUSER" >> /etc/ssmtp/ssmtp.conf
sed -e "s/AuthPass=$SMTPPASSWORD//" /etc/ssmtp/ssmtp.conf > /etc/ssmtp/ssmtp.conf.tmp && mv -f /etc/ssmtp/ssmtp.conf.tmp /etc/ssmtp/ssmtp.conf
echo "AuthPass=$SMTPPASSWORD" >> /etc/ssmtp/ssmtp.conf

echo "Validating smtp..."
echo "To: $SMTPTESTUSER" > /etc/ssmtp/message.txt
echo "From: $SMTPUSER" > /etc/ssmtp/message.txt
echo "Subject: Test" >> /etc/ssmtp/message.txt
echo "" >> /etc/ssmtp/message.txt
echo "Hi there, this is a test message." >> /etc/ssmtp/message.txt
ssmtp -v $SMTPTESTUSER < /etc/ssmtp/message.txt

#Configure scheduled tasks
echo "Configuring scheduled tasks..."
cp -f $SOURCEDIR/patches/8.16.0/cron/root /var/spool/cron/root
chmod 644 /var/spool/cron/root
service crond restart

#reboot