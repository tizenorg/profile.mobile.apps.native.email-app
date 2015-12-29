
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

# Used for package build configuration
# set value 0 for Tizen 3.0 build
# set value 1 for Tizen 2.4 build
%define _tizen_2_4 0

%define _enable_attach_panel 1

Requires: email-service
BuildRequires: cmake
BuildRequires: edje-tools
BuildRequires: gettext-tools
BuildRequires: pkgconfig(capi-base-utils-i18n)
BuildRequires: pkgconfig(email-service)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(ecore-evas)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-appfw-preference)
BuildRequires: pkgconfig(capi-base-common)
BuildRequires: pkgconfig(contacts-service2)
BuildRequires: pkgconfig(accounts-svc)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(capi-media-metadata-extractor)
BuildRequires: pkgconfig(capi-media-image-util)
BuildRequires: pkgconfig(capi-content-media-content)
BuildRequires: pkgconfig(libexif)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-network-wifi)
BuildRequires: pkgconfig(libpng)
BuildRequires: pkgconfig(storage)
BuildRequires: pkgconfig(capi-content-mime-type)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(feedback)

%if 0%{?_tizen_2_4}
BuildRequires: pkgconfig(ewebkit2)
%else
BuildRequires: pkgconfig(chromium-efl)
BuildRequires: pkgconfig(libtzplatform-config)
%endif

%if 0%{?_enable_attach_panel}
BuildRequires: pkgconfig(attach-panel)
%endif

%description
Description: Native email application

%prep
%setup -q

%build

%if 0%{?_tizen_2_4}
%define _opt_dir /opt
%define _usr_dir /usr
%define _share_dir %{_usr_dir}/share
%define _usr_apps_dir %{_usr_dir}/apps

%define _pkg_dir %{_usr_apps_dir}/org.tizen.email
%define _pkg_lib_dir %{_pkg_dir}/lib
%define _pkg_shared_dir %{_pkg_dir}/shared
%define _opt_pkg_dir %{_opt_dir}/%{_pkg_dir}
%define _sys_icons_dir %{_share_dir}/icons/default/small
%define _sys_packages_dir %{_share_dir}/packages
%define _sys_license_dir %{_share_dir}/license
%define _sys_smack_dir /etc/smack/accesses.d

%else
%define _pkg_dir %{TZ_SYS_RO_APP}/%{name}
%define _pkg_lib_dir %{_pkg_dir}/lib
%define _sys_icons_dir %{TZ_SYS_RO_ICONS}/default/small
%define _sys_packages_dir %{TZ_SYS_RO_PACKAGES}
%define _sys_license_dir %{TZ_SYS_SHARE}/license
%endif

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
	-DSYS_ICONS_DIR=%{_sys_icons_dir} \
	-DSYS_PACKAGES_DIR=%{_sys_packages_dir} \
%if 0%{?_tizen_2_4}
	-DTIZEN_2_4=%{_tizen_2_4} \
	-DSYS_LICENSE_DIR=%{_sys_license_dir} \
	-DSYS_SMACK_DIR=%{_sys_smack_dir}
%else
	-DSYS_LICENSE_DIR=%{_sys_license_dir}
%endif

make

%install
rm -rf %{buildroot}
%if 0%{?_tizen_2_4}
mkdir -p %{buildroot}/%{_opt_pkg_dir}/data
mkdir -p %{buildroot}/%{_opt_pkg_dir}/cache
mkdir -p %{buildroot}/%{_opt_pkg_dir}/shared/data
%endif
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

%if 0%{?_tizen_2_4}
chown -R 5000:5000 %{_opt_pkg_dir}/data
chown -R 5000:5000 %{_opt_pkg_dir}/cache
chown -R 5000:5000 %{_opt_pkg_dir}/shared/data
%else
pkgdir_maker --create --pkgid=%{name}
%endif

%files
%defattr(-,root,root,-)
%manifest %{_tmp_buld_dir}/%{name}.manifest

%if 0%{?_tizen_2_4}
%dir %{_opt_pkg_dir}/data
%dir %{_opt_pkg_dir}/cache
%dir %{_opt_pkg_dir}/shared/data
%{_sys_smack_dir}/%{name}.efl
%endif

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

%{_sys_icons_dir}/%{name}.png
%{_sys_packages_dir}/%{name}.xml
%{_sys_license_dir}/%{name}