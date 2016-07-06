KSI COMMAND-LINE TOOL

Guardtime Keyless Signature Infrastructure (KSI) is an industrial scale
blockchain platform that cryptographically ensures data integrity and
proves time of existence. Its keyless signatures, based on hash chains,
link data to global calendar blockchain. The checkpoints of the blockchain,
published in newspapers and electronic media, enable long term integrity of
any digital asset without the need to trust any system. There are many
applications for KSI, a classical example is signing of any type of logs -
system logs, financial transactions, call records, etc. For more, see
https://guardtime.com

KSI command line tool enables the access to the KSI blockchain and its
functions from the shell. It can be used for signing data in files or other
sources, extending existing KSI signatures and verifying them using different
trust anchors.


INSTALLION

In order to install the packages from the Guardtime repository under CentOS /
RHEL, add the repository by creating a file /etc/yum.repos.d/ksi.repo with
the following contents:

  [ksi]
  name=KSI
  baseurl=https://download.guardtime.com/ksi/rhel/6/x86_64/
  enabled=1
  gpgcheck=1

Import the GPG key for verification of the packages and install packages as
usual:

  rpm --import https://verify.guardtime.com/GUARDTIME-GPG-KEY
  yum install ksi


If the latest version is needed or the package is not available for the
platform download the source code and build it using gcc or VS. To build
ksi tool libksi and libksi-devel packages are needed. Use rebuild.sh script
to build ksi tool on CentOS /RHEL. See WinBuild.txt to read how to build ksi
tool on Windows.

  
USAGE

In order to get trial access to the KSI platform, go to
https://guardtime.com/blockchain-developers

The first argument of the tool is the KSI command followed by the KSI service
configuration parameters and options. An example of obtaining a signature from
a data file 'data':

  ksi sign -i data -S http://example.gateway.com:3333/gt-signingservice

See ksi man page to read about usage in detail.


LICENSE

See LICENSE file.


DEPENDENCIES

Library   Version    License type  Source
libksi    3.9        Apache 2.0    https://github.com/GuardTime/libksi


COMPATIBILITY

OS / PLatform                       Compatibility
RHEL 6 and 7, x86_64 architecture   Fully compatible and tested
CentOS, x86_64 architecture         Fully Compatible and tested
Debian                              Compatible but not tested on regular basis
OS X                                Compatible but not tested on regular basis
Windows 7, 8, 10                    Compatible but not tested on regular basis
