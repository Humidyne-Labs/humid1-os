import os
import json
from google.cloud import texttospeech

# Define free tier monthly limits based on the pricing guide
MODEL_LIMITS = {
    "Standard": 4_000_000,  # 4 million characters[cite: 1]
    "Wavenet": 4_000_000,   # 4 million characters[cite: 1]
    "Neural2": 1_000_000,   # 1 million characters[cite: 1]
    "Polyglot": 1_000_000,  # 1 million characters[cite: 1]
    "Chirp": 1_000_000,     # 1 million characters[cite: 1]
    "Studio": 1_000_000     # 1 million characters[cite: 1]
}

USAGE_FILE = "tts_usage_tracker.json"
OUTPUT_DIR = "voice_previews_free"

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

# --- Main TTS Batch Execution ---
client = texttospeech.TextToSpeechClient()

# Read your text document
with open("document.txt", "r", encoding="utf-8") as file:
    text_content = file.read()

text_length = len(text_content)
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Fetch all available voices from the API
response = client.list_voices()

# Filter for English variants belonging to free-tier model families
free_model_keywords = list(MODEL_LIMITS.keys())
english_voices = [
    voice for voice in response.voices 
    if any(lang.startswith("en-") for lang in voice.language_codes)
    and any(keyword.lower() in voice.name.lower() for keyword in free_model_keywords)
]

print(f"Found {len(english_voices)} free-tier English voices. Processing previews...")

for voice in english_voices:
    voice_name = voice.name
    language_code = voice.language_codes[0]
    
    print(f"\nProcessing: {voice_name} ({language_code})")
    
    try:
        # Validate against the PDF-defined free tier limits locally before calling API
        check_and_update_usage(text_length, voice_name)
        
        # Proceed with API call if check passes
        input_text = texttospeech.SynthesisInput(text=text_content)
        
        voice_params = texttospeech.VoiceSelectionParams(
            language_code=language_code, 
            name=voice_name,
            ssml_gender=voice.ssml_gender
        )
        
        audio_config = texttospeech.AudioConfig(
            audio_encoding=texttospeech.AudioEncoding.MP3
        )
        
        response_audio = client.synthesize_speech(
            input=input_text, voice=voice_params, audio_config=audio_config
        )
        
        filename = os.path.join(OUTPUT_DIR, f"preview_{voice_name}.mp3")
        with open(filename, "wb") as out:
            out.write(response_audio.audio_content)
            print(f" -> Successfully saved to {filename}")
            
    except Exception as e:
        print(f" -> Skipped: {e}")

print(f"\nFinished batch processing! All available previews saved in the '{OUTPUT_DIR}/' directory.")
