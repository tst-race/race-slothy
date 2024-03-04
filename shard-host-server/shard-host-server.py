#!/usr/bin/env python3

# Extended python3 http.server
#  - Adds ssl capability
#  - Adds POST and PUT support

# generate server.pem with the following command:
#    openssl req -new -x509 -keyout server.pem -out server.pem -days 365 -nodes

import os
import sys

from http import server
from functools import partial

class HTTPRequestHandler(server.SimpleHTTPRequestHandler):

    def do_POST(self):
        """Serve POST request"""
        filename = os.path.join(self.directory, os.path.basename(self.path))

        # Don't overwrite files
        if os.path.exists(filename):
            self.send_response(409, 'Conflict')
            self.end_headers()
            reply_body = '"%s" already exists\n' % filename
            self.wfile.write(reply_body.encode('utf-8'))
            return

        file_length = int(self.headers['Content-Length'])
        with open(filename, 'wb') as output_file:
            output_file.write(self.rfile.read(file_length))
        self.send_response(201, 'Created')
        self.end_headers()
        reply_body = 'Saved "%s"\n' % filename
        self.wfile.write(reply_body.encode('utf-8'))

    def do_PUT(self):
        """SErve PUT request"""
        self.do_POST()


def test(HandlerClass=server.BaseHTTPRequestHandler,
         ServerClass=server.ThreadingHTTPServer,
         protocol="HTTP/1.0", port=8000, bind="", cert=False):
    """Test the HTTP request handler class.

    This runs an HTTP server on port 8000 (or the port argument).

    """
    server_address = (bind, port)

    HandlerClass.protocol_version = protocol
    with ServerClass(server_address, HandlerClass) as httpd:
        sa = httpd.socket.getsockname()
        # Make ssl a thing
        if cert:
            from ssl import wrap_socket
            httpd.socket = wrap_socket(httpd.socket, certfile=cert, server_side=True)
            serve_message = "Serving HTTPS on {host} port {port} (https://{host}:{port}/) ..."
        else:
            serve_message = "Serving HTTP on {host} port {port} (http://{host}:{port}/) ..."
        
        print(serve_message.format(host=sa[0], port=sa[1]))
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nKeyboard interrupt received, exiting.")
            sys.exit(0)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--cgi', action='store_true',
                       help='Run as CGI Server')
    # parser.add_argument('--cert', '-c', default="/etc/ssl/certs/server.pem",
    #                     help='Specify cert path for HTTPS'
    #                     '[default: no /etc/ssl/certs/server.pem]')
    parser.add_argument('--cert', '-c', default=None,
                        help='Specify cert path for HTTPS'
                        '[default: no /etc/ssl/certs/server.pem]')
    parser.add_argument('--bind', '-b', default='0.0.0.0', metavar='ADDRESS',
                        help='Specify alternate bind address '
                             '[default: 0.0.0.0]')
    parser.add_argument('--directory', '-d', default="/root/serve-from-here",
                        help='Specify alternative directory '
                        '[default:/root/serve-from-here]')
    parser.add_argument('port', action='store',
                        default=443, type=int,
                        nargs='?',
                        help='Specify alternate port [default: 443]')
    args = parser.parse_args()
    if args.cgi:
        handler_class = server.CGIHTTPRequestHandler
    else:
        handler_class = partial(HTTPRequestHandler,
                                directory=args.directory)
    test(HandlerClass=handler_class, port=args.port, bind=args.bind, cert=args.cert)
