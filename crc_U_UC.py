#!/usr/bin/python

import sys


if len(sys.argv) != 8:
    print ("[Usage :] python crc.py UID")
    print ("\twith UID equivalent to 7 bytes")
else :
    u0 = (int(sys.argv[1], 16))
    u1 = (int(sys.argv[2], 16))
    u2 = (int(sys.argv[3], 16))
    u3 = (int(sys.argv[4], 16))
    u4 = (int(sys.argv[5], 16))
    u5 = (int(sys.argv[6], 16))
    u6 = (int(sys.argv[7], 16))



    CRC0 = 0x88 ^ u0 ^ u1 ^ u2
    CRC1 = u3 ^ u4 ^ u5 ^ u6


    print("CRC0 = " + hex(CRC0))
    print("CRC1 = " + hex(CRC1))