import os
import sys

PROGRESS_FILE = ".checked_files.txt"

def list_files_recursive(directory):
    file_list = []
    for root, dirs, files in os.walk(directory):
        for name in files:
            file_list.append(os.path.join(root, name))
    return sorted(file_list)

def load_checked_files():
    if not os.path.exists(PROGRESS_FILE):
        return set()
    with open(PROGRESS_FILE, "r") as f:
        return set(line.strip() for line in f)

def save_checked_file(filepath):
    with open(PROGRESS_FILE, "a") as f:
        f.write(filepath + "\n")

def check_off_files(files, checked_files):
    remaining = [f for f in files if f not in checked_files]
    total = len(files)

    for i, file in enumerate(remaining, len(checked_files) + 1):
        input(f"[{i}/{total}] {file}\nPress Enter to check off this file...")
        save_checked_file(file)

    print("\n✅ All files checked off!")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python check_files_with_progress.py <directory>")
        sys.exit(1)

    directory = sys.argv[1]
    if not os.path.isdir(directory):
        print("Invalid directory.")
        sys.exit(1)

    files = list_files_recursive(directory)
    if not files:
        print("No files found.")
        sys.exit(0)

    checked_files = load_checked_files()
    check_off_files(files, checked_files)
