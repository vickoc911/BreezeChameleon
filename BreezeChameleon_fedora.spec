Name:           breezechameleon
Version:        999.git
Release:        1%{?dist}
Summary:        BreezeChameleon KDE decoration

License:        GPL-3.0-or-later
URL:            https://github.com/vickoc911/BreezeChameleon
Source0:        %{url}/archive/refs/heads/master.tar.gz#/%{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  extra-cmake-modules
BuildRequires:  kf6-rpm-macros

# Qt6
BuildRequires:  qt6-qtbase-devel

# KDE Frameworks 6 (todos en -devel)
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-kguiaddons-devel
BuildRequires:  kf6-kconfigwidgets-devel
BuildRequires:  kf6-kcmutils-devel
BuildRequires:  kf6-kwindowsystem-devel
BuildRequires:  kf6-ki18n-devel

# KDecoration (fuera de KF6 core)
BuildRequires:  kdecoration-devel

# Otros
BuildRequires:  pkgconfig(xcb)
BuildRequires:  pkgconfig(fftw3)

# Runtime
Requires:       kf6-kcmutils
Requires:       kdecoration

%description
BreezeChameleon is a fork of KDE Breeze decoration with enhancements.

%prep
%autosetup -p1 -n BreezeChameleon-master

%build
%cmake \
  -DBUILD_TESTING=OFF \
  -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
%cmake_build

%install
%cmake_install

%files
%{_libdir}/qt6/plugins/org.kde.kdecoration3/breezechameleon.so
%{_libdir}/qt6/plugins/org.kde.kdecoration3.kcm/kcm_breezechameleon.so
%{_datadir}/applications/kcm_breezechameleon.desktop
%{_libdir}/libbreezechameleoncommon6.so.*

%changelog
* Thu Apr 16 2026 Victor Calles <vcalles@gmail.com> - 999.git-1
- Initial Fedora build
