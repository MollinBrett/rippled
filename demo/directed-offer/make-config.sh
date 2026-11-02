#!/usr/bin/env bash
# Writes xrpld.cfg for a standalone node whose genesis ledger enables every
# supported amendment plus DirectedOffer (listed in amendments.txt).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
data="$here/data"
mkdir -p "$data"
{
    cat <<CFG
[server]
port_rpc_admin_local
port_ws_admin_local

[port_rpc_admin_local]
port = 5005
ip = 127.0.0.1
admin = 127.0.0.1
protocol = http

[port_ws_admin_local]
port = 6006
ip = 127.0.0.1
admin = 127.0.0.1
protocol = ws
send_queue_limit = 500

[node_size]
tiny

[node_db]
type = NuDB
path = $data/nudb
online_delete = 256
advisory_delete = 0

[database_path]
$data

[debug_logfile]
$data/debug.log

[ssl_verify]
0

[amendments]
CFG
    cat "$here/amendments.txt"
} >"$here/xrpld.cfg"
echo "wrote $here/xrpld.cfg"
