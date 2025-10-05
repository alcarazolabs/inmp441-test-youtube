from flask import Flask, request, jsonify
import os
import base64

app = Flask(__name__)

# Folder to store uploaded and generated files
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_FOLDER = os.path.join(BASE_DIR, "uploads")
os.makedirs(UPLOAD_FOLDER, exist_ok=True)


@app.route("/upload", methods=["POST"])
def upload_audio():
    """Receive audio from client and save it"""
    mic_path = os.path.join(UPLOAD_FOLDER, "microphone.wav")

    # Remove old files if they exist
    #for f in [mic_path, response_wav, temp_mp3]:
    for f in [mic_path]:
        if os.path.exists(f):
            os.remove(f)
            print(f"[CLEANUP] Removed old file: {f}")

    # Save uploaded audio
    with open(mic_path, "wb") as f:
        f.write(request.data)
    print(f"[UPLOAD] File saved: {mic_path}")

    return "OK", 200

# Endpoint para recibir audios en base64 (opcional)
@app.route('/upload2', methods=['POST'])
def upload_audio2():
    try:
        data = request.get_json()
        filename = data.get("filename", "audio.wav")
        audio_base64 = data.get("data")

        if not audio_base64:
            return jsonify({"status": "error", "message": "No audio data received"}), 400

        # Decodificar Base64
        audio_bytes = base64.b64decode(audio_base64)

        # Guardar archivo
        filepath = os.path.join(UPLOAD_FOLDER, filename)
        with open(filepath, "wb") as f:
            f.write(audio_bytes)

        return jsonify({"status": "success", "message": f"File saved as {filepath}"})

    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)
