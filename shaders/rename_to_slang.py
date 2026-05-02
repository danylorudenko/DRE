import os
import sys
from pathlib import Path


def rename_to_slang(folder: str, recursive: bool = False, dry_run: bool = False) -> None:
    root = Path(folder)
    if not root.is_dir():
        print(f"Error: '{folder}' is not a valid directory.")
        sys.exit(1)

    pattern = "**/*" if recursive else "*"
    files = [p for p in root.glob(pattern) if p.is_file() and p.suffix != ".slang"]

    if not files:
        print("No files to rename.")
        return

    for file in files:
        new_path = file.with_suffix(".slang")
        if new_path.exists():
            print(f"SKIP (target exists): {file} -> {new_path.name}")
            continue
        print(f"{'DRY RUN: ' if dry_run else ''}Rename: {file} -> {new_path.name}")
        if not dry_run:
            file.rename(new_path)

    print(f"\nDone. {len(files)} file(s) processed.")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Rename all files in a folder to .slang extension.")
    parser.add_argument("folder", help="Path to the target folder.")
    parser.add_argument("-r", "--recursive", action="store_true", help="Also rename files in subdirectories.")
    parser.add_argument("--dry-run", action="store_true", help="Preview changes without renaming anything.")
    args = parser.parse_args()

    rename_to_slang(args.folder, recursive=args.recursive, dry_run=args.dry_run)
