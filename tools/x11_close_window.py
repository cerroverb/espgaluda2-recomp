#!/usr/bin/env python3
# Original code (WORKLOG.md "Clean exit"): asks the X11 windows of a process to close (WM_DELETE_WINDOW), as a window
# manager's close button does. gamescope's window manager has no _NET_CLIENT_LIST, so wmctrl can't do it.
#   DISPLAY=:0 tools/x11_close_window.py <pid>
import ctypes, ctypes.util, sys

x = ctypes.CDLL(ctypes.util.find_library("X11"))
x.XOpenDisplay.restype = ctypes.c_void_p
x.XOpenDisplay.argtypes = [ctypes.c_char_p]
x.XDefaultRootWindow.restype = ctypes.c_ulong
x.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
x.XInternAtom.restype = ctypes.c_ulong
x.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
x.XQueryTree.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_ulong),
                         ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)), ctypes.POINTER(ctypes.c_uint)]
x.XGetWindowProperty.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_ulong, ctypes.c_long, ctypes.c_long,
                                 ctypes.c_int, ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong),
                                 ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_ulong),
                                 ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.POINTER(ctypes.c_ubyte))]
x.XSendEvent.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_long, ctypes.c_void_p]
x.XFlush.argtypes = [ctypes.c_void_p]


class XClientMessageEvent(ctypes.Structure):
    _fields_ = [("type", ctypes.c_int), ("serial", ctypes.c_ulong), ("send_event", ctypes.c_int),
                ("display", ctypes.c_void_p), ("window", ctypes.c_ulong), ("message_type", ctypes.c_ulong),
                ("format", ctypes.c_int), ("l", ctypes.c_long * 5), ("pad", ctypes.c_long * 12)]


pid = int(sys.argv[1])
d = x.XOpenDisplay(None)
if not d:
    sys.exit("cannot open display")
pid_atom = x.XInternAtom(d, b"_NET_WM_PID", 0)
protocols = x.XInternAtom(d, b"WM_PROTOCOLS", 0)
delete = x.XInternAtom(d, b"WM_DELETE_WINDOW", 0)


def window_pid(w):
    t, f, n, after = ctypes.c_ulong(), ctypes.c_int(), ctypes.c_ulong(), ctypes.c_ulong()
    data = ctypes.POINTER(ctypes.c_ubyte)()
    if x.XGetWindowProperty(d, w, pid_atom, 0, 1, 0, 6, ctypes.byref(t), ctypes.byref(f), ctypes.byref(n),
                            ctypes.byref(after), ctypes.byref(data)) != 0 or n.value == 0:
        return None
    return ctypes.cast(data, ctypes.POINTER(ctypes.c_ulong))[0]


def walk(w):
    root, parent = ctypes.c_ulong(), ctypes.c_ulong()
    kids, count = ctypes.POINTER(ctypes.c_ulong)(), ctypes.c_uint()
    if not x.XQueryTree(d, w, ctypes.byref(root), ctypes.byref(parent), ctypes.byref(kids), ctypes.byref(count)):
        return
    for i in range(count.value):
        yield kids[i]
        yield from walk(kids[i])


sent = 0
for w in walk(x.XDefaultRootWindow(d)):
    if window_pid(w) == pid:
        e = XClientMessageEvent(type=33, window=w, message_type=protocols, format=32)  # ClientMessage
        e.l[0], e.l[1] = delete, 0
        x.XSendEvent(d, w, 0, 0, ctypes.byref(e))
        sent += 1
x.XFlush(d)
print(f"WM_DELETE_WINDOW sent to {sent} window(s) of {pid}")
sys.exit(0 if sent else 1)
