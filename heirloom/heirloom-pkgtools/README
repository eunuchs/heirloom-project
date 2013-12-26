README file for the Heirloom Packaging Tools
============================================

The Heirloom Packaging Tools are Linux ports of the SVR4
application packaging tools, as released by Sun as part of
OpenSolaris.

Currently, there exists no Linux distribution which bases
its package management on these tools. Nevertheless, they
are useful for evaluation purposes. If run in interactive
mode, "pkgadd" will ask for confirmation before overwriting
existing files, so it is principally possible to use these
tools in addition to another packaging system.

The packaging tools depend on other components of the
Heirloom Project, namely, the Bourne shell, the toolchest,
and the development tools. You must install them on the
compilation system before. For a target system, only the
Bourne shell and the toolchest are required.

To build and install:

- Adjust the installation paths and compiler settings in the file
  "mk.config", which is in makefile syntax.

- Execute "make", followed by "make install". "make mrproper" will
  destroy all generated files.

Once the tools are installed, "make heirloom-pkgtools.pkg" will
create a SVR4 datastream format package containing them. You can
use it as a starting point for further experiments. For further
information, see the enclosed manual pages, or refer to an
introductory text for SVR4 packaging.

Some of the features recently introduced by Sun are partially
disabled in this variant; this applies to SQLite, Solaris zones,
signed packages, and web installation. Still, OpenSSL header
files are required while building, although the libraries are
not used for linking.

New releases of this project are announced on Freshmeat. If you want to
get notified by email on each release, use their subscription service at
<http://freshmeat.net/projects/pkgtools/>.

The project homepage is currently at <http://heirloom.sourceforge.net>.

Report bugs to

Gunnar Ritter
Freiburg i. Br.
Germany
<http://heirloom.sourceforge.net>
<gunnarr@acm.org>                                               2/28/07
