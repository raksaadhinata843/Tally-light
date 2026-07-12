from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse
import asyncio
import websockets
import requests
import xml.etree.ElementTree as ET
import uvicorn
import threading

app = FastAPI()
status = {"active": 0, "preview": 0}
connected_clients = set()
last_state = bytes([0, 0])
vmix_api_url = "http://192.168.0.101:8088/api/"

@app.get("/", response_class=HTMLResponse)
async def read_root():
    return """
    <html>
        <head>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <style>
                body { font-family: sans-serif; text-align: center; background: #222; color: white; }
                .card { padding: 20px; margin: 10px; border-radius: 10px; background: #333; }
                .pgm { color: #ff4444; font-size: 2em; }
                .pvw { color: #44ff44; font-size: 2em; }
            </style>
        </head>
        <body>
            <h1>vMix Tally Status</h1>
            <div class="card">PGM: <span id="pgm" class="pgm">-</span></div>
            <div class="card">PVW: <span id="pvw" class="pvw">-</span></div>
            <script>
                setInterval(() => {
                    fetch('/status').then(r => r.json()).then(d => {
                        document.getElementById('pgm').innerText = d.active;
                        document.getElementById('pvw').innerText = d.preview;
                    });
                }, 500);
            </script>
            <input type="text" id="vmix_ip" placeholder="Masukkan IP vMix">
            <button onclick="saveIP()">Simpan IP</button>
            <script>
            function saveIP() {
                let ip = document.getElementById('vmix_ip').value;
                fetch('/update_config', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ip: "http://" + ip + ":8088/api/"})
                });
            }
            </script>
        </body>
    </html>
    """

@app.post("/update_config")
async def update_config(data: dict):
    global vmix_api_url
    vmix_api_url = data.get("ip")
    return {"status": "success", "new_ip": vmix_api_url}

@app.get("/status")
async def get_status():
    return status

async def ws_handler(websocket):
    connected_clients.add(websocket)
    try:
        await websocket.wait_closed()
    finally:
        connected_clients.remove(websocket)

async def vmix_monitor():
    global status
    async with websockets.connect("ws://localhost:8080") as ws:
        while True:
            try:
                res = requests.get(vmix_api_url, timeout=1)
                root = ET.fromstring(res.content)
                status["active"] = root.find('active').text
                status["preview"] = root.find('preview').text
            
            # Format ke JSON agar ESP mudah baca
                current_state = bytes([int(status["active"]), int(status["preview"])])
            
                if current_state != last_state:
                    print(f"State changed: PGM={status['active']}, PVW={status['preview']}")
                    await ws.send(current_state)
                    last_state = current_state
            except:
                pass
            await asyncio.sleep(0.1)

async def main():
    await asyncio.gather(
        websockets.serve(ws_handler, "0.0.0.0", 8080),
        vmix_monitor(),
        uvicorn.Server(uvicorn.Config(app, host="0.0.0.0", port=8000)).serve()
    )
if __name__ == "__main__":
    asyncio.run(main())