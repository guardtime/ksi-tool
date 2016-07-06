# KSI COMMAND-LINE TOOL

Guardtime Keyless Signature Infrastructure (KSI) is an industrial scale
blockchain platform that cryptographically ensures data integrity and
proves time of existence. Its keyless signatures, based on hash chains,
link data to global calendar blockchain. The checkpoints of the blockchain,
published in newspapers and electronic media, enable long term integrity of
any digital asset without the need to trust any system. There are many
applications for KSI, a classical example is signing of any type of logs -
system logs, financial transactions, call records, etc. For more, see
[https://guardtime.com](https://guardtime.com)


KSI command line tool enables the access to the KSI blockchain and its
functions from the shell. It can be used for signing data in files or other
sources, extending existing KSI signatures and verifying them using different
trust anchors.


## INSTALLION

### From rpm package public repository
In order to install the ksi CentOS / RHEL packages directly from the Guardtime
public repository, download and save the repository configuration to the
/etc/yum.repos.d/ folder:

```
cd /etc/yum.repos.d

# In case of RHEL / CentOS 6
sudo curl -O http://download.guardtime.com/ksi/configuration/guardtime.el6.repo

# In case of RHEL / CentOS 7
sudo curl -O http://download.guardtime.com/ksi/configuration/guardtime.el7.repo

yum install ksi
```

### From source code

If the latest version is needed or the package is not available for the
platform download the source code and build it using gcc or VS. To build
ksi tool libksi and libksi-devel packages are needed. Use rebuild.sh script
to build ksi tool on CentOS /RHEL. See WinBuild.txt to read how to build ksi
tool on Windows.

  
## USAGE

In order to get trial access to the KSI platform, go to
[https://guardtime.com/blockchain-developers](https://guardtime.com/blockchain-developers)


The first argument of the tool is the KSI command followed by the KSI service
configuration parameters and options. An example of obtaining a signature from
a data file 'data':

```
  ksi sign -i data -S http://example.gateway.com:3333/gt-signingservice
```

See ksi man page to read about usage in detail.


## LICENSE

See LICENSE file.


## DEPENDENCIES

```
Library   Version    License type  Source

libksi    3.9>       Apache 2.0    https://github.com/GuardTime/libksi
```

## COMPATIBILITY

```
OS / PLatform                       Compatibility

RHEL 6 and 7, x86_64 architecture   Fully compatible and tested.
CentOS, x86_64 architecture         Fully Compatible and tested.
Debian                              Compatible but not tested on regular basis.
OS X                                Compatible but not tested on regular basis.
Windows 7, 8, 10                    Compatible but not tested on regular basis.
```
