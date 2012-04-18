#! /bin/sh
set -e
ssh osumed 'cd dev/dcmpi/dcmpi-cvs && cvs up && scons' &
./test-all.sh hostfiles/h.akron
./test-all.sh hostfiles/h.akron-dc01
./test-all.sh hostfiles/h.akron-dc
./test-all.sh hostfiles/h.dc01-dc02
wait
./test-all.sh hostfiles/h.akron-dc01-osumed01
./test-all.sh hostfiles/h.akron-dc-osumed
