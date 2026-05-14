"""
Package 4 — Double McHarvard with Cheese Circular Shifts
Processor Simulator GUI  v2
────────────────────────────────────────────────────────
Run:   python processor_gui.py
Click  Load Binary  →  select your compiled C executable  →  Start
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, messagebox
import subprocess, threading, queue, re, os, random

# ── CONFIG ────────────────────────────────────────────────────────────────────
BINARY_NAME    = "e.exe"
NUM_REGS       = 64
INSTR_MEM_SIZE = 32
DATA_MEM_SIZE  = 64
REFRESH_MS     = 80

# ── THEMES ────────────────────────────────────────────────────────────────────
DARK = {
    "BG":       "#0d1117", "PANEL":    "#161b22", "PANEL2":   "#1c2128",
    "BORDER":   "#30363d", "ACCENT":   "#58a6ff", "ACCENT2":  "#1f6feb",
    "GREEN":    "#3fb950", "YELLOW":   "#d29922", "RED":      "#f85149",
    "PURPLE":   "#bc8cff", "ORANGE":   "#f0883e", "TEAL":     "#39d353",
    "MUTED":    "#8b949e", "FG":       "#e6edf3", "CONBG":    "#010409",
    "HDR_BG":   "#1c2128", "ROW_ALT":  "#1a2233", "BADGE_FG": "#ffffff",
    "IF_CLR":   "#1f6feb", "ID_CLR":   "#388bfd", "EX_CLR":   "#58a6ff",
    "CARD":     "#21262d",
}
PINK = {
    "BG":       "#ffafcc", "PANEL":    "#ffffff", "PANEL2":   "#ffc8dd",
    "BORDER":   "#ff85a2", "ACCENT":   "#fb6f92", "ACCENT2":  "#ff92bb",
    "GREEN":    "#2f9e44", "YELLOW":   "#f08c00", "RED":      "#e03131",
    "PURPLE":   "#9c36b5", "ORANGE":   "#e8590c", "TEAL":     "#087f5b",
    "MUTED":    "#868e96", "FG":       "#495057", "CONBG":    "#fffcfd",
    "HDR_BG":   "#ffc8dd", "ROW_ALT":  "#ffe5ec", "BADGE_FG": "#ffffff",
    "IF_CLR":   "#fb6f92", "ID_CLR":   "#ff85a2", "EX_CLR":   "#ff92bb",
    "CARD":     "#ffffff",
}

F_MONO  = ("Courier New", 10)
F_UI    = ("Segoe UI",    10)
F_SM    = ("Courier New",  9)
F_TINY  = ("Courier New",  8)
F_BIG   = ("Segoe UI",    13, "bold")
F_BADGE = ("Segoe UI",     8, "bold")


def signed8(v):
    return v - 256 if v > 127 else v


class ProcessorGUI:
    # =========================================================================
    def __init__(self, root):
        self.root = root
        self.root.title("Package 4 — Processor Simulator")
        self.root.geometry("1650x960")
        self.root.minsize(1300, 800)

        self._T    = DARK
        self._pink = False

        self.process   = None
        self.out_queue = queue.Queue()
        self.running   = False

        # processor state
        self.clock_cycle = 0
        self.regs        = [0] * NUM_REGS
        self.pc          = 0
        self.sreg        = 0
        self.instr_mem   = [""] * 1024
        self.data_mem    = [0]  * 2048
        self.stage_if    = ""
        self.stage_id    = ""
        self.stage_ex    = ""
        self.change_log  = []
        self._log_shown  = 0
        self._start_time = 0

        # widget registries — used for full theme repaint
        self._bg_widgets   = []   # (widget, theme_key)
        self._fg_widgets   = []   # (widget, theme_key)
        self._sec_labels   = []
        self._panels       = []
        self._btn_meta     = []   # (button, bg_key)

        # grid row registries — each entry is a flat tuple of widgets
        # reg:   (frame, idx_l, name_l, dec_l, hex_l, bin_l)
        # instr: (frame, addr_l, bin_l)
        # data:  (frame, addr_l, dec_l, hex_l, bin_l)
        self._reg_rows   = []
        self._instr_rows = []
        self._data_rows  = []

        # stage widgets: list of dicts for easy access
        self._stages = []   # {"name", "badge", "instr_l", "frame"}

        # SREG flag value labels
        self._sreg_val_lbls = {}   # flag_char -> Label

        self._build_ui()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self._poll_queue()

    # =========================================================================
    # THEME HELPERS
    # =========================================================================
    def T(self, k):
        return self._T[k]

    def _toggle_theme(self):
        self._pink = not self._pink
        self._T    = PINK if self._pink else DARK
        self.btn_theme.configure(
            text="🌙 Dark Mode" if self._pink else "🌸 Pink Mode",
            bg=self.T("ACCENT"), fg=self.T("BADGE_FG"),
            activebackground=self.T("ACCENT"),
            activeforeground=self.T("BADGE_FG"))
        self._repaint()

    def _repaint(self):
        T = self._T
        self.root.configure(bg=T["BG"])

        for w, k in self._bg_widgets:
            try: w.configure(bg=T[k])
            except: pass
        for w, k in self._fg_widgets:
            try: w.configure(fg=T[k])
            except: pass

        for lbl in self._sec_labels:
            lbl.configure(bg=T["BG"], fg=T["MUTED"])
        for pnl in self._panels:
            pnl.configure(bg=T["PANEL"], highlightbackground=T["BORDER"])
        for btn, k in self._btn_meta:
            btn.configure(bg=T[k], fg=T["BADGE_FG"],
                          activebackground=T[k], activeforeground=T["BADGE_FG"])
            btn.bind("<Enter>", lambda e, b=btn, ck=k: b.configure(bg=self._lighten(self.T(ck))))
            btn.bind("<Leave>", lambda e, b=btn, ck=k: b.configure(bg=self.T(ck)))

        # stage cards
        clr_map = {"IF": "IF_CLR", "ID": "ID_CLR", "EX": "EX_CLR"}
        for s in self._stages:
            c = T[clr_map[s["name"]]]
            s["frame"].configure(bg=T["CARD"], highlightbackground=c)
            s["header"].configure(bg=c)
            s["badge"].configure(bg=c,       fg=T["BADGE_FG"])
            s["sub"].configure(  bg=c,       fg=T["BADGE_FG"])
            s["instr_l"].configure(bg=T["CARD"], fg=T["FG"])

        # SREG flag cards
        for flag, d in self._sreg_cards.items():
            d["card"].configure(bg=T["CARD"], highlightbackground=T["BORDER"])
            d["name_l"].configure(bg=T["CARD"], fg=T["ACCENT"])
            d["desc_l"].configure(bg=T["CARD"], fg=T["MUTED"])
            bit = {"C":4,"V":3,"N":2,"S":1,"Z":0}[flag]
            val = (self.sreg >> bit) & 1
            d["val_l"].configure(bg=T["CARD"],
                                 fg=T["ACCENT"] if val else T["MUTED"])

        # register rows
        for row in self._reg_rows:
            frame, idx_l, name_l, dec_l, hex_l, bin_l = row
            frame.configure( bg=T["PANEL"])
            idx_l.configure( bg=T["PANEL"], fg=T["MUTED"])
            name_l.configure(bg=T["PANEL"], fg=T["ACCENT"])
            dec_l.configure( bg=T["PANEL"], fg=T["GREEN"])
            hex_l.configure( bg=T["PANEL"], fg=T["YELLOW"])
            bin_l.configure( bg=T["PANEL"], fg=T["MUTED"])

        # instruction rows
        for row in self._instr_rows:
            frame, addr_l, bin_l = row
            frame.configure( bg=T["PANEL"])
            addr_l.configure(bg=T["PANEL"], fg=T["MUTED"])
            bin_l.configure( bg=T["PANEL"], fg=T["PURPLE"])

        # data rows
        for row in self._data_rows:
            frame, addr_l, dec_l, hex_l, bin_l = row
            frame.configure( bg=T["PANEL"])
            addr_l.configure(bg=T["PANEL"], fg=T["MUTED"])
            dec_l.configure( bg=T["PANEL"], fg=T["FG"])
            hex_l.configure( bg=T["PANEL"], fg=T["YELLOW"])
            bin_l.configure( bg=T["PANEL"], fg=T["MUTED"])

        # consoles
        self.console.configure(    bg=T["CONBG"], fg=T["FG"])
        self.change_text.configure(bg=T["PANEL"], fg=T["FG"])
        self._retag_console()
        self._retag_changelog()

    def _retag_console(self):
        T = self._T
        for tag, clr in [
            ("cycle",     T["ACCENT"]), ("if_tag",    T["IF_CLR"]),
            ("id_tag",    T["ID_CLR"]), ("ex_tag",    T["EX_CLR"]),
            ("reg_write", T["GREEN"]),  ("mem_write", T["ORANGE"]),
            ("flush_tag", T["RED"]),    ("stall_tag", T["PURPLE"]),
            ("sreg_tag",  T["YELLOW"]), ("dim",       T["MUTED"]),
        ]:
            self.console.tag_configure(tag, foreground=clr)

    def _retag_changelog(self):
        T = self._T
        for tag, clr in [
            ("reg_change",  T["GREEN"]),  ("mem_change", T["ORANGE"]),
            ("pc_change",   T["ACCENT"]), ("sreg_change",T["YELLOW"]),
            ("flush",       T["RED"]),    ("stall",      T["PURPLE"]),
            ("dim",         T["MUTED"]),
        ]:
            self.change_text.tag_configure(tag, foreground=clr)

    def _lighten(self, hex_clr):
        # simple hex lighten for hover effect
        hex_clr = hex_clr.lstrip('#')
        rgb = tuple(int(hex_clr[i:i+2], 16) for i in (0, 2, 4))
        new_rgb = tuple(min(255, int(c * 1.15)) for c in rgb)
        return '#%02x%02x%02x' % new_rgb

    # =========================================================================
    # UI CONSTRUCTION
    # =========================================================================
    def _build_ui(self):
        T = self._T

        # ── TOP BAR (DASHBOARD HEADER) ──────────────────────────────────────
        top = tk.Frame(self.root, bg=T["PANEL"], height=70)
        top.pack(fill=tk.X, side=tk.TOP)
        top.pack_propagate(False)
        self._bg_widgets.append((top, "PANEL"))

        accent_bar = tk.Frame(top, bg=T["ACCENT"], width=4)
        accent_bar.pack(side=tk.LEFT, fill=tk.Y)
        self._bg_widgets.append((accent_bar, "ACCENT"))

        # Action section (Pack RIGHT first to pin to the side)
        actions = tk.Frame(top, bg=T["PANEL"])
        actions.pack(side=tk.RIGHT, padx=20)
        self._bg_widgets.append((actions, "PANEL"))

        self.btn_theme = tk.Button(actions, text="✨ THEME",
            command=self._toggle_theme,
            bg=T["ACCENT"], fg=T["BADGE_FG"],
            font=("Segoe UI", 8, "bold"), relief="flat", padx=12, pady=6,
            cursor="hand2", bd=0)
        self.btn_theme.pack(side=tk.RIGHT, padx=4)
        self.btn_theme.bind("<Enter>", lambda e: self.btn_theme.configure(bg=self._lighten(self.T("ACCENT"))))
        self.btn_theme.bind("<Leave>", lambda e: self.btn_theme.configure(bg=self.T("ACCENT")))

        self.btn_stop  = self._mkbtn(actions, "■ STOP",  self._stop,  "RED")
        self.btn_start = self._mkbtn(actions, "▶ START", self._start, "GREEN")
        self.btn_next  = self._mkbtn(actions, "⏭ NEXT",  self._step,  "ACCENT")
        self.btn_load  = self._mkbtn(actions, "📂 LOAD",  self._load_binary, "ACCENT2")

        # Brand area
        brand = tk.Frame(top, bg=T["PANEL"])
        brand.pack(side=tk.LEFT, padx=(10, 20))
        self._bg_widgets.append((brand, "PANEL"))

        title_lbl = tk.Label(brand, text="Package 4 Simulator",
                             bg=T["PANEL"], fg=T["FG"],
                             font=("Segoe UI", 12, "bold"))
        title_lbl.pack(anchor="w")
        self._bg_widgets.append((title_lbl, "PANEL"))
        self._fg_widgets.append((title_lbl, "FG"))

        sub_lbl = tk.Label(brand, text="MC-HARVARD 8-BIT",
                           bg=T["PANEL"], fg=T["MUTED"], font=("Segoe UI", 7, "bold"))
        sub_lbl.pack(anchor="w")
        self._bg_widgets.append((sub_lbl, "PANEL"))
        self._fg_widgets.append((sub_lbl, "MUTED"))

        # Stat section
        stats = tk.Frame(top, bg=T["PANEL"])
        stats.pack(side=tk.LEFT, padx=10, fill=tk.Y)
        self._bg_widgets.append((stats, "PANEL"))

        self.clock_lbl  = self._pill(stats, "Cycle",  "0",        "ACCENT")
        self.pc_pill    = self._pill(stats, "PC",     "0",        "GREEN")
        self.sreg_pill  = self._pill(stats, "SREG",   "00000000", "YELLOW")
        self.status_lbl = self._pill(stats, "Status", "IDLE",     "MUTED")
        self.btn_start["state"] = "disabled"
        self.btn_stop ["state"] = "disabled"
        self.btn_next ["state"] = "disabled"

        # ── MAIN AREA ───────────────────────────────────────────────────────
        main = tk.Frame(self.root, bg=T["BG"])
        main.pack(fill=tk.BOTH, expand=True, padx=10, pady=8)
        self._bg_widgets.append((main, "BG"))
        main.columnconfigure(0, weight=2, minsize=310)
        main.columnconfigure(1, weight=3, minsize=440)
        main.columnconfigure(2, weight=2, minsize=310)
        main.rowconfigure(0, weight=1)

        self.col_l = tk.Frame(main, bg=T["BG"])
        self.col_c = tk.Frame(main, bg=T["BG"])
        self.col_r = tk.Frame(main, bg=T["BG"])
        self.col_l.grid(row=0, column=0, sticky="nsew", padx=(0, 6))
        self.col_c.grid(row=0, column=1, sticky="nsew", padx=6)
        self.col_r.grid(row=0, column=2, sticky="nsew", padx=(6, 0))
        for col in (self.col_l, self.col_c, self.col_r):
            col.columnconfigure(0, weight=1)
            self._bg_widgets.append((col, "BG"))

        self._build_left()
        self._build_centre()
        self._build_right()

    # ── LEFT COLUMN ──────────────────────────────────────────────────────────
    def _build_left(self):
        T  = self._T
        col = self.col_l

        # Pipeline stages
        self._sec(col, "🔄  Pipeline Stages", 0)
        pipe_panel = self._panel(col, 1)
        self._sreg_cards = {}   # will also be populated in SREG section

        clr_map  = {"IF": "IF_CLR", "ID": "ID_CLR", "EX": "EX_CLR"}
        desc_map = {"IF": "Instruction Fetch",
                    "ID": "Instruction Decode",
                    "EX": "Execute  ·  ALU  ·  Mem  ·  Writeback"}
        for name in ("IF", "ID", "EX"):
            c    = T[clr_map[name]]
            card = tk.Frame(pipe_panel, bg=T["CARD"],
                            highlightbackground=c, highlightthickness=2)
            card.pack(fill=tk.X, padx=8, pady=5)

            header = tk.Frame(card, bg=c)
            header.pack(fill=tk.X)

            badge = tk.Label(header, text=f"  {name}  ",
                             bg=c, fg=T["BADGE_FG"],
                             font=("Segoe UI", 9, "bold"))
            badge.pack(side=tk.LEFT, padx=4, pady=3)

            sub = tk.Label(header, text=desc_map[name],
                           bg=c, fg=T["BADGE_FG"], font=("Segoe UI", 8))
            sub.pack(side=tk.LEFT)

            instr_l = tk.Label(card, text="—", bg=T["CARD"], fg=T["FG"],
                               font=F_SM, anchor="w", wraplength=270, justify="left")
            instr_l.pack(fill=tk.X, padx=8, pady=(4, 7))

            self._stages.append({
                "name": name, "badge": badge, "instr_l": instr_l,
                "frame": card, "header": header, "sub": sub,
            })

        # SREG flags
        self._sec(col, "🚩  SREG Flags", 2)
        sreg_panel = self._panel(col, 3)
        flag_row   = tk.Frame(sreg_panel, bg=T["PANEL"])
        flag_row.pack(fill=tk.X, padx=8, pady=8)
        self._bg_widgets.append((flag_row, "PANEL"))

        flag_desc = {"C":"Carry","V":"Oflow","N":"Neg","S":"Sign","Z":"Zero"}
        for flag in ("C", "V", "N", "S", "Z"):
            card = tk.Frame(flag_row, bg=T["CARD"],
                            highlightbackground=T["BORDER"], highlightthickness=1)
            card.pack(side=tk.LEFT, padx=3, expand=True, fill=tk.X)

            name_l = tk.Label(card, text=flag, bg=T["CARD"], fg=T["ACCENT"],
                              font=("Segoe UI", 10, "bold"))
            name_l.pack(pady=(6, 0))

            val_l  = tk.Label(card, text="0", bg=T["CARD"], fg=T["MUTED"],
                              font=("Courier New", 14, "bold"))
            val_l.pack()
            self._sreg_val_lbls[flag] = val_l

            desc_l = tk.Label(card, text=flag_desc[flag], bg=T["CARD"],
                              fg=T["MUTED"], font=("Segoe UI", 7))
            desc_l.pack(pady=(0, 6))

            self._sreg_cards[flag] = {
                "card":   card,
                "name_l": name_l,
                "val_l":  val_l,
                "desc_l": desc_l,
            }

        # Change log
        self._sec(col, "📝  Change Log", 4)
        col.rowconfigure(5, weight=1)
        clog_panel = self._panel(col, 5, sticky="nsew")
        self.change_text = scrolledtext.ScrolledText(
            clog_panel, bg=T["PANEL"], fg=T["FG"],
            font=F_TINY, relief="flat", wrap=tk.NONE, height=10)
        self.change_text.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)
        self.change_text.configure(state="disabled")
        self._retag_changelog()

    # ── CENTRE COLUMN ────────────────────────────────────────────────────────
    def _build_centre(self):
        T   = self._T
        col = self.col_c

        # Register file
        self._sec(col, "📋  Register File  (R0–R63  ·  PC  ·  SREG)", 0)
        col.rowconfigure(1, weight=2)
        reg_panel = self._panel(col, 1, sticky="nsew")

        rc = tk.Canvas(reg_panel, bg=T["PANEL"], highlightthickness=0)
        rs = ttk.Scrollbar(reg_panel, orient="vertical", command=rc.yview)
        rc.configure(yscrollcommand=rs.set)
        rs.pack(side=tk.RIGHT, fill=tk.Y)
        rc.pack(fill=tk.BOTH, expand=True)
        ri = tk.Frame(rc, bg=T["PANEL"])
        rc.create_window((0, 0), window=ri, anchor="nw")
        ri.bind("<Configure>",
            lambda e: rc.configure(scrollregion=rc.bbox("all")))
        self._bg_widgets.append((rc, "PANEL"))
        self._bg_widgets.append((ri, "PANEL"))
        self._build_reg_grid(ri)

        # Console
        self._sec(col, "🖥  Console Output", 2)
        col.rowconfigure(3, weight=3)
        con_panel = self._panel(col, 3, sticky="nsew")
        self.console = scrolledtext.ScrolledText(
            con_panel, bg=T["CONBG"], fg=T["FG"],
            font=F_SM, relief="flat", wrap=tk.WORD, height=12)
        self.console.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)
        self.console.configure(state="disabled")
        self._retag_console()

    # ── RIGHT COLUMN ─────────────────────────────────────────────────────────
    def _build_right(self):
        T   = self._T
        col = self.col_r

        # Instruction memory
        self._sec(col, f"💾  Instruction Memory  (first {INSTR_MEM_SIZE})", 0)
        col.rowconfigure(1, weight=2)
        im_panel = self._panel(col, 1, sticky="nsew")
        ic = tk.Canvas(im_panel, bg=T["PANEL"], highlightthickness=0)
        is_ = ttk.Scrollbar(im_panel, orient="vertical", command=ic.yview)
        ic.configure(yscrollcommand=is_.set)
        is_.pack(side=tk.RIGHT, fill=tk.Y)
        ic.pack(fill=tk.BOTH, expand=True)
        ii = tk.Frame(ic, bg=T["PANEL"])
        ic.create_window((0, 0), window=ii, anchor="nw")
        ii.bind("<Configure>",
            lambda e: ic.configure(scrollregion=ic.bbox("all")))
        self._bg_widgets.append((ic, "PANEL"))
        self._bg_widgets.append((ii, "PANEL"))
        self._build_instr_grid(ii)

        # Data memory
        self._sec(col, f"🗄  Data Memory  (first {DATA_MEM_SIZE} bytes)", 2)
        col.rowconfigure(3, weight=3)
        dm_panel = self._panel(col, 3, sticky="nsew")
        dc = tk.Canvas(dm_panel, bg=T["PANEL"], highlightthickness=0)
        ds = ttk.Scrollbar(dm_panel, orient="vertical", command=dc.yview)
        dc.configure(yscrollcommand=ds.set)
        ds.pack(side=tk.RIGHT, fill=tk.Y)
        dc.pack(fill=tk.BOTH, expand=True)
        di = tk.Frame(dc, bg=T["PANEL"])
        dc.create_window((0, 0), window=di, anchor="nw")
        di.bind("<Configure>",
            lambda e: dc.configure(scrollregion=dc.bbox("all")))
        self._bg_widgets.append((dc, "PANEL"))
        self._bg_widgets.append((di, "PANEL"))
        self._build_data_grid(di)

    # ── grid builders ─────────────────────────────────────────────────────────
    def _grid_hdr(self, parent, cols):
        T   = self._T
        hdr = tk.Frame(parent, bg=T["HDR_BG"])
        hdr.pack(fill=tk.X, padx=2, pady=(2, 1))
        self._bg_widgets.append((hdr, "HDR_BG"))
        for txt, w in cols:
            l = tk.Label(hdr, text=txt, bg=T["HDR_BG"], fg=T["MUTED"],
                         font=("Courier New", 8, "bold"), width=w, anchor="w")
            l.pack(side=tk.LEFT, padx=3, pady=2)
            self._bg_widgets.append((l, "HDR_BG"))
            self._fg_widgets.append((l, "MUTED"))

    def _build_reg_grid(self, parent):
        T = self._T
        self._grid_hdr(parent,
            [("#", 4), ("Reg", 6), ("Dec", 7), ("Hex", 7), ("Bin", 10)])
        for i in range(NUM_REGS):
            f      = tk.Frame(parent, bg=T["PANEL"])
            f.pack(fill=tk.X, padx=2)
            idx_l  = tk.Label(f, text=f"{i:02d}", bg=T["PANEL"], fg=T["MUTED"],
                              font=F_TINY, width=4,  anchor="w")
            name_l = tk.Label(f, text=f"R{i}",    bg=T["PANEL"], fg=T["ACCENT"],
                              font=F_TINY, width=6,  anchor="w")
            dec_l  = tk.Label(f, text="0",         bg=T["PANEL"], fg=T["GREEN"],
                              font=F_TINY, width=7,  anchor="w")
            hex_l  = tk.Label(f, text="0x00",      bg=T["PANEL"], fg=T["YELLOW"],
                              font=F_TINY, width=7,  anchor="w")
            bin_l  = tk.Label(f, text="00000000",  bg=T["PANEL"], fg=T["MUTED"],
                              font=F_TINY, width=10, anchor="w")
            for w in (idx_l, name_l, dec_l, hex_l, bin_l):
                w.pack(side=tk.LEFT, padx=3)
            # store as named tuple-like flat tuple: (frame, idx_l, name_l, dec_l, hex_l, bin_l)
            self._reg_rows.append((f, idx_l, name_l, dec_l, hex_l, bin_l))

        # PC special row
        rf = tk.Frame(parent, bg=T["ROW_ALT"])
        rf.pack(fill=tk.X, padx=2, pady=(4, 0))
        self._bg_widgets.append((rf, "ROW_ALT"))
        tk.Label(rf, text="──", bg=T["ROW_ALT"], fg=T["MUTED"],
                 font=F_TINY, width=4, anchor="w").pack(side=tk.LEFT, padx=3)
        nl = tk.Label(rf, text="PC", bg=T["ROW_ALT"], fg=T["ACCENT"],
                      font=("Courier New", 8, "bold"), width=6, anchor="w")
        nl.pack(side=tk.LEFT, padx=3)
        self._bg_widgets.append((nl, "ROW_ALT"))
        self._fg_widgets.append((nl, "ACCENT"))
        self.pc_dec_lbl = tk.Label(rf, text="0",      bg=T["ROW_ALT"], fg=T["GREEN"],
                                   font=F_TINY, width=7, anchor="w")
        self.pc_hex_lbl = tk.Label(rf, text="0x0000", bg=T["ROW_ALT"], fg=T["YELLOW"],
                                   font=F_TINY, width=7, anchor="w")
        for w in (self.pc_dec_lbl, self.pc_hex_lbl):
            w.pack(side=tk.LEFT, padx=3)
            self._bg_widgets.append((w, "ROW_ALT"))

        # SREG special row
        sf = tk.Frame(parent, bg=T["ROW_ALT"])
        sf.pack(fill=tk.X, padx=2, pady=(1, 4))
        self._bg_widgets.append((sf, "ROW_ALT"))
        tk.Label(sf, text="──", bg=T["ROW_ALT"], fg=T["MUTED"],
                 font=F_TINY, width=4, anchor="w").pack(side=tk.LEFT, padx=3)
        sl = tk.Label(sf, text="SREG", bg=T["ROW_ALT"], fg=T["ACCENT"],
                      font=("Courier New", 8, "bold"), width=6, anchor="w")
        sl.pack(side=tk.LEFT, padx=3)
        self._bg_widgets.append((sl, "ROW_ALT"))
        self._fg_widgets.append((sl, "ACCENT"))
        self.sreg_dec_lbl = tk.Label(sf, text="0",        bg=T["ROW_ALT"], fg=T["GREEN"],
                                     font=F_TINY, width=7, anchor="w")
        self.sreg_bin_lbl = tk.Label(sf, text="00000000", bg=T["ROW_ALT"], fg=T["MUTED"],
                                     font=F_TINY, width=10, anchor="w")
        for w in (self.sreg_dec_lbl, self.sreg_bin_lbl):
            w.pack(side=tk.LEFT, padx=3)
            self._bg_widgets.append((w, "ROW_ALT"))

    def _build_instr_grid(self, parent):
        T = self._T
        self._grid_hdr(parent, [("Addr", 6), ("Binary  (16-bit)", 18)])
        for i in range(INSTR_MEM_SIZE):
            f      = tk.Frame(parent, bg=T["PANEL"])
            f.pack(fill=tk.X, padx=2)
            addr_l = tk.Label(f, text=f"{i:04d}", bg=T["PANEL"], fg=T["MUTED"],
                              font=F_TINY, width=6,  anchor="w")
            bin_l  = tk.Label(f, text="—",         bg=T["PANEL"], fg=T["PURPLE"],
                              font=F_TINY, width=18, anchor="w")
            addr_l.pack(side=tk.LEFT, padx=3)
            bin_l.pack( side=tk.LEFT, padx=3, fill=tk.X, expand=True)
            self._instr_rows.append((f, addr_l, bin_l))

    def _build_data_grid(self, parent):
        T = self._T
        self._grid_hdr(parent, [("Addr", 6), ("Dec", 5), ("Hex", 7), ("Bin", 10)])
        for i in range(DATA_MEM_SIZE):
            f      = tk.Frame(parent, bg=T["PANEL"])
            f.pack(fill=tk.X, padx=2)
            addr_l = tk.Label(f, text=f"{i:04d}",    bg=T["PANEL"], fg=T["MUTED"],
                              font=F_TINY, width=6,  anchor="w")
            dec_l  = tk.Label(f, text="0",            bg=T["PANEL"], fg=T["FG"],
                              font=F_TINY, width=5,  anchor="w")
            hex_l  = tk.Label(f, text="0x00",         bg=T["PANEL"], fg=T["YELLOW"],
                              font=F_TINY, width=7,  anchor="w")
            bin_l  = tk.Label(f, text="00000000",     bg=T["PANEL"], fg=T["MUTED"],
                              font=F_TINY, width=10, anchor="w")
            for w in (addr_l, dec_l, hex_l, bin_l):
                w.pack(side=tk.LEFT, padx=3)
            self._data_rows.append((f, addr_l, dec_l, hex_l, bin_l))

    # ── widget helpers ─────────────────────────────────────────────────────────
    def _pill(self, parent, label, value, clr_key):
        T = self._T
        frame = tk.Frame(parent, bg=T["CARD"],
                         highlightbackground=T["BORDER"], highlightthickness=1)
        frame.pack(side=tk.LEFT, padx=6, pady=8)
        self._bg_widgets.append((frame, "CARD"))

        lk = tk.Label(frame, text=f" {label.upper()} ", bg=T["CARD"],
                      fg=T["MUTED"], font=("Segoe UI", 7, "bold"))
        lk.pack(side=tk.LEFT, padx=(6, 0), pady=6)
        self._bg_widgets.append((lk, "CARD"))
        self._fg_widgets.append((lk, "MUTED"))

        vk = tk.Label(frame, text=value, bg=T["CARD"],
                      fg=T[clr_key], font=("Courier New", 10, "bold"))
        vk.pack(side=tk.LEFT, padx=(4, 10), pady=6)
        self._bg_widgets.append((vk, "CARD"))
        self._fg_widgets.append((vk, clr_key))
        return vk

    def _mkbtn(self, parent, text, cmd, clr_key):
        T = self._T
        b = tk.Button(parent, text=text, command=cmd,
                      bg=T[clr_key], fg=T["BADGE_FG"],
                      font=("Segoe UI", 8, "bold"), relief="flat",
                      padx=12, pady=6, cursor="hand2", bd=0,
                      activebackground=T[clr_key],
                      activeforeground=T["BADGE_FG"])
        b.pack(side=tk.LEFT, padx=3)
        b.bind("<Enter>", lambda e, k=clr_key: b.configure(bg=self._lighten(self.T(k))))
        b.bind("<Leave>", lambda e, k=clr_key: b.configure(bg=self.T(k)))
        self._btn_meta.append((b, clr_key))
        return b

    def _sec(self, parent, text, row):
        T = self._T
        lbl = tk.Label(parent, text=text, bg=T["BG"], fg=T["MUTED"],
                       font=("Segoe UI", 9, "bold"))
        lbl.grid(row=row, column=0, sticky="w", pady=(10, 2))
        self._sec_labels.append(lbl)

    def _panel(self, parent, row, sticky="ew"):
        T = self._T
        f = tk.Frame(parent, bg=T["PANEL"],
                     highlightbackground=T["BORDER"], highlightthickness=1)
        f.grid(row=row, column=0, sticky=sticky)
        self._panels.append(f)
        return f

    # =========================================================================
    # CONTROLS
    # =========================================================================
    def _load_binary(self):
        path = filedialog.askopenfilename(
            title="Select compiled C executable",
            filetypes=[("Executables", "*.exe *.out *"), ("All files", "*.*")])
        if path:
            global BINARY_NAME
            BINARY_NAME = path
            self._set_status(f"Loaded: {os.path.basename(path)}", "ACCENT")
            self.btn_start["state"] = "normal"

    def _start(self):
        if self.running:
            return
        self._reset_state()
        self.running = True
        self._set_status("Running ▶", "GREEN")
        self.btn_start["state"] = "disabled"
        self.btn_stop ["state"] = "normal"
        self.btn_next ["state"] = "normal"
        self._start_time = self.root.tk.call('clock', 'milliseconds')
        try:
            self.process = subprocess.Popen(
                [BINARY_NAME],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                stdin=subprocess.PIPE, text=True, bufsize=1, encoding="utf-8")
        except FileNotFoundError:
            self._set_status("ERROR: BINARY NOT FOUND", "RED")
            messagebox.showerror("Error",
                f"Binary not found:\n{BINARY_NAME}\n\nUse Load Binary.")
            self.running = False
            self.btn_start["state"] = "normal"
            return
        threading.Thread(target=self._reader_thread, daemon=True).start()
        self._set_status("RUNNING", "GREEN")

    def _stop(self):
        self.running = False
        if self.process:
            try: self.process.terminate()
            except: pass
        self._set_status("Stopped ■", "RED")
        self.btn_start["state"] = "normal"
        self.btn_stop ["state"] = "disabled"
        self.btn_next ["state"] = "disabled"

    def _step(self):
        if self.process and self.process.poll() is None:
            try:
                self.process.stdin.write("\n")
                self.process.stdin.flush()
                self._set_status("NEXT STEP", "ACCENT")
            except:
                pass

    def _on_close(self):
        self._stop()
        self.root.destroy()

    # =========================================================================
    # READER THREAD
    # =========================================================================
    def _reader_thread(self):
        try:
            for line in self.process.stdout:
                self.out_queue.put(line.rstrip("\n"))
        except Exception:
            pass
        self.out_queue.put(None)

    # =========================================================================
    # OUTPUT PARSER — adapt regexes to match your C printf() output
    # =========================================================================
    _RE_CYCLE    = re.compile(r"[Cc]ycle\s+(\d+)", re.I)
    _RE_IF       = re.compile(r"\[IF\]\s*(.*)",  re.I)
    _RE_ID       = re.compile(r"\[ID\]\s*(.*)",  re.I)
    _RE_EX       = re.compile(r"\[EX\]\s*(.*)",  re.I)
    _RE_REG_CHG  = re.compile(
        r"R(\d+)\s*(?:←|changed to|updated[:\s*=]+|=\s*)(-?\d+)", re.I)
    _RE_REG_TABLE = re.compile(
        r"│\s*R(\d+)\s*│\s*(-?\d+)\s*│", re.I)
    _RE_PC_CHG   = re.compile(r"\bPC\s*=\s*(\d+)|\bnew PC\s*=\s*(\d+)", re.I)
    _RE_SREG_BIN = re.compile(r"SREG\s*[=:]\s*([01]{8})", re.I)
    _RE_SREG_FLG = re.compile(
        r"Z\s*=\s*([01]).*?S\s*=\s*([01]).*?N\s*=\s*([01]).*?V\s*=\s*([01]).*?C\s*=\s*([01])",
        re.I)
    _RE_MEM_CHG  = re.compile(
        r"mem\[(0x[0-9A-F]+|\d+)\]\s*(?:←|updated|[:=])\s*(\d+)", re.I)
    _RE_MEM_TABLE = re.compile(
        r"│\s*(0x[0-9A-F]+)\s*│\s*(\d+)\s*\(0x[0-9A-F]+\)", re.I)
    _RE_IMEM_CHG = re.compile(
        r"│\s*(\d+)\s*│\s*(0x[0-9A-F]+)", re.I)
    _RE_FLUSH    = re.compile(r"\bflush\b|\bflushing\b|\bIF\+ID\s+flushed\b|\btaken\b", re.I)
    _RE_STALL    = re.compile(r"\bstall\b|\bstalling\b|\bhazard\b", re.I)
    _RE_DUMP_REG = re.compile(r"^R(\d+)\s*=\s*(-?\d+)$")
    _RE_DUMP_PC  = re.compile(r"^PC\s*=\s*(\d+)$")
    _RE_DUMP_SREG= re.compile(r"^SREG\s*=\s*([01]{8})$")

    def _parse_line(self, line):
        s = line.strip()
        if not s:
            return line, "dim"

        m = self._RE_CYCLE.search(s)
        if m and "cycle" in s.lower():
            self.clock_cycle = int(m.group(1))
            return line, "cycle"

        m = self._RE_IF.search(s)
        if m: self.stage_if = m.group(1); return line, "if_tag"

        m = self._RE_ID.search(s)
        if m: self.stage_id = m.group(1); return line, "id_tag"

        m = self._RE_EX.search(s)
        if m: self.stage_ex = m.group(1); return line, "ex_tag"

        m = self._RE_REG_CHG.search(s)
        if m:
            idx = int(m.group(1)); val = int(m.group(2)) & 0xFF
            if 0 <= idx < NUM_REGS: self.regs[idx] = val
            self._log("reg", f"R{idx} ← {signed8(val)}  (0x{val:02X})")
            return line, "reg_write"

        m = self._RE_REG_TABLE.search(s)
        if m:
            idx = int(m.group(1)); val = int(m.group(2)) & 0xFF
            if 0 <= idx < NUM_REGS: self.regs[idx] = val
            return line, "dim"

        m = self._RE_PC_CHG.search(s)
        if m:
            val = m.group(1) or m.group(2)
            self.pc = int(val)
            self._log("pc", f"PC ← {self.pc}")
            return line, "id_tag"

        m = self._RE_SREG_BIN.search(s)
        if m:
            self.sreg = int(m.group(1), 2)
            self._log("sreg", f"SREG ← {m.group(1)}")
            return line, "sreg_tag"

        m = self._RE_SREG_FLG.search(s)
        if m:
            z, ss, n, v, c = (int(x) for x in m.groups())
            self.sreg = (c << 4) | (v << 3) | (n << 2) | (ss << 1) | z
            self._log("sreg", f"SREG  Z={z} S={ss} N={n} V={v} C={c}")
            return line, "sreg_tag"

        m = self._RE_MEM_CHG.search(s)
        if m:
            addr_str = m.group(1); val = int(m.group(2)) & 0xFF
            addr = int(addr_str, 16) if addr_str.startswith("0x") else int(addr_str)
            if 0 <= addr < 2048: self.data_mem[addr] = val
            self._log("mem", f"MEM[{addr}] ← {val}  (0x{val:02X})")
            return line, "mem_write"

        m = self._RE_MEM_TABLE.search(s)
        if m:
            addr = int(m.group(1), 16); val = int(m.group(2)) & 0xFF
            if 0 <= addr < 2048: self.data_mem[addr] = val
            return line, "dim"

        m = self._RE_IMEM_CHG.search(s)
        if m:
            addr = int(m.group(1)); hex_val = m.group(2)
            if 0 <= addr < 1024: self.instr_mem[addr] = hex_val
            return line, "dim"

        if self._RE_FLUSH.search(s):
            self._log("flush", s); return line, "flush_tag"

        if self._RE_STALL.search(s):
            self._log("stall", s); return line, "stall_tag"

        m = self._RE_DUMP_REG.match(s)
        if m:
            idx = int(m.group(1)); val = int(m.group(2)) & 0xFF
            if 0 <= idx < NUM_REGS: self.regs[idx] = val
            return line, "dim"

        m = self._RE_DUMP_PC.match(s)
        if m: self.pc = int(m.group(1)); return line, "dim"

        m = self._RE_DUMP_SREG.match(s)
        if m: self.sreg = int(m.group(1), 2); return line, "dim"

        return line, "dim"

    def _log(self, kind, desc):
        self.change_log.append((self.clock_cycle, kind, desc))

    def _show_success_fx(self):
        T = self._T
        w = self.root.winfo_width()
        h = self.root.winfo_height()
        
        # Full-screen overlay
        overlay = tk.Canvas(self.root, bg=T["BG"], highlightthickness=0, borderwidth=0)
        overlay.place(relx=0, rely=0, relwidth=1, relheight=1)
        
        # Draw background glitch/grid
        for i in range(0, w, 40):
            overlay.create_line(i, 0, i, h, fill=T["PANEL"], width=1)
        for i in range(0, h, 40):
            overlay.create_line(0, i, w, i, fill=T["PANEL"], width=1)

        # Main Banner
        txt = "SIMULATION COMPLETE" if not self._pink else "🌸 MISSION ACCOMPLISHED 🌸"
        # Shadow/Glow
        overlay.create_text(w//2 + 3, h//2 - 57, text=txt, 
                            fill=T["ACCENT2"], font=("Segoe UI", 45, "bold"))
        main_txt = overlay.create_text(w//2, h//2 - 60, text=txt, 
                                       fill=T["GREEN"], font=("Segoe UI", 45, "bold"))
        
        stats = f"TOTAL CYCLES: {self.clock_cycle}   •   PC REACHED: {self.pc}   •   SREG: {self.sreg:08b}"
        overlay.create_text(w//2, h//2 + 20, text=stats, 
                            fill=T["FG"], font=("Courier New", 16, "bold"))

        # Floating Binary Particles
        particles = []
        for _ in range(70):
            px, py = random.randint(0, w), random.randint(0, h)
            char = random.choice(["0", "1", "1", "0", "SYSTEM", "OK"])
            clr = random.choice([T["GREEN"], T["ACCENT"], T["PURPLE"], T["MUTED"]])
            p = overlay.create_text(px, py, text=char, fill=clr, 
                                    font=("Courier New", random.randint(9, 15), "bold"))
            particles.append({
                'id': p, 
                'vx': random.uniform(-0.5, 0.5), 
                'vy': random.uniform(1, 4),
                'base_clr': clr
            })

        # Dismiss Button
        btn_frame = tk.Frame(overlay, bg=T["GREEN"], padx=2, pady=2)
        overlay.create_window(w//2, h - 150, window=btn_frame)
        
        btn = tk.Button(btn_frame, text=" DISMISS NOTIFICATION ", 
                        bg=T["BG"], fg=T["GREEN"], font=("Segoe UI", 11, "bold"),
                        relief="flat", padx=30, pady=12, command=overlay.destroy,
                        activebackground=T["GREEN"], activeforeground=T["BG"],
                        cursor="hand2")
        btn.pack()

        def anim(step=0):
            if not overlay.winfo_exists(): return
            
            # Pulse main text
            if step % 15 == 0:
                curr = overlay.itemcget(main_txt, "fill")
                next_c = T["FG"] if curr == T["GREEN"] else T["GREEN"]
                overlay.itemconfig(main_txt, fill=next_c)

            # Update particles
            for p in particles:
                overlay.move(p['id'], p['vx'], p['vy'])
                pos = overlay.coords(p['id'])
                if pos[1] > h:
                    overlay.coords(p['id'], random.randint(0, w), -20)
                
                if random.random() < 0.02:
                    overlay.itemconfig(p['id'], fill=T["BADGE_FG"])
                elif random.random() < 0.02:
                    overlay.itemconfig(p['id'], fill=p['base_clr'])

            self.root.after(30, lambda: anim(step + 1))

        # Auto-dismiss on overlay click too
        overlay.bind("<Button-1>", lambda e: overlay.destroy())
        anim()

    # =========================================================================
    # POLL + REFRESH
    # =========================================================================
    def _poll_queue(self):
        try:
            count = 0
            while count < 400:
                line = self.out_queue.get_nowait()
                if line is None:
                    self.running = False
                    self._set_status("Finished ✓", "GREEN")
                    self._show_success_fx()
                    self.btn_start["state"] = "normal"
                    self.btn_stop ["state"] = "disabled"
                    break
                text, tag = self._parse_line(line)
                self._append_console(text, tag)
                count += 1
        except queue.Empty:
            pass
        self._refresh_ui()
        self.root.after(REFRESH_MS, self._poll_queue)

    def _refresh_ui(self):
        T = self._T

        # top pills
        self.clock_lbl.configure(text=str(self.clock_cycle))
        self.pc_pill.configure(  text=str(self.pc))
        self.sreg_pill.configure(text=f"{self.sreg:08b}")

        # pipeline stage text
        stage_map = {"IF": self.stage_if, "ID": self.stage_id, "EX": self.stage_ex}
        for s in self._stages:
            s["instr_l"].configure(text=stage_map[s["name"]] or "—")

        # SREG flag value labels
        bit_map = {"C": 4, "V": 3, "N": 2, "S": 1, "Z": 0}
        for flag, lbl in self._sreg_val_lbls.items():
            val = (self.sreg >> bit_map[flag]) & 1
            lbl.configure(text=str(val),
                          fg=T["ACCENT"] if val else T["MUTED"])

        # register rows  — (frame, idx_l, name_l, dec_l, hex_l, bin_l)
        for frame, idx_l, name_l, dec_l, hex_l, bin_l in self._reg_rows:
            v = self.regs[int(idx_l["text"])]
            dec_l.configure(text=str(signed8(v)))
            hex_l.configure(text=f"0x{v:02X}")
            bin_l.configure(text=f"{v:08b}")

        # PC + SREG special rows
        self.pc_dec_lbl.configure(  text=str(self.pc))
        self.pc_hex_lbl.configure(  text=f"0x{self.pc:04X}")
        self.sreg_dec_lbl.configure(text=str(self.sreg))
        self.sreg_bin_lbl.configure(text=f"{self.sreg:08b}")

        # instruction rows  — (frame, addr_l, bin_l)
        for i, (frame, addr_l, bin_l) in enumerate(self._instr_rows):
            bin_l.configure(text=self.instr_mem[i] or "—")

        # data rows  — (frame, addr_l, dec_l, hex_l, bin_l)
        for i, (frame, addr_l, dec_l, hex_l, bin_l) in enumerate(self._data_rows):
            v = self.data_mem[i]
            dec_l.configure(text=str(v))
            hex_l.configure(text=f"0x{v:02X}")
            bin_l.configure(text=f"{v:08b}")

        # change log — append only new entries
        new = self.change_log[self._log_shown:]
        if new:
            tag_map = {
                "reg":   "reg_change",  "mem":  "mem_change",
                "pc":    "pc_change",   "sreg": "sreg_change",
                "flush": "flush",       "stall":"stall",
            }
            self.change_text.configure(state="normal")
            for cyc, kind, desc in new:
                self.change_text.insert(
                    tk.END, f"[{cyc:>4}]  {desc}\n", tag_map.get(kind, "dim"))
            self.change_text.see(tk.END)
            self.change_text.configure(state="disabled")
            self._log_shown = len(self.change_log)

    # ── helpers ───────────────────────────────────────────────────────────────
    def _append_console(self, line, tag="dim"):
        self.console.configure(state="normal")
        self.console.insert(tk.END, line + "\n", tag)
        n = int(self.console.index("end-1c").split(".")[0])
        if n > 3000:
            self.console.delete("1.0", f"{n-3000}.0")
        self.console.see(tk.END)
        self.console.configure(state="disabled")

    def _set_status(self, text, clr_key="MUTED"):
        self.status_lbl.configure(text=text.upper(), fg=self.T(clr_key))

    def _reset_state(self):
        self.clock_cycle = 0
        self.regs        = [0] * NUM_REGS
        self.pc          = 0
        self.sreg        = 0
        self.instr_mem   = [""] * 1024
        self.data_mem    = [0]  * 2048
        self.stage_if = self.stage_id = self.stage_ex = ""
        self.change_log = []
        self._log_shown = 0
        for w in (self.console, self.change_text):
            w.configure(state="normal")
            w.delete("1.0", tk.END)
            w.configure(state="disabled")


# ── ENTRY POINT ───────────────────────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    try:
        import ctypes
        hwnd = ctypes.windll.user32.GetForegroundWindow()
        ctypes.windll.dwmapi.DwmSetWindowAttribute(
            hwnd, 20, ctypes.byref(ctypes.c_int(2)), ctypes.sizeof(ctypes.c_int))
    except Exception:
        pass
    ProcessorGUI(root)
    root.mainloop()