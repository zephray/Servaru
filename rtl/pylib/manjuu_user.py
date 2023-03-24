#
# Manjuu
# Copyright 2023 Wenting Zhang
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# Optional library import
from manjuu_tilelink import *
from manjuu_base import prefix

ras_req_t = [
    ["i", "left_edge", 13],
    ["i", "right_edge", 13],
    ["i", "upper_edge", 13],
    ["i", "lower_edge", 13],
    ["i", "edge0", 28],
    ["i", "edge1", 28],
    ["i", "edge2", 28],
    ["i", "step0x", 13],
    ["i", "step0y", 13],
    ["i", "step1x", 13],
    ["i", "step1y", 13],
    ["i", "step2x", 13],
    ["i", "step2y", 13]
]

ras_resp_t = [
    ["i", "x", 13],
    ["i", "y", 13]
]

rop_csr_t = [
    ["i", "fb_base", 32],
    ["i", "fb_width", 13],
    ["i", "fb_format", 2],
    ["i", "rgb_blend_mode", 3],
    ["i", "alpha_blend_mode", 3],
    ["i", "srgb", 9],
    ["i", "drgb", 9],
    ["i", "sa", 9],
    ["i", "da", 9]
]

setup_csr_t = [
    ["i", "x0", 13],
    ["i", "y0", 13],
    ["i", "x1", 13],
    ["i", "y1", 13],
    ["i", "x2", 13],
    ["i", "y2", 13],
    ["i", "trigger_valid"], 
    ["o", "trigger_ready"]
]

csr_t = prefix("rop", rop_csr_t) + prefix("setup", setup_csr_t)

define("FMT_Y8", "2'd0")
define("FMT_RGB16", "2'd1")
define("FMT_ARGB32", "2'd2") # Integer 8 bpc
define("FMT_ARGB128", "2'd3") # Float 32 bpc, not supported

define("BLEND_NOP", "3'd0")
define("BLEND_ADD", "3'd1")
define("BLEND_SUBTRACT", "3'd2")
define("BLEND_REVERSE_SBUTRACT", "3'd3")
define("BLEND_MIN", "3'd4")
define("BLEND_MAX", "3'd5")
