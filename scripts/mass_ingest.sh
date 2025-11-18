#!/usr/bin/env bash
set -euo pipefail
COUNT=${1:-100000}
FILE=${2:-data/fleet.csv}
mkdir -p "$(dirname "$FILE")"
./car_rental --backend=memory <<EOF_CLI
generate $COUNT $FILE
ingest $FILE 5000
save
exit
EOF_CLI
