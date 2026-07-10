import os

# Folders to ignore so the file doesn't get too massive
IGNORE_DIRS = ["ImGui", "DirectXTK", "x64", "Debug", "Release", ".git", ".vs"]

with open("AllCode.txt", "w", encoding="utf-8") as outfile:
    for root, dirs, files in os.walk("."):
        # Remove ignored directories from the search
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
        
        for file in files:
            # Only grab C++ headers and source files
            if file.endswith((".h", ".cpp")):
                filepath = os.path.join(root, file)
                
                # Write a nice header so I know what file I am looking at
                outfile.write(f"\n\n{'='*60}\n")
                outfile.write(f"FILE: {filepath}\n")
                outfile.write(f"{'='*60}\n\n")
                
                # Write the actual code
                with open(filepath, "r", encoding="utf-8", errors="ignore") as infile:
                    outfile.write(infile.read())

print("Success! Created AllCode.txt")
input("Press Enter to close this window...")