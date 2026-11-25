#!/usr/bin/env python3

# Copyright (c) Arduino s.r.l. and/or its affiliated companies
# SPDX-License-Identifier: Apache-2.0

"""
Prepend erased padding to a signed image for a USB DFU upload.

In swap-using-offset mode, MCUboot expects the update one sector into the
secondary slot, while its USB DFU writes from the start of the slot: the
upload file starts with one erased (0xff) sector. The loader DFU also accepts
this padding.
"""

import sys
from pathlib import Path


def main():
    if len(sys.argv) != 5:
        sys.exit(f"usage: {sys.argv[0]} <input.bin> <output.bin> <padding> <slot_size>")

    image = Path(sys.argv[1]).read_bytes()
    padding = int(sys.argv[3], 0)
    slot_size = int(sys.argv[4], 0)
    if padding + len(image) > slot_size:
        sys.exit(f"{sys.argv[1]}: {padding + len(image)} bytes with padding, "
                 f"larger than the {slot_size} bytes secondary slot")

    Path(sys.argv[2]).write_bytes(b"\xff" * padding + image)


if __name__ == "__main__":
    main()
