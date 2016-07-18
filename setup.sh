#!/usr/bin/env bash

set -e

HERE=$(cd `dirname $0`; pwd)

DTC_REPO=git://git.kernel.org/pub/scm/utils/dtc/dtc.git

function fetch_dtc_repo {
    if [ -d "$HERE/dtc" ]
    then
        return 0
    fi

    cd $HERE
    git clone $DTC_REPO
}

fetch_dtc_repo
