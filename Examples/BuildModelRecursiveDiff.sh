if ! test -f "$1"; then
  echo "File \'$1\' does not exist"
fi
if ! test -f "$2"; then
  echo "File \'$2\' does not exist"
fi
BUILD="tr.*/td.*/td.*td bgcolor=..FF0000.*/td.*/td.*/td.*/td.*/td"
SIM="tr.*/td.*td bgcolor=..FF0000.*/td.*/td.*/td.*/td"
SEARCH=">Modelica[A-Za-z0-9._]*(<| [(])"
REV1=`grep -o "[(]r[0-9]*" "$1" | tr -d "("`
REV2=`grep -o "[(]r[0-9]*" "$2" | tr -d "("`
grep "$BUILD" $1 | egrep -o "$SEARCH" | tr -d "<> (" > "$1.build"
grep "$BUILD" $2 | egrep -o "$SEARCH" | tr -d "<> (" > "$2.build"
echo "Build diff (failures between $REV1 and $REV2)"
diff -u "$1.build" "$2.build" | grep ^[+-]

grep "$SIM" "$1" | egrep -o "$SEARCH" | tr -d "<> (" > "$1.sim"
grep "$SIM" "$2" | egrep -o "$SEARCH" | tr -d "<> (" > "$2.sim"
echo "Sim diff (failures between $REV1 and $REV2)"
diff -u "$1.sim" "$2.sim" | grep ^[+-]
