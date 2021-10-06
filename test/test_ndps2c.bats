#!/usr/bin/env bats
# ------------------------------------------------------------------------ #
# Copyright 2021 SPTK Working Group                                        #
#                                                                          #
# Licensed under the Apache License, Version 2.0 (the "License");          #
# you may not use this file except in compliance with the License.         #
# You may obtain a copy of the License at                                  #
#                                                                          #
#     http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                          #
# Unless required by applicable law or agreed to in writing, software      #
# distributed under the License is distributed on an "AS IS" BASIS,        #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
# See the License for the specific language governing permissions and      #
# limitations under the License.                                           #
# ------------------------------------------------------------------------ #

sptk3=tools/sptk/bin
sptk4=bin

setup() {
    mkdir -p tmp
}

teardown() {
    rm -rf tmp
}

@test "ndps2c: compatibility" {
    $sptk3/nrand -l 10 | $sptk3/ndps2c -l 8 -m 3 > tmp/1
    $sptk3/nrand -l 10 | $sptk4/ndps2c -l 8 -m 3 > tmp/2
    run $sptk4/aeq tmp/1 tmp/2
    [ "$status" -eq 0 ]
}

@test "ndps2c: reversibility" {
    $sptk3/nrand -l 10 | $sptk3/c2ndps -m 4 -l 8 > tmp/1
    $sptk4/ndps2c -l 8 -m 4 tmp/1 | $sptk4/c2ndps -m 4 -l 8 > tmp/2
    run $sptk4/aeq tmp/1 tmp/2
    [ "$status" -eq 0 ]
}

@test "ndps2c: valgrind" {
    $sptk3/nrand -l 10 > tmp/1
    run valgrind $sptk4/ndps2c -l 8 -m 4 tmp/1
    [ "$(echo "${lines[-1]}" | sed -r 's/.*SUMMARY: ([0-9]*) .*/\1/')" -eq 0 ]
}
