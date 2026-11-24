#!/usr/bin/env python3
"""Send touch commands to the lavender iOS kernel over USB NCM.

The kernel (patch 0221) listens on 192.168.7.2:5141 for ASCII datagrams:

  SWEET tap X Y
  SWEET down X Y | move X Y | up X Y
  SWEET swipe X1 Y1 X2 Y2 [MS]
  SWEET key N D          (0 power, 1 vol+, 2 vol-, 3 home)

Coordinates are panel-native: x 0..1079, y 0..2399.

Usage:
  python tools/touch-remote.py tap 200 2200
  python tools/touch-remote.py swipe 540 2300 540 700 400
  python tools/touch-remote.py key 3 1
"""
import socket
import sys

HOST = "192.168.7.2"
PORT = 5141


def send(text):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(("SWEET " + text).encode(), (HOST, PORT))
    sock.close()
    print("->", text)


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 2
    cmd = args[0]
    if cmd == "tap" and len(args) == 3:
        send("tap %s %s" % (args[1], args[2]))
    elif cmd in ("down", "move", "up") and len(args) == 3:
        send("%s %s %s" % (cmd, args[1], args[2]))
    elif cmd == "swipe" and len(args) in (5, 6):
        send("swipe " + " ".join(args[1:]))
    elif cmd == "key" and len(args) == 3:
        send("key %s %s" % (args[1], args[2]))
    else:
        print(__doc__)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
