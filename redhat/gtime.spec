%define	name	gtime
%define version 1.0
%define upstream_release 0


Summary: Command-line access to Guardtime Keyless Signature services
Name: %{name}
Version: %{version}
Release: UNSTABLE 
License: Apache 2.0
Group: Applications/Security
Source:  http://download.guardtime.com/%{name}-%{version}-%{upstream_release}.tar.gz
URL: http://www.guardtime.com/
Vendor: Guardtime AS
Packager: Guardtime AS <info@guardtime.com>
Distribution: Guardtime utilities
BuildRoot: %{_tmppath}/%{name}-%{version}-%{upstream_release}-build

Requires: openssl
Requires: curl
Requires: libksi
BuildRequires: openssl-devel
BuildRequires: curl-devel
BuildRequires: libksi-devel 

%description
Guardtime signing and verification tool. Execute 
gtime -h to view brief usage instructions.

%prep
%setup -q -n %{name}-%{version}-%{upstream_release}

%build
%configure
make

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}


%files
%defattr (-,root,root)

%{_bindir}/gtime*


%changelog
