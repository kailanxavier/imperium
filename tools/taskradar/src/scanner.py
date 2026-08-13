from pathlib import Path
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Dict, List, Tuple
import re

CPP_EXTENSIONS = { '.cpp', '.hpp', '.h' }
TODO_PATTERN = re.compile(r'\bTODO\b:?\s*', re.IGNORECASE)

@dataclass
class FileEntry:
    # Represents a file and its extracted TODOs
    path: Path
    relative_path: Path
    todos: List[str] = field(default_factory=list)

class Scanner:
    # Handle recursive scanning of directories
    def __init__(self, extensions: set = CPP_EXTENSIONS):
        self.extensions = extensions

    def find_files(self, root: Path) -> List[Path]:
        # Return list of cpp files under given dir
        if not root.is_dir():
            raise NotADirectoryError(f"Not a directory: {root}")
        return [
            p for p in root.rglob('*')
            if p.is_file() and p.suffix.lower() in self.extensions
        ]

    def extract_todos(self, file_path: Path) -> List[str]:
        todos = []
        try:
            lines = file_path.read_text(encoding='utf-8', errors='ignore').splitlines()
        except OSError:
            return todos

        i = 0
        while i < len(lines):
            line = lines[i].strip()

            # Only consider lines that start with (line) comment
            if (line.startswith('//')):
                comment_text = line[2:].strip()
                match = TODO_PATTERN.search(comment_text)
                if match:
                    # Extract text after TODOs marker
                    description = comment_text[match.end():].strip()
                    j = i + 1

                    # Collect continuation comment lines
                    while j < len(lines):
                        next_line = lines[j].strip()
                        if next_line.startswith('//'):
                            cont_text = next_line[2:].strip()
                            if (TODO_PATTERN.search(cont_text) or 
                                cont_text.upper().startswith(('FIXME', 'HACK', 'XXX'))):
                                break
                            if cont_text:
                                description += ' ' + cont_text
                            j += 1
                        else:
                            break

                    todos.append(description.strip())
                    i = j
                    continue
            i += 1
        return todos

    def scan_directory(self, root: Path) -> List[FileEntry]:
        entries = []
        for file_path in self.find_files(root):
            todos = self.extract_todos(file_path)
            if todos:
                rel = file_path.relative_to(root)
                entries.append(FileEntry(path=file_path, relative_path=rel, todos=todos))
        return entries
