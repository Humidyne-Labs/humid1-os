import os
import msvcrt
import pygame

OUTPUT_DIR = "voice_previews_free"

def play_eval_loop():
    if not os.path.exists(OUTPUT_DIR):
        print(f"❌ Directory '{OUTPUT_DIR}' not found. Make sure your batch generator script has created it.")
        return

    # Find and sort all generated MP3 files alphabetically
    audio_files = sorted([f for f in os.listdir(OUTPUT_DIR) if f.endswith(".mp3")])
    
    if not audio_files:
        print(f"📂 No MP3 previews found in '{OUTPUT_DIR}' yet. (The generator might still be working on the first few!)")
        return

    print(f"Found {len(audio_files)} preview files ready for evaluation.")
    print("--------------------------------------------------")
    print(" Controls:")
    print("  • Press ANY KEY to play the next voice.")
    print("  • Press 'q' to quit the evaluation loop.")
    print("--------------------------------------------------")

    # Initialize audio mixer
    pygame.mixer.init()

    for idx, filename in enumerate(audio_files, 1):
        filepath = os.path.join(OUTPUT_DIR, filename)
        
        print(f"\n[{idx}/{len(audio_files)}] Ready. Tap any key to play: {filename}")
        
        # Wait for a single key stroke without requiring Enter (Windows specific)
        key_press = msvcrt.getch()
        
        # Check if user wants to quit
        if key_press.lower() == b'q':
            print("Exiting evaluation loop.")
            break
            
        print(f" ▶ Playing: {filename}...")
        pygame.mixer.music.load(filepath)
        pygame.mixer.music.play()
        
        # Keep the script paused here until the audio finishes playing
        while pygame.mixer.music.get_busy():
            pygame.time.Clock().tick(10)

    print("\n🏁 Evaluation sequence finished!")

if __name__ == "__main__":
    play_eval_loop()
