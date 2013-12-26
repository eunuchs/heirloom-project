#! /bin/sh
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright (c) 1988 AT&T
# All Rights Reserved
# Copyright 2003 Sun Microsystems, Inc. All rights reserved.
# Use is subject to license terms.
#
# from sccsdiff.sh 1.15 06/12/12
#
#ident	"from sccsdiff.sh"
#ident	"from sccs:cmd/sccsdiff.sh"
#
# Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
#
# Sccsid @(#)sccsdiff.sh	1.4 (gritter) 1/3/07
#
#	DESCRIPTION:
#		Execute diff(1) on two versions of a set of
#		SCCS files and optionally pipe through pr(1).
#		Optionally specify diff segmentation size.
# if running under control of the NSE (-q option given), will look for
# get in the directtory from which it was run (if -q, $0 will be full pathname)
#
PATH=$PATH:/usr/ccs/bin

trap "rm -f /tmp/get[abc]$$;exit 1" 1 2 3 15
umask 077

if [ $# -lt 3 ]
then
	echo "Usage: sccsdiff -r<sid1> -r<sid2> [-p] [-q] sccsfile ..." 1>&2
	exit 1
fi

nseflag=
rflag=
addflags=
get=get
for i in $@
do
	if [ "$addflags" = "yes" ]
	then
		flags="$flags $i"
		addflags=
		continue
	fi
	case $i in

	-*)
		case $i in

		-r*)
			if [ ! "$sid1" ]
			then
				sid1=`echo $i | sed -e 's/^-r//'`
				if [ "$sid1" = "" ]
				then
					rflag=yes
				fi	
			elif [ ! "$sid2" ]
			then
				sid2=`echo $i | sed -e 's/^-r//'`
				if [ "$sid2" = "" ]
				then 
					rflag=yes
				fi	
			fi
			;;
		-s*)
			num=`echo $i | sed -e 's/^-s//'`
			;;
		-p*)
			pipe=yes
			;;
		-q*)
			nseflag=-q
			get=`dirname $0`/get
			;;
		-[cefnhbwituaB]*)
			flags="$flags $i"
			;;
		-P)
			flags="$flags -p"
			;;
		-D*)
			Dopt="-D"
			Darg=`echo $i | sed -e 's/^-D//'`
			if [ "$Darg" = "" ]
			then
				flags="$flags $i"
				addflags=yes
			else
				flags="$flags $Dopt $Darg"
			fi
			;;
		-C)
			flags="$flags $i"
			addflags=yes
			;;
		-U*)
			if [ `echo $i | wc -c` = 3 ]
			then
				flags="$flags $i"
				addflags=yes
			else
				opt=`echo $i | sed -e 's/^-U/-U /'`
				flags="$flags $opt"
			fi
			;;
		*)
			echo "$0: unknown argument: $i" 1>&2
			exit 1
			;;
		esac
		;;
	*s.*)
		files="$files $i"
		;;
	[1-9]*)
		if [ "$rflag" != "" ]
		then
			if [ ! "$sid1" ]
			then
				sid1=$i
			elif [ ! "$sid2" ]
			then
				sid2=$i	
			fi
		fi
		rflag=	
		;;	
	*)
		echo "$0: $i not an SCCS file" 1>&2
		;;
	esac
done

if [ "$files" = "" ]
then
	echo "$0: missing file arg (cm3)" 1>&2
	exit 1
fi

error=0
for i in $files
do
	rm -f /tmp/get[abc]$$
	# Good place to check if tmp-files are not deleted
	# if [ -f /tmp/geta$$ ] ...
	if $get $nseflag -s -p -k -r$sid1 $i > /tmp/geta$$
	then
		if $get $nseflag -s -p -k -r$sid2 $i > /tmp/getb$$
		then
			diff $flags /tmp/geta$$ /tmp/getb$$ > /tmp/getc$$
		else
			error=1
			continue	# error: cannot get release $sid2 
		fi
	else
		error=1
		continue	# error: cannot get release $sid1
	fi

	if [ ! -s /tmp/getc$$ ]
	then
		if [ -f /tmp/getc$$ ]
		then
			echo "$i: No differences" > /tmp/getc$$
		else
			error=1
			continue	# error: cannot get differences
		fi
	fi
	if [ "$pipe" ]
	then
		pr -h "$i: $sid1 vs. $sid2" /tmp/getc$$
	else
		file=`basename $i | sed -e 's/^s\.//'`
		echo
		echo "------- $file -------"
		cat /tmp/getc$$
	fi
done
rm -f /tmp/get[abc]$$
exit $error
