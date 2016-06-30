Name:       org.tizen.email
#VCS_FROM:   profile/mobile/apps/native/email-app#30f8d6295c1537bc9751953d3c2f77a9d6d786a3
#RS_Ver:    20160630_3 
Summary:    Native email application
Version:    0.9.34
Release:    1
Group:      Applications/Messaging
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

ExcludeArch:  aarch64 x86_64
BuildRequires:  pkgconfig(libtzplatform-config)
Requires(post):  /usr/bin/tpk-backend

%define internal_name org.tizen.email
%define preload_tpk_path %{TZ_SYS_RO_APP}/.preload-tpk 

%ifarch i386 i486 i586 i686 x86_64
%define target i386
%else
%ifarch arm armv7l aarch64
%define target arm
%else
%define target noarch
%endif
%endif

%description
profile/mobile/apps/native/email-app#30f8d6295c1537bc9751953d3c2f77a9d6d786a3
This is a container package which have preload TPK files

%prep
%setup -q

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/%{preload_tpk_path}
install %{internal_name}-%{version}-%{target}.tpk %{buildroot}/%{preload_tpk_path}/

%post

%files
%defattr(-,root,root,-)
%{preload_tpk_path}/*
