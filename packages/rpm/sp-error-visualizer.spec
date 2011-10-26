Name: sp-error-visualizer
Version: 0.1.0
Release: 1%{?dist}
Summary: Displays errors (or anything from stdin) as banners
Group: Development/Tools
License: GPLv2+
URL: http://www.gitorious.org/+maemo-tools-developers/maemo-tools/sp-error-visualizer
Source: %{name}_%{version}.tar.gz
BuildRoot: {_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: dbus-1-glib-devel, pkg-config

%description
 This small tool displays any messages from stdin graphically
 (currently as banners). It's mainly intended to display errors
 from syslog to make them more visible to testers. If syslog
 is not present, it can also read directly the socket into which
 syslog clients write.

%prep
%setup -q -n %{name}

%build
make

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm -rf %{buildroot}/usr/share/meegotouch


%clean
rm -rf %{buildroot}

%files
%defattr(755,root,root,-)
%{_bindir}/sp-error-visualizer
%defattr(644,root,root,-)
%{_mandir}/man1/sp-error-visualizer.1.gz
%{_datadir}/%{name}/data/*
%doc COPYING 

