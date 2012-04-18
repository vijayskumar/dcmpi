#! /bin/sh

function setsocket() {
    (echo 1; echo 1; echo x) | dcmpiconfig
}

function setmpich() {
    (echo 1; echo 2; echo x) | dcmpiconfig
}

function testmany() {
    ./test-all.sh `mkfile akron`
    ./test-all.sh `mkfile akron newhope`
    ./test-all.sh `mkfile akron dc01`
    ./test-all.sh `mkfile akron dc01 dc02`;    
    ./test-all.sh `mkfile akron newhope dc01`;    
    ./test-all.sh `mkfile akron newhope dc01 dc02`;    
    ./test-all.sh `mkfile akron newhope dc01 mob02 dc02`;    
}

set -x
set -e
setsocket
testmany
setmpich
testmany
