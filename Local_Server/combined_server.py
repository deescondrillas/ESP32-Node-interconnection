# combined_server.py
from flask import Flask, request, send_file, jsonify
import time, os

app = Flask(__name__)
TESTFILE = "testfile.bin"

@app.route('/testfile.bin', methods = ['GET'])
def serve_file():
    # send the file directly
    return send_file(TESTFILE, mimetype = 'application/octet-stream', as_attachment = True)

@app.route('/upload', methods=['POST'])
def upload():
    start = time.time()
    data = request.get_data()
    duration = time.time() - start
    size = len(data)
    mbps = (size * 8 / duration) / 1e6 if duration > 0 else 0
    print(f"[UPLOAD] Received {size} bytes in {duration:.4f}s -> {mbps:.2f} Mbps")
    return jsonify({"size": size, "duration_s": duration, "mbps": mbps}), 200

if __name__ == "__main__":
    if not os.path.exists(TESTFILE):
        with open(TESTFILE, "wb") as f:
            f.write(b"\0" * 1024 * 1024)   # create 1 MiB if missing
        print("Created testfile.bin (1 MiB)")
    app.run(host = '0.0.0.0', port = 8080)
