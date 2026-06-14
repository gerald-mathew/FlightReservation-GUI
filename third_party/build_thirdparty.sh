#!/usr/bin/env bash
# Builds the sqlite3 and argon2 static libraries used by reservation_core,
# using the SAME MinGW-Builds 13.1.0 (MSVCRT) toolchain as Qt 6.11.1.
#
# Why: linking the MSYS2 ucrt64 archives (UCRT runtime, newer GCC) into Qt's
# MSVCRT binary makes the app fail to load with STATUS_ENTRYPOINT_NOT_FOUND
# (0xC0000139) before main(). Compiling here with the kit GCC keeps the whole
# binary on one runtime/toolchain.
#
# Re-run this if the sources under third_party/ change or the Qt kit moves.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GCC="C:/Qt/Tools/mingw1310_64/bin/gcc.exe"
AR="C:/Qt/Tools/mingw1310_64/bin/ar.exe"
SQLITE_DIR="$HERE/sqlite-amalgamation-3450000"
ARGON2_DIR="$HERE/phc-winner-argon2-20190702"

mkdir -p "$HERE/lib" "$HERE/include" "$HERE/obj"
cp "$SQLITE_DIR/sqlite3.h" "$HERE/include/"
cp "$ARGON2_DIR/include/argon2.h" "$HERE/include/"

echo "Compiling sqlite3..."
"$GCC" -O2 -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_JSON1 \
    -c "$SQLITE_DIR/sqlite3.c" -o "$HERE/obj/sqlite3.o"
"$AR" rcs "$HERE/lib/libsqlite3.a" "$HERE/obj/sqlite3.o"

echo "Compiling argon2 (portable ref variant)..."
for f in argon2 core blake2/blake2b thread encoding ref; do
    "$GCC" -O2 -I"$ARGON2_DIR/include" -I"$ARGON2_DIR/src" \
        -c "$ARGON2_DIR/src/$f.c" -o "$HERE/obj/argon2_$(basename "$f").o"
done
"$AR" rcs "$HERE/lib/libargon2.a" \
    "$HERE/obj/argon2_argon2.o" "$HERE/obj/argon2_core.o" \
    "$HERE/obj/argon2_blake2b.o" "$HERE/obj/argon2_thread.o" \
    "$HERE/obj/argon2_encoding.o" "$HERE/obj/argon2_ref.o"

echo "Done: lib/libsqlite3.a, lib/libargon2.a"
