#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os 
import SocketServer
import time



global last_notify_time;

class MyUDPHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        global last_notify_time;
        data = self.request[0].strip();
        count = int(data);
        print "{} wrote:".format(self.client_address[0]);
        print data;
        if (count > 0 and time.time() - last_notify_time >= 10):
            last_notify_time = time.time();
            os.popen("/usr/bin/notify-send -i /usr/share/notify-osd/icons/hicolor/scalable/status/notification-message-im.svg '当前 RTX 未读条数: %d'  "%count,"r");

if __name__ == "__main__":
    HOST, PORT = "10.1.6.210", 3000

    server = SocketServer.UDPServer((HOST, PORT), MyUDPHandler);
    last_notify_time = time.time();
    server.serve_forever()
