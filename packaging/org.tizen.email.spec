
Name: org.tizen.email
Summary: Native email application
Version : 0.0.1
Release: 1
Group: Applications/Messaging
License: Apache-2.0
Source0: %{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "wearable" || "%{?tizen_profile_name}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%define _enable_attach_panel 0

Requires(post): sys-assert
Requires: email-service
BuildRequires: cmake
BuildRequires: edje-tools
BuildRequires: embryo-bin
BuildRequires: gettext-tools
BuildRequires: boost-devel
BuildRequires: hash-signer
BuildRequires: pkgconfig(capi-base-utils-i18n)
BuildRequires: pkgconfig(email-service)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(ecore-evas)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(gthread-2.0)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(chromium-efl)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-appfw-preference)
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(capi-base-common)
BuildRequires: pkgconfig(contacts-service2)
BuildRequires: pkgconfig(accounts-svc)
BuildRequires: pkgconfig(calendar-service2)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(capi-media-metadata-extractor)
BuildRequires: pkgconfig(capi-media-image-util)
BuildRequires: pkgconfig(capi-content-media-content)
BuildRequires: pkgconfig(libexif)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(xmlsec1)
BuildRequires: pkgconfig(xmlsec1-openssl)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(tapi)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-network-wifi)
BuildRequires: pkgconfig(libpng)
BuildRequires: pkgconfig(storage)
BuildRequires: pkgconfig(capi-content-mime-type)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(libtzplatform-config)

%if 0%{?_enable_attach_panel}
BuildRequires: pkgconfig(attach-panel)
%endif

%description
Description: Native email application

%prep
%setup -q

%build

%define _pkg_dir %{TZ_SYS_RO_APP}/%{name}
%define _pkg_lib_dir %{_pkg_dir}/lib

export CFLAGS="${CFLAGS} -Wall -fvisibility=hidden"
export CXXFLAGS="${CXXFLAGS} -Wall -fvisibility=hidden"
export LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--rpath=%{_pkg_lib_dir} -Wl,--as-needed -Wl,-zdefs"

%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="${CFLAGS} -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="${CXXFLAGS} -DTIZEN_DEBUG_ENABLE"
export FFLAGS="${FFLAGS} -DTIZEN_DEBUG_ENABLE"
%endif

%define _tmp_buld_dir TEMP_BUILD_DIR/%{_project}-%{_arch}

mkdir -p %{_tmp_buld_dir}
cd %{_tmp_buld_dir}

cmake ../../CMake -DCMAKE_INSTALL_PREFIX=%{_pkg_dir} \
	-DENABLE_ATTACH_PANEL=%{_enable_attach_panel} \
	-DSYS_ICONS_DIR=%{TZ_SYS_RO_ICONS} \
	-DSYS_PACKAGES_DIR=%{TZ_SYS_RO_PACKAGES} \
	-DSYS_SHARE_DIR=%{TZ_SYS_SHARE}
make

%install
rm -rf %{buildroot}
cd %{_tmp_buld_dir}

%make_install

%define tizen_sign 1
%define tizen_sign_base %{_pkg_dir}
%define tizen_sign_level platform
%define tizen_author_sign 1
%define tizen_dist_sign 1

%clean
rm -f debugfiles.list debuglinks.list debugsources.list

%post
pkgdir_maker --create --pkgid=%{name}

%files
%defattr(-,root,root,-)
%manifest %{name}.manifest

%{_pkg_lib_dir}/*.so

%{_pkg_dir}/res/edje/email-common-theme.edj
%{_pkg_dir}/res/edje/email-setting-theme.edj
%{_pkg_dir}/res/edje/email-composer-view.edj
%{_pkg_dir}/res/edje/email-viewer.edj
%{_pkg_dir}/res/edje/email-mailbox.edj
%{_pkg_dir}/res/edje/email-account.edj
%{_pkg_dir}/res/edje/email-filter.edj
%{_pkg_dir}/res/locale/*/LC_MESSAGES/*
%{_pkg_dir}/res/images/*
%{_pkg_dir}/res/misc/*

%{_pkg_dir}/bin/email
%{_pkg_dir}/bin/email-setting
%{_pkg_dir}/bin/email-composer

%{TZ_SYS_RO_ICONS}/%{name}.png
%{TZ_SYS_RO_PACKAGES}/%{name}.xml
%{TZ_SYS_SHARE}/license/%{name}

%{TZ_SYS_SMACK}/accesses.d/%{name}.efl

%{_pkg_dir}/author-signature.xml
%{_pkg_dir}/signature1.xml
