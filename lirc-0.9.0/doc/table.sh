#! /bin/sh

HWDB="${1:-/dev/null}"
HEURISTIC=`mktemp`

TOP_SRCDIR=${top_srcdir:-..}
SRCDIR=${srcdir:-.}
HTML_SOURCE=${SRCDIR}/html-source

SETUP_DATA="${TOP_SRCDIR}/setup.data"
. ${TOP_SRCDIR}/setup-functions.sh

write_heuristic="no"
cat ${TOP_SRCDIR}/configure.ac | while read REPLY; do
    if echo $REPLY|grep "START HARDWARE HEURISTIC" >/dev/null; then
	write_heuristic="yes"
	continue;
    fi
    if echo $REPLY|grep "END HARDWARE HEURISTIC" >/dev/null; then
	write_heuristic="no"
    fi
    if test "${write_heuristic}" = "yes"; then
	if echo $REPLY|grep "AC_DEFINE">/dev/null; then
	    continue;
	fi
	if echo $REPLY|grep "^echo">/dev/null; then
	    continue;
	fi
	if echo $REPLY|grep "exit 1">/dev/null; then
	    continue;
	fi
	echo $REPLY >>${HEURISTIC}
    fi
done

cat << HWDB_HEADER > "${HWDB}"
# LIRC - Hardware DataBase
#
# THIS IS A GENERATED FILE. DO NOT EDIT.
#
# This file lists all the remote controls supported by LIRC
# in a parseable form.
#
# The format is:
#
# [remote controls type]
# description;driver;lirc driver;HW_DEFAULT;lircd_conf;
#
#
HWDB_HEADER

cat ${HTML_SOURCE}/head.html

echo "<table border=\"1\">"
echo "<tr><th>Hardware</th><th>configure --with-driver option</th><th>Required LIRC kernel modules</th><th>lircd driver</th><th>default lircd and lircmd config files</th><th>Supported remotes</th></tr>"
grep ".*: \(\".*\"\)\|@" ${SETUP_DATA} | while read REPLY; do
    #echo $REPLY

    if echo $REPLY|grep ": @any" >/dev/null; then
	continue;
    fi
    
    if echo $REPLY|grep ": @" >/dev/null; then
	entry=`echo $REPLY|sed --expression="s/.*: \(@.*\)/\1/"`
	desc=`grep "${entry}:" ${SETUP_DATA}|sed --expression="s/.*\"\(.*\)\".*/\1/"`
	echo "" >> "${HWDB}"
	echo "[$desc]" >> "${HWDB}"
	echo "<tr><th colspan=\"6\"><a name=\"${entry}\">${desc}</a></th></tr>"
	continue;
    fi
    
    desc=`echo $REPLY|sed --expression="s/.*\"\(.*\)\".*/\1/"`
    driver=`echo $REPLY|sed --expression="s/\(.*\):.*/\1/"`
    
    if test "$driver" = "any" -o "$driver" = "none"; then
	continue;
    fi

    if echo $driver|grep @ >/dev/null; then
	echo "<tr><th colspan=\"6\"><a href=\"#${driver}\">${desc}</a></th></tr>"
	true;
    else
	. ${HEURISTIC}
	if ! echo "${lirc_driver}"|grep lirc_dev>/dev/null; then
	    lirc_driver="none"
	fi
	echo -n "<tr><td>${desc}</td><td align=\"center\">"
	
	if test -f ${HTML_SOURCE}/${driver}.html; then
	    driver_doc=${driver}
	elif test -f `echo ${HTML_SOURCE}/${driver}.html|sed --expression="s/_/-/g"`; then
	    driver_doc=`echo $driver|sed --expression="s/_/-/g"`
	else
	    driver_doc=""
	fi

	if test "$driver_doc" != ""; then
	    echo -n "<A HREF=\"${driver_doc}.html\">${driver}</A>"
	else
	    echo -n "${driver}"
	fi
	remote=$(query_setup_data 1 2 remote "${driver}")
	case "$remote" in
	none)
		remote="bundled"
		;;
	depends)
		remote="usually only bundled"
		;;
	special-config)
		remote="any, but config file receiver specific, no transmit capability"
		;;
	rc5)
		remote="RC-5 protocol only"
		;;
	esac
	echo "</td><td align=\"center\">${lirc_driver}</td><td align=\"center\">${HW_DEFAULT#???}</td><td>${lircd_conf}<br>${lircmd_conf}</td><td>${remote}</td></tr>"
	echo "${desc};${driver};${lirc_driver};${HW_DEFAULT};${lircd_conf};" >> "${HWDB}"
    fi
done
echo "</table>"

cat ${HTML_SOURCE}/foot.html
rm ${HEURISTIC}
