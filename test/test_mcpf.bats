#!/usr/bin/env bats
# ----------------------------------------------------------------- #
#             The Speech Signal Processing Toolkit (SPTK)           #
#             developed by SPTK Working Group                       #
#             http://sp-tk.sourceforge.net/                         #
# ----------------------------------------------------------------- #
#                                                                   #
#  Copyright (c) 1984-2007  Tokyo Institute of Technology           #
#                           Interdisciplinary Graduate School of    #
#                           Science and Engineering                 #
#                                                                   #
#                1996-2021  Nagoya Institute of Technology          #
#                           Department of Computer Science          #
#                                                                   #
# All rights reserved.                                              #
#                                                                   #
# Redistribution and use in source and binary forms, with or        #
# without modification, are permitted provided that the following   #
# conditions are met:                                               #
#                                                                   #
# - Redistributions of source code must retain the above copyright  #
#   notice, this list of conditions and the following disclaimer.   #
# - Redistributions in binary form must reproduce the above         #
#   copyright notice, this list of conditions and the following     #
#   disclaimer in the documentation and/or other materials provided #
#   with the distribution.                                          #
# - Neither the name of the SPTK working group nor the names of its #
#   contributors may be used to endorse or promote products derived #
#   from this software without specific prior written permission.   #
#                                                                   #
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            #
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       #
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          #
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          #
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS #
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          #
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   #
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     #
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON #
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   #
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    #
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           #
# POSSIBILITY OF SUCH DAMAGE.                                       #
# ----------------------------------------------------------------- #

sptk3=tools/sptk/bin
sptk4=bin

setup() {
   mkdir -p tmp
}

teardown() {
   rm -rf tmp
}

@test "mcpf: compatibility" {
   m=9
   M=39
   l=128
   s=2
   a=0.2
   b=0.1

   # From postfiltering_mcp on Training.pl.
   $sptk3/nrand -l 512 | \
      $sptk3/mcep -m $m -q 0 -j 5 > tmp/mc

   $sptk3/step -v 1 -l $s > tmp/w
   $sptk3/step -v $b -l $(($m-$s+1)) | \
      $sptk3/sopr -a 1 >> tmp/w

   $sptk3/freqt -m $m -M $M -a $a -A 0 tmp/mc | \
      $sptk3/c2acr -m $M -M 0 -l $l > tmp/r0

   $sptk3/vopr -m -n $m tmp/w < tmp/mc > tmp/mcw
   $sptk3/freqt -m $m -M $M -a $a -A 0 tmp/mcw | \
      $sptk3/c2acr -m $M -M 0 -l $l > tmp/r0w

   $sptk3/mc2b -m $m -a $a tmp/mcw | \
      $sptk3/bcp +d -n $m -s 0 -e 0 > tmp/b0w

   $sptk3/vopr -d < tmp/r0 tmp/r0w | \
      $sptk3/sopr -LN -d 2 | \
      $sptk3/vopr -a tmp/b0w > tmp/b0p

   $sptk3/mc2b -m $m -a $a tmp/mcw | \
      $sptk3/bcp +d -n $m -s 1 -e $m | \
      $sptk3/merge +d -n $(($m-1)) -s 0 -N 0 tmp/b0p | \
      $sptk3/b2mc -m $m -a $a > tmp/mcp

   $sptk4/mcpf -m $m -l $l -s $s -a $a -b $b tmp/mc > tmp/mcp2
   run $sptk4/aeq tmp/mcp tmp/mcp2
   [ "$status" -eq 0 ]
}

@test "mcpf: valgrind" {
   $sptk3/nrand -l 20 > tmp/1
   run valgrind $sptk4/mcpf -m 9 tmp/1
   [ $(echo "${lines[-1]}" | sed -r 's/.*SUMMARY: ([0-9]*) .*/\1/') -eq 0 ]
}
