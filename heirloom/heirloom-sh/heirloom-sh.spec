#
# Sccsid @(#)heirloom-sh.spec	1.5 (gritter) 7/12/05
#
Summary: The Heirloom Bourne Shell.
Name: heirloom-sh
Version: 000000
Release: 1
License: Other
Source: %{name}-%{version}.tar.bz2
Group: System Environment/Base
Vendor: Gunnar Ritter <gunnarr@acm.org>
URL: <http://heirloom.sourceforge.net>
BuildRoot: %{_tmppath}/%{name}-root

%define	usr		/usr
%define	defbin		%{usr}/5bin
%define	sv3bin		%{defbin}

%define	mandir		%{usr}/share/man/5man

%define	lns		ln

%define	xcc		gcc
%define	cflags		'-O -fomit-frame-pointer'
%define	cppflags	'-D__NO_STRING_INLINES -D_GNU_SOURCE'

#
# Combine the settings defined above.
#
%define	makeflags	ROOT=%{buildroot} DEFBIN=%{defbin} SV3BIN=%{sv3bin} MANDIR=%{mandir} CC=%{xcc} CFLAGS=%{cflags} CPPFLAGS=%{cppflags} LNS=%{lns} UCBINST=install

%description
The Heirloom Bourne Shell is a portable variant of the traditional Unix
shell. It is especially suitable for testing the portability of shell
scripts and for processing legacy scripts. The Bourne shell does not
provide as many features as newer Unix shells, but it does provide a
stable shell language. With this in mind, it is also suitable for
general script processing and interactive use. This variant of the
Bourne shell has been derived from OpenSolaris code and thus provides
the SVR4/SVID3 level of the shell.

%prep
rm -rf %{buildroot}
%setup

%build
make %{makeflags}

%install
make %{makeflags} install

%clean
cd .. && rm -rf %{_builddir}/%{name}-%{version}
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc CALDERA.LICENSE CHANGES OPENSOLARIS.LICENSE README
%{sv3bin}/*
%{mandir}/man1/*
