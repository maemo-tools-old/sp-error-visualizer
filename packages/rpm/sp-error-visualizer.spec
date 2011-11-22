Name: sp-error-visualizer
Version: 0.1.0
Release: 1%{?dist}
Summary: Displays errors (or anything from stdin) as banners
Group: Development/Tools
License: GPLv2+
URL: http://www.gitorious.org/+maemo-tools-developers/maemo-tools/sp-error-visualizer
Source: %{name}_%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-build
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
%defattr(-,root,root,-)
%{_bindir}/sp-error-visualizer
%{_mandir}/man1/sp-error-visualizer.1.gz
%{_datadir}/%{name}/data/*
%doc COPYING 


%changelog
* Tue May 10 2011 Eero Tamminen <eero.tamminen@nokia.com> 0.1.0
  * Remove debugging printf - 

* Wed Apr 27 2011 Eero Tamminen <eero.tamminen@nokia.com> 0.0.9
  *  - sp-error-visualizer uses /etc/init.d/ & libosso which
    are deprecated in Harmattan

* Tue Jul 29 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.0.8-1
  * Common syslog error matching data has been updated. 

* Mon Jan 28 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.0.7-1
  * Add force-reload action to the sp-error-visualizer initscript to be
    more LSB-compliant.

* Fri Nov 16 2007 Eero Tamminen <eero.tamminen@nokia.com> 0.0.6-1
  * Minor fixes to the manual page. 

* Wed Sep 19 2007 Eero Tamminen <eero.tamminen@nokia.com> 0.0.5-1
  * Fixed incorrect year in copyright information
  * Minor documentation cleanup
  * 

* Mon Sep 10 2007 Eero Tamminen <eero.tamminen@nokia.com> 0.0.4-1
  * Check for alternative syslog locations to support different
    configurations
  * Added inotify-based log rotation monitoring support to work around
    Busybox tail limitations
  * 

* Wed Aug 29 2007 Eero Tamminen <eero.tamminen@nokia.com> 0.0.3-1
  * Moved the sp-error-visualizer startup to occur after both desktop
    and dbus, as it needs both in order to work properly.
  * Fixed various initscript issues, including missing licence information
  * Minor code cleanup
  * 

* Wed Aug 08 2007 Eero Tamminen <eero.tamminen@nokia.com> 0.0.2-1
  * Syslog socket support has been added to make this useful for
    environments without installed syslog daemon.
  * Licencing information/documentation fixes.
  * Minor other changes (unified indentation, more generic argument
    parsing etc)
  * 
  * 
  * Cleaned up/increased robustness of init scripts by starting to use
    start-stop-daemon

* Wed May 23 2007 Eero Tamminen <eero.tamminen@nokia.com> 0.0.1-1
  * Initial Release.
