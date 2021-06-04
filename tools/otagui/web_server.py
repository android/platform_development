"""
A local HTTP server for Android OTA package generation.
Based on OTA_from_target_files.py 

Usage::
  python ./web_server.py [<port>]

API::
  GET /check : check the status of all jobs
  GET /check/<id> : check the status of the job with <id>
  GET /file : fetch the target file list
  GET /download/<id> : download the ota package with <id>
  POST /run/<id> : submit a job with <id>,
                 arguments set in a json uploaded together
  POST /file/<filename> : upload a target file 
  [TODO] POST /cancel/<id> : cancel a job with <id>

Other GET request will be redirected to the static request under 'dist' directory
"""

from http.server import BaseHTTPRequestHandler, SimpleHTTPRequestHandler, HTTPServer
from socketserver import ThreadingMixIn
from threading import Lock
from ota_interface import ProcessesManagement
import logging
import json
import pipes
import cgi
import subprocess
import os

LOCAL_ADDRESS = '0.0.0.0'

class CORSSimpleHTTPHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        try:
            origin_address, _ = cgi.parse_header(self.headers['Origin'])
            self.send_header('Access-Control-Allow-Credentials', 'true')
            self.send_header('Access-Control-Allow-Origin', origin_address)
        except TypeError:
            pass
        super().end_headers()


class RequestHandler(CORSSimpleHTTPHandler):
    def _set_response(self, code=200, type='text/html'):
        self.send_response(code)
        self.send_header('Content-type', type)
        self.end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Methods', 'GET, OPTIONS')
        self.send_header("Access-Control-Allow-Headers", "X-Requested-With")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        if self.path.startswith('/check'):
            if self.path == '/check' or self.path == '/check/':
                status = jobs.get_status()
                self._set_response(type='application/json')
                self.wfile.write(
                    status.encode()
                )
            else:
                id = self.path[7:]
                status = jobs.get_status_by_ID(id=id, details=True)
                self._set_response(type='application/json')
                self.wfile.write(
                    status.encode()
                )
            logging.info(
                "GET request:\nPath:%s\nHeaders:\n%s\nBody:\n%s\n",
                str(self.path), str(self.headers), status
            )
            return
        elif self.path.startswith('/file'):
            file_list = jobs.get_list(self.path[6:])
            self._set_response(type='application/json')
            self.wfile.write(
                file_list.encode()
            )
            logging.info(
                "GET request:\nPath:%s\nHeaders:\n%s\nBody:\n%s\n",
                str(self.path), str(self.headers), file_list
            )
            return
        elif self.path.startswith('/download'):
            self.path = self.path[10:]
            return CORSSimpleHTTPHandler.do_GET(self)
        else:
            self.path = '/dist' + self.path
            return CORSSimpleHTTPHandler.do_GET(self)

    def do_POST(self):
        if self.path.startswith('/run'):
            content_type, _ = cgi.parse_header(self.headers['content-type'])
            if content_type != 'application/json':
                self.send_response(400)
                self.end_headers()
                return
            content_length = int(self.headers['Content-Length'])
            post_data = json.loads(self.rfile.read(content_length))
            try:
                jobs.ota_generate(post_data, id=str(self.path[5:]))
                self._set_response(code=201)
                self.wfile.write(
                    "ota generator start running".encode('utf-8'))
            except SyntaxError:
                self.send_error(400)
            logging.info(
                "POST request:\nPath:%s\nHeaders:\n%s\nBody:\n%s\n",
                str(self.path), str(self.headers),
                json.dumps(post_data)
            )
        elif self.path.startswith('/file'):
            file_name = os.path.join('target', self.path[6:])
            file_length = int(self.headers['Content-Length'])
            with open(file_name, 'wb') as output_file:
                # Unwrap the uploaded file first
                # The wrapper has a boundary line at the top and bottom
                # and some file information in the beginning
                upper_boundary = self.rfile.readline()
                file_length -= len(upper_boundary) * 2 + 4
                file_length -= len(self.rfile.readline())
                file_length -= len(self.rfile.readline())
                file_length -= len(self.rfile.readline())
                output_file.write(self.rfile.read(file_length))
            self._set_response(code=201)
            self.wfile.write(
                "File received, saved into {}".format(
                    file_name).encode('utf-8')
            )
        else:
            self.send_error(400)


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    pass


def run_server(SeverClass=ThreadedHTTPServer, HandlerClass=RequestHandler, port=8000):
    logging.basicConfig(level=logging.DEBUG)
    server_address = (LOCAL_ADDRESS, port)
    server_instance = SeverClass(server_address, HandlerClass)
    try:
        logging.info(
            'Server is on, address:\n %s',
            'http://' + str(server_address[0]) + ':' + str(port))
        server_instance.serve_forever()
    except KeyboardInterrupt:
        pass
    server_instance.server_close()
    logging.info('Server has been turned off.')


if __name__ == '__main__':
    from sys import argv
    print(argv)
    jobs = ProcessesManagement()
    if len(argv) == 2:
        run_server(port=int(argv[1]))
    else:
        run_server()
