#! /bin/sh
rm -rf Makefile aclocal.m4 config.guess config.h.in* config.sub configure depcomp install-sh install.sh ltmain.sh missing autom4te.cache
find  -name Makefile.in -exec rm {} \;
autoreconf -i -f

#depcom is not added to DIST_COMMON by the previous run of automake, strange
automake

TMPFILE=$(mktemp)

cat >$TMPFILE <<EOF
#! /bin/sh      
if test "\$#" = "0"; then
  if ! ./setup.sh; then
    echo "Please read the documentation!!!"
    exit 1
  fi
  trap - EXIT
  exit 0
fi

EOF
cat configure >>$TMPFILE
mv $TMPFILE configure
chmod "u=rwx,g=rx,o=rx" configure

echo "Creating setup-driver.sh ..."
./data2setup.sh > setup-driver.sh
(cd contrib/hal/ && ./gen-hal-fdi.pl)

