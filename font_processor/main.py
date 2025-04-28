import json
import argparse
import os
from fontTools.ttLib import TTFont
from fontTools.subset import Subsetter, Options

def extract_text_from_file(file_path):
    """Extract text from a file based on its extension."""
    ext = os.path.splitext(file_path)[1].lower()
    if ext == '.txt':
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    elif ext == '.json':
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
            return extract_text_from_json(data)
    else:
        raise ValueError(f"Unsupported file type: {ext}")

def extract_text_from_json(data):
    """Recursively extract all string values from a JSON object."""
    if isinstance(data, dict):
        return ''.join(extract_text_from_json(v) for v in data.values())
    elif isinstance(data, list):
        return ''.join(extract_text_from_json(item) for item in data)
    elif isinstance(data, str):
        return data
    else:
        return ''

def get_unique_characters(files):
    """Get a set of unique characters from a list of files."""
    unique_chars = set()
    for file in files:
        text = extract_text_from_file(file)
        unique_chars.update(text)
    return unique_chars

def subset_font(font_path, characters, output_path):
    """Subset the font to include only the specified characters, optimized for size."""
    options = Options()
    options.no_hinting = True  # Remove hinting to reduce size
    options.no_layout_closure = True  # Avoid including related glyphs
    options.prune_unicode_ranges = True  # Remove unused Unicode ranges
    subsetter = Subsetter(options)
    font = TTFont(font_path)
    subsetter.populate(text=''.join(characters))
    subsetter.subset(font)
    font.save(output_path)

def main():
    parser = argparse.ArgumentParser(description="Subset NotoSansSC-Regular.ttf to a minimal TTF file.")
    parser.add_argument('files', nargs='+', help="List of .txt or .json files containing text.")
    parser.add_argument('--font', default='NotoSansSC-Regular.ttf', help="Path to the font file.")
    parser.add_argument('--output', default='NotoSansSC-Subset.ttf', help="Output path for the subsetted font.")
    args = parser.parse_args()

    # Validate input files
    for file in args.files:
        if not os.path.exists(file):
            raise FileNotFoundError(f"Input file not found: {file}")

    # Validate font file
    if not os.path.exists(args.font):
        raise FileNotFoundError(f"Font file not found: {args.font}")

    # Extract unique characters
    unique_chars = get_unique_characters(args.files)
    print(f"Found {len(unique_chars)} unique characters for subsetting.")

    # Subset the font
    subset_font(args.font, unique_chars, args.output)
    print(f"Optimized subsetted font saved to {args.output}")

if __name__ == "__main__":
    main()