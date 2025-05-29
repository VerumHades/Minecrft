from github import Github
import os,shutil


# === CONFIG ===
REPO_NAME = "VerumHades/MinecrftRelease"  # Replace with your actual repo name
VERSION_FILE = "version.txt"
BUILD_DIR = "build/Release"
BRANCH = "main"  # or "master" or whatever branch you're releasing from
FRESH_NAME = "Majnkraft-0.0.1-win64"  # fixed name your build system always produces

# === READ VERSION ===
with open(VERSION_FILE, "r") as f:
    version_number = f.read().strip()
    version = f"0.0.{version_number}"
    
with open(VERSION_FILE, "w") as f:
    f.write(str(int(version_number) + 1))
    
tag = f"v{version}"
release_name = f"Majnkraft {version}"
release_body = f"Automated release for version {version}"

# === RENAME FILES ===
src_exe = os.path.join(BUILD_DIR, f"{FRESH_NAME}.exe")
src_zip = os.path.join(BUILD_DIR, f"{FRESH_NAME}.zip")

new_exe = os.path.join(BUILD_DIR, f"Majnkraft-{version}-win64.exe")
new_zip = os.path.join(BUILD_DIR, f"Majnkraft-{version}-win64.zip")

# Rename if needed (overwrite if already exists)
if os.path.exists(src_exe):
    shutil.copyfile(src_exe, new_exe)
if os.path.exists(src_zip):
    shutil.copyfile(src_zip, new_zip)

assets = [new_exe, new_zip]

# === AUTHENTICATE ===
token = "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" #fuck it
if not token:
    raise EnvironmentError("Missing GITHUB_TOKEN environment variable")

g = Github(token)
repo = g.get_repo(REPO_NAME)

# === CREATE TAG (if not exists) ===
# Get latest commit on the release branch
commit = repo.get_branch(BRANCH).commit

try:
    # Try to get the tag — this will fail if it doesn't exist
    repo.get_git_ref(f"tags/{tag}")
    print(f"✔️ Tag {tag} already exists.")
except:
    # Create tag object
    tag_obj = repo.create_git_tag(
        tag=tag,
        message=f"Tag for version {version}",
        object=commit.sha,
        type="commit"
    )
    # Create tag reference
    repo.create_git_ref(ref=f"refs/tags/{tag}", sha=tag_obj.sha)
    print(f"✅ Created tag {tag} pointing to {commit.sha}")

# === CREATE RELEASE ===
release = repo.create_git_release(
    tag=tag,
    name=release_name,
    message=release_body,
    draft=False,
    prerelease=False
)

# === UPLOAD ASSETS ===
for asset in assets:
    if not os.path.isfile(asset):
        print(f"⚠️ File not found: {asset}")
        continue
    print(f"⬆️ Uploading {asset}...")
    release.upload_asset(asset)

print("✅ GitHub release created successfully.")