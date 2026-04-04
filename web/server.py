import json
import os
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
OUTPUT = os.path.join(ROOT, 'program.tsb')

class TinyScriptHandler(BaseHTTPRequestHandler):
    def _serve_file(self, path, content_type='text/html'):
        try:
            with open(path, 'rb') as f:
                data = f.read()
            self.send_response(200)
            self.send_header('Content-Type', content_type)
            self.end_headers()
            self.wfile.write(data)
        except FileNotFoundError:
            self.send_response(404)
            self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == '/' or parsed.path == '/index.html':
            self._serve_file(os.path.join(os.path.dirname(__file__), 'index.html'))
        elif parsed.path == '/style.css':
            self._serve_file(os.path.join(os.path.dirname(__file__), 'style.css'), 'text/css')
        elif parsed.path == '/app.js':
            self._serve_file(os.path.join(os.path.dirname(__file__), 'app.js'), 'application/javascript')
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        if self.path != '/compile':
            self.send_response(404)
            self.end_headers()
            return
        length = int(self.headers.get('Content-Length', '0'))
        body = self.rfile.read(length)
        data = json.loads(body.decode('utf-8'))
        code = data.get('code', '')
        script_file = os.path.join(ROOT, 'web', 'scratch.ts')
        with open(script_file, 'w', encoding='utf-8') as f:
            f.write(code)
        compiler = os.path.join(ROOT, 'tinyscript.exe' if os.name == 'nt' else 'tinyscript')
        if not os.path.exists(compiler):
            self.send_response(500)
            self.end_headers()
            self.wfile.write(b'TinyScript compiler not built. Run build.bat or make first.')
            return
        result = os.popen(f'"{compiler}" "{script_file}" --run --emit-ir').read()
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain')
        self.end_headers()
        self.wfile.write(result.encode('utf-8'))

if __name__ == '__main__':
    server = HTTPServer(('localhost', 8000), TinyScriptHandler)
    print('TinyScript UI available at http://localhost:8000')
    server.serve_forever()
