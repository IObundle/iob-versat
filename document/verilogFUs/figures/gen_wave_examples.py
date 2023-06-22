#!/usr/bin/env python3

import wavedrom

svg = wavedrom.render(
    """
{ "signal": [
 { "name": "clk",   "wave": "P...............", "period": 1  },
 { "name": "rst",   "wave": "010............."},
 { "name": "run",   "wave": "x..10..........."},
 { "name": "in0",   "wave": "x...555555555555", "data": "I0 I1 I2 I3 I4 I5 I6 I7 I8 I9 I10 I11 I12" },
 { "name": "out0",  "wave": "x........5555555", "data": "O0 O1 O2 O3 O4 O5 O6" }
]}"""
)
svg.saveas("simple.svg")

svg = wavedrom.render(
    """
{ "signal": [
 { "name": "clk",   "wave": "P...............", "period": 1  },
 { "name": "rst",   "wave": "010............."},
 { "name": "run",   "wave": "x..10..........."},
 { "name": "delay0", "wave": "x..5............", "data": "2" },
 { "name": "in0",   "wave": "x.....5555555555", "data": "I0 I1 I2 I3 I4 I5 I6 I7 I8 I9 I10" },
 { "name": "out0",  "wave": "x..........55555", "data": "O0 O1 O2 O3 O4" }
]}"""
)
svg.saveas("delay.svg")
