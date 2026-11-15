import http.server
import socketserver
import json
import os
import subprocess
import csv

PORT = 8000
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

class ShuttleRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=BASE_DIR, **kwargs)

    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        if self.path in ('/', '/index.html'):
            self.path = '/web/index.html'
        return super().do_GET()

    def do_POST(self):
        if self.path == '/api/add-stop':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            try:
                data = json.loads(body)
                name = str(data.get('name', '')).strip()
                lat = float(data.get('lat', 0.0))
                lon = float(data.get('lon', 0.0))
                deadline = int(data.get('deadline', 0))

                if not name or deadline <= 0:
                    raise ValueError("Invalid stop name or deadline minutes.")

                stops_path = os.path.join(BASE_DIR, 'data', 'stops.csv')

                # Check if stop already exists and calculate next sequential ID
                next_id = 0
                if os.path.exists(stops_path):
                    with open(stops_path, 'r', encoding='utf-8') as f:
                        reader = csv.reader(f)
                        next(reader, None)  # skip header
                        for row in reader:
                            if row:
                                existing_name = row[1].strip() if len(row) > 1 else ''
                                if existing_name.lower() == name.lower():
                                    raise ValueError(f"Stop '{name}' has already been added to the schedule.")
                                if row[0].strip().isdigit():
                                    next_id = max(next_id, int(row[0].strip()) + 1)

                # Append new stop
                with open(stops_path, 'a', encoding='utf-8', newline='') as f:
                    f.write(f"\n{next_id},{name},{lat:.4f},{lon:.4f},{deadline}")

                # Re-run optimizer to recalculate output/final_schedule.csv
                exe_name = 'shuttle_optimizer.exe' if os.name == 'nt' else 'shuttle_optimizer'
                exe_path = os.path.join(BASE_DIR, exe_name)
                if not os.path.exists(exe_path):
                    subprocess.run(
                        ['g++', '-Iinclude', '-Wall', '-std=c++14',
                         'src/main.cpp', 'src/data.cpp', 'src/scheduler.cpp',
                         'src/optimizer.cpp', 'src/evaluator.cpp', '-o', exe_name],
                        cwd=BASE_DIR, check=True
                    )

                cmd = [exe_path, os.path.join('data', 'stops.csv'), '2']
                result = subprocess.run(cmd, cwd=BASE_DIR, capture_output=True, text=True, check=True)

                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({'success': True, 'output': result.stdout}).encode('utf-8'))
            except Exception as e:
                self.send_response(400)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({'success': False, 'error': str(e)}).encode('utf-8'))
        else:
            self.send_error(404, "Endpoint not found")

if __name__ == '__main__':
    with socketserver.TCPServer(('', PORT), ShuttleRequestHandler) as httpd:
        print(f"ShuttleOpt server running at http://localhost:{PORT}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server.")
