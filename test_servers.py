"""
PortPeek Multi-Server Test Playground
Run this script to simulate 3 real-world developer servers simultaneously:
1. Port 3000: Next.js / React App ("Customer Analytics Dashboard")
2. Port 8000: Python FastAPI Service ("FastAPI - Interactive Swagger UI")
3. Port 9090: Node.js Microservice ("Microservice Metrics API")

Press Ctrl+C or Enter to stop all test servers.
"""

import http.server
import socketserver
import threading
import time

SERVERS = [
    {
        "port": 3000,
        "title": "Customer Analytics Dashboard",
        "body": "<h1>Next.js Dashboard</h1><p>Running on localhost:3000</p>"
    },
    {
        "port": 8000,
        "title": "FastAPI - Interactive Swagger UI",
        "body": "<h1>FastAPI Documentation</h1><p>Running on localhost:8000</p>"
    },
    {
        "port": 9090,
        "title": "Microservice Metrics API",
        "body": "<h1>Metrics Engine</h1><p>Running on localhost:9090</p>"
    }
]

class CustomHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, page_title="Dev Server", page_body="OK", **kwargs):
        self.page_title = page_title
        self.page_body = page_body
        super().__init__(*args, **kwargs)

    def do_GET(self):
        html = f"""<!DOCTYPE html>
<html>
<head><title>{self.page_title}</title></head>
<body style="font-family: sans-serif; padding: 40px; background: #0f172a; color: #f8fafc;">
    {self.page_body}
    <p style="color: #94a3b8;">Port inspected by <b>PortPeek</b></p>
</body>
</html>""".encode('utf-8')

        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(html)))
        self.send_header("Server", "PortPeek-Test-Server/1.0")
        self.end_headers()
        self.wfile.write(html)

    def log_message(self, format, *args):
        pass # Keep console clean

def make_handler(title, body):
    return lambda *args, **kwargs: CustomHandler(*args, page_title=title, page_body=body, **kwargs)

def run_server(conf, server_list):
    handler = make_handler(conf["title"], conf["body"])
    try:
        httpd = socketserver.TCPServer(("127.0.0.1", conf["port"]), handler)
        server_list.append(httpd)
        print(f"  [+] Port {conf['port']}: Active -> \"{conf['title']}\"")
        httpd.serve_forever()
    except Exception as e:
        print(f"  [!] Port {conf['port']} failed to bind: {e}")

def main():
    print("=" * 60)
    print(" PortPeek Live Multi-Server Test Environment")
    print("=" * 60)
    print("\n[*] Spawning 3 simulated developer servers:\n")

    http_servers = []
    threads = []

    for conf in SERVERS:
        t = threading.Thread(target=run_server, args=(conf, http_servers), daemon=True)
        t.start()
        threads.append(t)

    time.sleep(0.5)

    print("\n" + "-" * 60)
    print(" >>> NOW CLICK THE PORTPEEK ICON IN YOUR TASKBAR! <<<")
    print("-" * 60)
    print("You should see:")
    print("  - Port 3000 -> Customer Analytics Dashboard")
    print("  - Port 8000 -> FastAPI - Interactive Swagger UI")
    print("  - Port 9090 -> Microservice Metrics API")
    print("\nClick any of them in PortPeek to launch it in your browser.")
    print("Press ENTER in this window when you want to stop all test servers.")
    print("-" * 60)

    try:
        input("\n[Press Enter to stop test servers] ")
    except KeyboardInterrupt:
        pass

    print("\n[*] Shutting down test servers...")
    for s in http_servers:
        try:
            s.shutdown()
        except:
            pass
    print("[OK] All test servers stopped.")

if __name__ == "__main__":
    main()
