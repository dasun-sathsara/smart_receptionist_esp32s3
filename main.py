import argparse
import logging
import os
from pathlib import Path

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    handlers=[
        logging.FileHandler("file_merger.log"),
        logging.StreamHandler(),
    ],
)


def merge_text_files(directory, extensions, folders):
    """Merges text content of files with given extensions within specified folders."""

    merged_content = []
    for root, dirs, files in os.walk(directory):
        # Exclude hidden folders and filter based on command line input
        dirs[:] = [d for d in dirs if not d.startswith(".") and (not folders or d in folders)]

        for file in files:
            if file.endswith(extensions):
                file_path = os.path.join(root, file)
                relative_path = os.path.relpath(file_path, directory)  # Make path relative
                logging.info(f"Processing: {relative_path}")

                try:
                    with open(file_path, "r", encoding="utf-8") as f:
                        content = f.read()
                        # Create delimiters with matching lengths, using relative path
                        delimiter_start = f"############# {relative_path} #############"
                        delimiter_end = "#" * len(delimiter_start)
                        merged_content.append(f"{delimiter_start}\n\n{content}\n\n{delimiter_end}\n\n")
                except FileNotFoundError:
                    logging.warning(f"File not found: {relative_path}")
                except PermissionError:
                    logging.warning(f"Permission denied: {relative_path}")
                except UnicodeDecodeError:
                    logging.warning(f"Unable to decode file (likely not text): {relative_path}")

    return merged_content


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Merge text files from a directory.")
    parser.add_argument(
        "-e",
        "--extensions",
        type=str,
        default=".txt",
        help="Comma-separated list of file extensions to include (e.g., .py,.txt,.md)",
    )
    parser.add_argument(
        "-f",
        "--folders",
        type=str,
        default=None,
        help="Comma-separated list of folder names to include (optional)",
    )

    args = parser.parse_args()

    # Convert command line extensions to a tuple.
    extensions = tuple(ext.strip() for ext in args.extensions.split(","))

    # Convert command line folders to a tuple if it's provided.
    if args.folders:
        folders = tuple(folder.strip() for folder in args.folders.split(","))
    else:
        folders = None

    # Ensure the current working directory exists
    directory = Path.cwd()
    if not directory.exists():
        logging.error(f"The specified directory '{directory}' does not exist.")
        exit(1)

    try:
        merged_content = merge_text_files(directory, extensions, folders)
    except Exception as e:  # Catch any unexpected errors
        logging.error(f"An error occurred during file merging: {e}")
        exit(1)

    if not merged_content:
        logging.warning(f"No files found matching the specified extensions and folders.")
        exit(0)

    output_file = "merged_text.txt"
    try:
        with open(output_file, "w", encoding="utf-8") as outfile:
            outfile.writelines(merged_content)
        logging.info(f"Merged text saved to: {output_file}")
    except PermissionError:
        logging.error(f"Permission denied to create output file: {output_file}")
    except Exception as e:  # Catch other potential errors
        logging.error(f"An error occurred while saving merged content: {e}")
