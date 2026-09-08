import os
import re
import json
from google.cloud import texttospeech

OUTPUT_DIR = "line_audio_output"
USAGE_FILE = "tts_usage_tracker.json"

# Define free tier monthly limits based on the pricing guide[cite: 1]
MODEL_LIMITS = {
    "Standard": 4_000_000,  # 4 million characters[cite: 1]
    "Wavenet": 4_000_000,   # 4 million characters[cite: 1]
    "Neural2": 1_000_000,   # 1 million characters[cite: 1]
    "Polyglot": 1_000_000,  # 1 million characters[cite: 1]
    "Chirp": 1_000_000,     # 1 million characters[cite: 1]
    "Studio": 1_000_000     # 1 million characters[cite: 1]
}

def sanitize_filename(text, max_length=50):
    """Converts a text string into a safe filename with underscores."""
    clean_text = re.sub(r'[^a-zA-Z0-9\s-]', '', text)
    filename = clean_text.strip().replace(' ', '_')
    if len(filename) > max_length:
        filename = filename[:max_length].rstrip('_')
    return filename or "line_audio"

def get_model_family(voice_name):
    """Identifies the model family from the voice name string."""
    for family in MODEL_LIMITS.keys():
        if family.lower() in voice_name.lower():
            return family
    return "Neural2"  # Default fallback tier if unmatched

def check_and_update_usage(char_count, voice_name):
    """Tracks cumulative character usage per model family and blocks if limit is hit."""
    model_family = get_model_family(voice_name)
    max_limit = MODEL_LIMITS.get(model_family, 1_000_000)
    
    # Load current usage data or initialize
    if os.path.exists(USAGE_FILE):
        with open(USAGE_FILE, "r") as f:
            data = json.load(f)
    else:
        data = {}
    
    if model_family not in data:
        data[model_family] = {"characters_used": 0}
        
    current_total = data[model_family]["characters_used"]
    
    if current_total + char_count > max_limit:
        raise Exception(
            f"❌ [Allowance Error]: Monthly free tier limit reached for {model_family} voices! "
            f"Used {current_total:,} of {max_limit:,} allowed characters."
        )
    
    # Update usage
    data[model_family]["characters_used"] += char_count
    with open(USAGE_FILE, "w") as f:
        json.dump(data, f, indent=4)
    
    print(f"📊 [{model_family} Quota]: {data[model_family]['characters_used']:,} / {max_limit:,} characters used.")

def select_voice(client):
    """Fetches available English voices and presents an interactive menu."""
    print("Fetching available English voices from Google Cloud...")
    response = client.list_voices()
    
    english_voices = [
        voice for voice in response.voices 
        if any(lang.startswith("en-") for lang in voice.language_codes)
    ]
    
    if not english_voices:
        print("❌ No English voices found!")
        return None, None

    print("\n--- Available English Voices Menu ---")
    for idx, voice in enumerate(english_voices, 1):
        lang = voice.language_codes[0]
        gender = texttospeech.SsmlVoiceGender(voice.ssml_gender).name
        print(f"[{idx:2d}] {voice.name} ({lang} - {gender})")
    
    while True:
        try:
            choice = input(f"\nSelect a voice number (1-{len(english_voices)}): ").strip()
            index = int(choice) - 1
            if 0 <= index < len(english_voices):
                selected = english_voices[index]
                return selected.name, selected.language_codes[0]
            print("❌ Invalid selection. Please choose a number from the list.")
        except ValueError:
            print("❌ Please enter a valid number.")

def process_document(filename="script.txt"):
    if not os.path.exists(filename):
        print(f"❌ Error: '{filename}' not found in this directory.")
        return

    client = texttospeech.TextToSpeechClient()

    # Get voice selection via interactive menu
    voice_name, language_code = select_voice(client)
    if not voice_name:
        return

    print(f"\n🎙️ Selected Voice: {voice_name} ({language_code})")
    
    # Read the text document line by line
    with open(filename, "r", encoding="utf-8") as f:
        lines = f.readlines()

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    success_count = 0
    for idx, line in enumerate(lines, 1):
        clean_line = line.strip()
        
        # Skip empty lines
        if not clean_line:
            continue
            
        line_length = len(clean_line)
        base_name = sanitize_filename(clean_line)
        file_name = f"{idx:03d}_{base_name}.mp3"
        output_path = os.path.join(OUTPUT_DIR, file_name)
        
        print(f"\n[{idx}] Processing line: '{clean_line[:40]}...'")

        try:
            # Validate usage locally before firing the API call
            check_and_update_usage(line_length, voice_name)
            
            input_text = texttospeech.SynthesisInput(text=clean_line)
            voice_params = texttospeech.VoiceSelectionParams(
                language_code=language_code,
                name=voice_name
            )
            audio_config = texttospeech.AudioConfig(
                audio_encoding=texttospeech.AudioEncoding.MP3
            )

            response = client.synthesize_speech(
                input=input_text, voice=voice_params, audio_config=audio_config
            )
            
            with open(output_path, "wb") as out:
                out.write(response.audio_content)
                success_count += 1
                print(f" -> Saved: {file_name}")
                
        except Exception as e:
            print(f" ❌ Error: {e}")
            if "Allowance Error" in str(e):
                print("🛑 Halting batch script due to quota limit.")
                break

    print(f"\n🏁 Complete! Successfully generated {success_count} audio files in the '{OUTPUT_DIR}/' folder.")

if __name__ == "__main__":
    process_document()
