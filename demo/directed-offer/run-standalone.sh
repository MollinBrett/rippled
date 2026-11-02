#!/usr/bin/env bash
# Starts a fresh standalone xrpld (genesis ledger, no peers). The ledger only
# advances when you call ledger_accept.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
xrpld="${XRPLD:-$here/../../.build/xrpld}"
"$here/make-config.sh"
rm -rf "$here/data/nudb" "$here/data/"*.db
exec "$xrpld" --conf "$here/xrpld.cfg" --standalone --start
