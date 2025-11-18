#!/usr/bin/env bash
set -euo pipefail
DATA_FILE=${1:-data/fleet.csv}
USERS=("C1001" "C1002" "C1003" "C1004" "C1005")
COMMANDS=()
# Rent cars that are currently available
for car in $(seq 1 80); do
  user=${USERS[$((RANDOM % ${#USERS[@]}))]}
  COMMANDS+=("rent car$car $user")
  if (( RANDOM % 5 == 0 )); then
    COMMANDS+=("return car$car")
  fi
  if (( RANDOM % 7 == 0 )); then
    COMMANDS+=("add auto$RANDOM Model$RANDOM excellent 4200")
  fi

done
# Attempt returns only for cars we rented
for car in $(seq 1 80); do
  if (( RANDOM % 3 == 0 )); then
    COMMANDS+=("return car$car")
  fi

done
COMMANDS+=("save" "exit")
./car_rental --backend=memory <<EOF_CLI
$(printf "%s\n" "${COMMANDS[@]}")
EOF_CLI
