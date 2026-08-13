import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pathlib import Path
from scanner import Scanner

class App:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Imperium TaskRadar")
        self.root.geometry("1000x600")

        self.scanner = Scanner()
        self.results = {}

        self._create_widgets()
        self._layout_widgets()

    def _create_widgets(self):
        self.top_frame = ttk.Frame(self.root, padding="10")
        self.add_btn = ttk.Button(self.top_frame, text="Add Directory", command=self.add_directory)
        self.clear_btn = ttk.Button(self.top_frame, text="Clear All", command=self.clear_all)

        # Dropdown to select root directory
        self.dir_label = ttk.Label(self.top_frame, text="Select a root directory:")
        self.dir_combo = ttk.Combobox(self.top_frame, state="readonly", width=70)
        self.dir_combo.bind("<<ComboboxSelected>>", self.on_dir_selected)

        # Treeview to show category and files and TODOs
        self.tree_frame = ttk.Frame(self.root, padding="10")
        self.tree = ttk.Treeview(self.tree_frame, columns=("path",), show="tree headings")
        self.tree.heading("#0", text="Category / File / TODO", anchor="w")
        self.tree.column("#0", width=300, anchor="w")
        self.tree.heading("path", text="Full Path / Description", anchor="w")
        self.tree.column("path", width=550, anchor="w")

        # Scrollbars
        self.vsb = ttk.Scrollbar(self.tree_frame, orient="vertical", command=self.tree.yview)
        self.hsb = ttk.Scrollbar(self.tree_frame, orient="horizontal", command=self.tree.xview)
        self.tree.configure(yscrollcommand=self.vsb.set, xscrollcommand=self.hsb.set)


    def _layout_widgets(self):
        self.top_frame.pack(fill="x")
        self.add_btn.pack(side="left", padx=5)
        self.clear_btn.pack(side="left", padx=5)
        self.dir_label.pack(side="left", padx=(20, 5))
        self.dir_combo.pack(side="left", fill="x", expand=True, padx=5)

        self.tree_frame.pack(fill="both", expand=True, padx=10, pady=10)
        self.tree.grid(row=0, column=0, sticky="nsew")
        self.vsb.grid(row=0, column=1, sticky="ns")
        self.hsb.grid(row=1, column=0, sticky="ew")
        self.tree_frame.grid_rowconfigure(0, weight=1)
        self.tree_frame.grid_columnconfigure(0, weight=1)

    def add_directory(self):
        dir_path = filedialog.askdirectory(title="Select a root directory to scan")
        if not dir_path:
            return
        root = Path(dir_path).expanduser().resolve()
        try:
            categories = self.scanner.scan_directory(root)
        except Exception as e:
            messagebox.showerror("Error", f"Failed to scan directory:\n{e}")
            return

        self.results[str(root)] = categories
        self._update_combobox()
        self.dir_combo.set(str(root))
        self._display_results(str(root))

    def clear_all(self):
        self.results.clear()
        self.dir_combo['values'] = []
        self.dir_combo.set('')
        self.tree.delete(*self.tree.get_children())

    def _update_combobox(self):
        dirs = list(self.results.keys())
        self.dir_combo['values'] = dirs

    def on_dir_selected(self, event=None):
        selected = self.dir_combo.get()
        if selected:
            self._display_results(selected)

    def _display_results(self, root_path: str):
        # Clear existing tree
        children = self.tree.get_children()
        if children:
            self.tree.delete(*children)

        entries = self.results.get(root_path, [])
        if not entries:
            return

        root_name = Path(root_path).name
        root_node = self.tree.insert("", "end", text=root_name, values=(root_path,), open=True)

        node_map = {(): root_node}
        entries_sorted = sorted(entries, key=lambda e: e.relative_path.parts)
        
        for entry in entries_sorted:
            parts = entry.relative_path.parts
            dir_parts = parts[:-1]
            file_name = parts[-1]

            current_node = root_node
            current_tuple = ()
            for part in dir_parts:
                current_tuple = current_tuple + (part,)
                if current_tuple not in node_map:
                    parent = node_map[current_tuple[:-1]]
                    node_map[current_tuple] = self.tree.insert(parent, "end", text=part, values=("",), open=True)
                current_node = node_map[current_tuple]

            file_node = self.tree.insert(current_node, "end", text=file_name, values=(str(entry.path),))
            for todo in entry.todos:
                self.tree.insert(file_node, "end", text="TODO", values=(todo,))
