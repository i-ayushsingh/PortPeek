import http.server
import socketserver
import threading
import sys

class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True

    def handle_error(self, request, client_address):
        # Gracefully silence client disconnects/aborts from fast probes
        pass

class ViteHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        body = b"<!DOCTYPE html><html><head><title>Vite + React Dashboard</title></head><body style='background:#111;color:#fff;font-family:sans-serif;padding:30px;'><h1>Vite + React App</h1><p>Running on port 5173</p></body></html>"
        try:
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        except Exception:
            pass

    def log_message(self, format, *args):
        pass

class ApiHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        body = b"<!DOCTYPE html><html><head><title>FastAPI Microservice Swagger UI</title></head><body style='background:#0f172a;color:#f8fafc;font-family:sans-serif;padding:30px;'><h1>FastAPI Microservice</h1><p>Swagger docs at /docs</p></body></html>"
        try:
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        except Exception:
            pass

    def log_message(self, format, *args):
        pass

class DocsHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory='c:/Projects/PortPeak/docs', **kwargs)

    def log_message(self, format, *args):
        pass

def run_server(port, handler_class, name):
    try:
        server = ThreadedHTTPServer(('0.0.0.0', port), handler_class)
        print(f"[{name}] Listening on 0.0.0.0:{port}")
        server.serve_forever()
    except Exception as e:
        print(f"[{name}] Error: {e}")

if __name__ == "__main__":
    t1 = threading.Thread(target=run_server, args=(3000, DocsHandler, "Docs Server"), daemon=True)
    t2 = threading.Thread(target=run_server, args=(5173, ViteHandler, "Vite Dev Server"), daemon=True)
    t3 = threading.Thread(target=run_server, args=(8000, ApiHandler, "FastAPI Service"), daemon=True)

    t1.start()
    t2.start()
    t3.start()

    print("===> All 3 dev servers active on 0.0.0.0: 3000 (Docs), 5173 (Vite), 8000 (FastAPI)")
    try:
        t1.join()
        t2.join()
        t3.join()
    except KeyboardInterrupt:
        print("Stopping servers...")
