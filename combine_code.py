import os
import argparse

def combine_code(directory, output_file, extensions=None):
    # Common code file extensions to include by default
    if extensions is None:
        extensions = ['.cpp', '.h']
    
    # Directories to ignore to prevent combining huge dependencies or binaries
    ignore_dirs = {
        '.git', 'node_modules', 'venv', 'env', '__pycache__', 
        '.idea', '.vscode', 'build', 'dist', 'target', 'out', 'imgui',
        'api', 'xtk', 'directxtk_utilities', 'packages'
    }
    
    combined_count = 0
    
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for root, dirs, files in os.walk(directory):
            # Modify dirs in-place to skip ignored directories and imgui
            dirs[:] = [d for d in dirs if d.lower() not in ignore_dirs and 'imgui' not in d.lower()]
            
            for file in files:
                # Skip imgui files completely
                if 'imgui' in file.lower():
                    continue
                    
                if any(file.endswith(ext) for ext in extensions):
                    file_path = os.path.join(root, file)
                    
                    # Prevent the output file from including itself if it matches the extensions
                    if os.path.abspath(file_path) == os.path.abspath(output_file):
                        continue
                        
                    try:
                        with open(file_path, 'r', encoding='utf-8', errors='replace') as infile:
                            content = infile.read()
                            
                        # Write separator and file path relative to the target directory
                        rel_path = os.path.relpath(file_path, directory)
                        outfile.write('\n' + '='*80 + '\n')
                        outfile.write(f'FILE: {rel_path}\n')
                        outfile.write('='*80 + '\n\n')
                        
                        outfile.write(content)
                        outfile.write('\n')
                        combined_count += 1
                        
                    except UnicodeDecodeError:
                        print(f"Skipping {file_path} (Binary or unsupported encoding)")
                    except Exception as e:
                        print(f"Skipping {file_path} due to error: {e}")

    return combined_count

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Combine code files into a single text document.")
    parser.add_argument("-d", "--directory", default=".", help="Directory to scan for code files (default: current directory)")
    parser.add_argument("-o", "--output", default="combined_code.txt", help="Path to the output text file (default: combined_code.txt)")
    parser.add_argument("-e", "--extensions", nargs="+", help="File extensions to include (e.g. .py .js). Default covers most common languages.")
    
    args = parser.parse_args()
    
    print(f"Scanning directory: {os.path.abspath(args.directory)}")
    print(f"Writing to: {os.path.abspath(args.output)}")
    
    count = combine_code(args.directory, args.output, args.extensions)
    print(f"\nSuccess! Combined {count} files into {args.output}")
