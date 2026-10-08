import streamlit as st
import subprocess
import os
import time
import re
import html

# Page Configuration
st.set_page_config(
    page_title="Multithreaded File Search Engine",
    layout="wide",
    initial_sidebar_state="expanded"
)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ENGINE_EXE = os.path.join(BASE_DIR, "build", "bin", "search_engine.exe")
INDEX_FILE = os.path.join(BASE_DIR, "index.bin")

# Clean, professional styling that adapts properly to Dark and Light modes
st.markdown("""
<style>
    .app-title {
        font-size: 1.85rem;
        font-weight: 700;
        letter-spacing: -0.02em;
        margin-bottom: 0.15rem;
    }
    .app-subtitle {
        color: #888888;
        font-size: 0.95rem;
        margin-bottom: 1.25rem;
    }
    .result-box {
        border: 1px solid rgba(128, 128, 128, 0.25);
        border-radius: 8px;
        padding: 16px 20px;
        margin-bottom: 14px;
        background-color: rgba(128, 128, 128, 0.04);
    }
    .result-title {
        font-size: 1.15rem;
        font-weight: 600;
        margin-bottom: 4px;
    }
    .result-path {
        font-family: Consolas, monospace;
        font-size: 0.82rem;
        color: #888888;
        margin-bottom: 8px;
        word-break: break-all;
    }
    .meta-tag {
        display: inline-block;
        font-size: 0.78rem;
        font-weight: 600;
        padding: 3px 8px;
        border-radius: 4px;
        margin-right: 6px;
        border: 1px solid rgba(128, 128, 128, 0.3);
    }
    .tag-term {
        display: inline-block;
        font-size: 0.78rem;
        padding: 2px 7px;
        border-radius: 3px;
        margin-right: 4px;
        background-color: rgba(66, 133, 244, 0.15);
        border: 1px solid rgba(66, 133, 244, 0.4);
    }
    .file-preview-box {
        background-color: #1e1e1e;
        color: #d4d4d4;
        border: 1px solid rgba(128, 128, 128, 0.25);
        border-radius: 6px;
        padding: 14px;
        font-family: Consolas, 'Courier New', monospace;
        font-size: 0.85rem;
        line-height: 1.5;
        white-space: pre-wrap;
        word-break: break-word;
        max-height: 420px;
        overflow-y: auto;
    }
    mark.match-hl {
        background-color: #eab308 !important;
        color: #000000 !important;
        font-weight: 700;
        padding: 1px 4px;
        border-radius: 2px;
    }
</style>
""", unsafe_allow_html=True)

def check_engine_built():
    return os.path.exists(ENGINE_EXE)

def clean_ansi(text):
    """Strips any residual ANSI escape codes or control characters."""
    ansi_regex = re.compile(r'\x1b\[[0-9;]*[mK]')
    return ansi_regex.sub('', text)

def run_engine_command(args):
    if not check_engine_built():
        return False, "Engine executable not found at: " + ENGINE_EXE
    try:
        cmd = [ENGINE_EXE] + args
        result = subprocess.run(
            cmd,
            cwd=BASE_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=120
        )
        raw = result.stdout + ("\n" + result.stderr if result.stderr else "")
        return True, clean_ansi(raw)
    except Exception as e:
        return False, str(e)

def parse_search_results(raw_output):
    """Parses output from search_engine.exe into clean structured results."""
    results = []
    pattern = re.compile(
        r"\[(\d+)\]\s+([^\r\n]+)\r?\n\s+Score\s+:\s+([0-9.]+)\r?\n\s+Path\s+:\s+([^\r\n]+)\r?\n\s+Size\s+:\s+(\d+)\s+bytes\s+([^\r\n]*)\r?\n\s+Matched:\s*([^\r\n]*)",
        re.MULTILINE
    )
    for match in pattern.finditer(raw_output):
        rank, filename, score, path, size, ext, matched = match.groups()
        results.append({
            "rank": int(rank),
            "filename": clean_ansi(filename).strip(),
            "score": float(score),
            "path": clean_ansi(path).strip(),
            "size": int(size),
            "extension": clean_ansi(ext).strip().lstrip("."),
            "matched_terms": [t for t in clean_ansi(matched).strip().split() if t]
        })
    return results

def parse_stats(raw_output):
    stats = {}
    doc_match = re.search(r"Documents\s*:\s*(\d+)", raw_output)
    tok_match = re.search(r"Unique tokens\s*:\s*(\d+)", raw_output)
    post_match = re.search(r"Total postings\s*:\s*(\d+)", raw_output)
    if doc_match: stats["documents"] = int(doc_match.group(1))
    if tok_match: stats["tokens"] = int(tok_match.group(1))
    if post_match: stats["postings"] = int(post_match.group(1))
    return stats

def highlight_content(raw_text, keywords):
    """HTML escapes text and highlights matched keyword tokens safely."""
    if not raw_text:
        return ""
    escaped = html.escape(raw_text)
    if not keywords:
        return escaped

    clean_kw = set()
    for k in keywords:
        k_str = str(k).strip()
        if k_str:
            clean_kw.add(k_str)

    if not clean_kw:
        return escaped

    # Sort descending by length so longer tokens take precedence over sub-tokens
    sorted_kw = sorted(clean_kw, key=len, reverse=True)
    words = [re.escape(w) for w in sorted_kw]

    # Case-insensitive substring match highlighting with high-visibility inline styling
    regex = r"(?i)(" + "|".join(words) + r")"
    return re.sub(
        regex,
        r'<mark style="background-color: #facc15; color: #000000; font-weight: 700; padding: 2px 4px; border-radius: 3px; border: 1px solid #ca8a04;">\1</mark>',
        escaped
    )

# Header
st.markdown('<div class="app-title">Multithreaded File Search Engine</div>', unsafe_allow_html=True)
st.markdown('<div class="app-subtitle">C++17 Multi-Directory Inverted Index and TF-IDF Retrieval</div>', unsafe_allow_html=True)

# Sidebar: Multiple Directory Ingestion & Controls
with st.sidebar:
    st.subheader("Indexing & Folders")

    is_built = check_engine_built()
    if is_built:
        st.caption("Status: C++ Core Ready")
    else:
        st.error("Status: C++ Core Not Found")
        if st.button("Build Engine"):
            with st.spinner("Building C++ Engine..."):
                ps_script = os.path.join(BASE_DIR, "run.ps1")
                subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-File", ps_script, "-NoInteractive"], cwd=BASE_DIR)
                st.rerun()

    st.markdown("---")
    st.markdown("**Ingest Multiple Folders**")
    st.caption("Enter directory paths below (one folder per line):")

    default_dirs = ["./data/sample", "./include"]
    dirs_input = st.text_area(
        "Folder Paths",
        value="\n".join(default_dirs),
        height=130,
        label_visibility="collapsed",
        help="Add as many folders as you like, each on a new line."
    )

    threads = st.slider("Thread Pool Workers", min_value=1, max_value=16, value=8)

    col_btn1, col_btn2 = st.columns(2)
    with col_btn1:
        btn_index = st.button("Index All Folders", use_container_width=True)
    with col_btn2:
        btn_clear = st.button("Clear Index", use_container_width=True)

target_directories = [line.strip() for line in dirs_input.splitlines() if line.strip()]

# Tabs (Concurrency benchmark removed as requested)
tab_search, tab_stats, tab_browse = st.tabs([
    "Search",
    "Index Statistics",
    "Browse Files"
])

# Handle Indexing Action
if btn_index:
    if not target_directories:
        st.sidebar.error("Provide at least one folder path.")
    else:
        index_args = []
        for d in target_directories:
            index_args.extend(["--index", d])
        index_args.extend(["--threads", str(threads), "--save-index", INDEX_FILE])

        with st.spinner(f"Indexing {len(target_directories)} folders concurrently..."):
            t0 = time.time()
            ok, out = run_engine_command(index_args)
            dt = time.time() - t0
            if ok:
                st.sidebar.success(f"Indexed {len(target_directories)} folders in {dt:.2f}s")
                st.toast(f"Saved index with {len(target_directories)} folders.")
                with st.expander("Indexing Log"):
                    st.code(out)
            else:
                st.sidebar.error(out)

# Handle Clear Index Action
if btn_clear:
    if os.path.exists(INDEX_FILE):
        os.remove(INDEX_FILE)
    st.toast("Index cleared.")
    st.rerun()

# TAB 1: SEARCH & HIGHLIGHT
with tab_search:
    col_q1, col_q2 = st.columns([5, 1])
    with col_q1:
        query = st.text_input(
            "Search Query",
            value="machine learning",
            placeholder="Enter keywords...",
            label_visibility="collapsed"
        )
    with col_q2:
        max_results = st.number_input("Max Results", min_value=1, max_value=50, value=10, label_visibility="collapsed")

    btn_search = st.button("Search", type="primary")

    if btn_search or query:
        # Extract individual search words for highlighting
        query_words = [w.strip() for w in re.split(r"[\s,;+]+", query) if len(w.strip()) > 0]

        with st.spinner("Searching..."):
            # If target_directories are specified, index them dynamically so search always has live data
            cmd_args = []
            if os.path.exists(INDEX_FILE):
                cmd_args.extend(["--load-index", INDEX_FILE])
            else:
                for d in target_directories:
                    cmd_args.extend(["--index", d])
            
            cmd_args.extend(["--search", query, "--max-results", str(max_results)])

            ok, out = run_engine_command(cmd_args)

            if not ok:
                st.error(out)
            else:
                results = parse_search_results(out)
                if not results:
                    st.info(f"No documents matched the query: '{query}'")
                    with st.expander("Output Log"):
                        st.code(out)
                else:
                    st.write(f"Found **{len(results)}** ranked document(s):")
                    for item in results:
                        highlight_words = list(set(item['matched_terms'] + query_words))
                        fname = item['filename']
                        fpath = item['path']

                        matched_html = " ".join([f'<span class="tag-term">{t}</span>' for t in item['matched_terms']])

                        st.markdown(f"""
                        <div class="result-box">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 4px;">
                                <div class="result-title">#{item['rank']} {fname}</div>
                                <div>
                                    <span class="meta-tag">Score: {item['score']:.4f}</span>
                                    <span class="meta-tag">{item['size']:,} bytes</span>
                                    <span class="meta-tag">.{item['extension']}</span>
                                </div>
                            </div>
                            <div class="result-path">{fpath}</div>
                            <div style="margin-top: 6px;">
                                <span style="font-size: 0.82rem; font-weight: 600; color: #888;">Matched Terms:</span>
                                {matched_html}
                            </div>
                        </div>
                        """, unsafe_allow_html=True)

                        # File Preview with text highlighting
                        with st.expander(f"File Preview: {fname}", expanded=True):
                            if os.path.exists(fpath):
                                try:
                                    with open(fpath, "r", encoding="utf-8", errors="replace") as f:
                                        raw_lines = f.readlines()

                                    # Compile search regex for line scanning
                                    clean_kw = [k.strip() for k in highlight_words if k.strip()]
                                    kw_pattern = None
                                    if clean_kw:
                                        sorted_kw = sorted(set(clean_kw), key=len, reverse=True)
                                        kw_pattern = re.compile(r"(?i)(" + "|".join(re.escape(w) for w in sorted_kw) + r")")

                                    # Collect matching line numbers
                                    matched_lines = []
                                    if kw_pattern:
                                        for idx, line in enumerate(raw_lines):
                                            if kw_pattern.search(line):
                                                matched_lines.append((idx + 1, line.rstrip("\r\n")))

                                    # Display match statistics and quick snippets
                                    if matched_lines:
                                        line_refs = ", ".join(f"L{num}" for num, _ in matched_lines[:12])
                                        if len(matched_lines) > 12:
                                            line_refs += f", ... (+{len(matched_lines) - 12} more lines)"
                                        st.markdown(
                                            f'<div style="font-size: 0.82rem; color: #888888; margin-bottom: 8px;">'
                                            f'<strong>Matches found on {len(matched_lines)} line(s):</strong> <code>{line_refs}</code>'
                                            f'</div>',
                                            unsafe_allow_html=True
                                        )

                                        # Quick snippets preview for the first 5 matching lines
                                        snippet_html = []
                                        for num, text in matched_lines[:5]:
                                            hl_snippet = highlight_content(text.strip(), highlight_words)
                                            snippet_html.append(
                                                f'<div style="margin: 3px 0;">'
                                                f'<span style="color: #888888; font-weight: 600; margin-right: 8px; user-select: none;">Line {num:4d}:</span>'
                                                f'<span>{hl_snippet}</span>'
                                                f'</div>'
                                            )
                                        st.markdown(
                                            f'<div style="background-color: rgba(128, 128, 128, 0.08); border-left: 3px solid #facc15; padding: 10px 14px; margin-bottom: 12px; border-radius: 0 4px 4px 0; font-family: Consolas, monospace; font-size: 0.83rem;">'
                                            + "".join(snippet_html)
                                            + '</div>',
                                            unsafe_allow_html=True
                                        )

                                    # Full content preview with line numbers to preserve formatting & line breaks
                                    preview_slice = raw_lines[:500]
                                    formatted_rows = []
                                    for line_idx, line_str in enumerate(preview_slice, start=1):
                                        hl_line = highlight_content(line_str.rstrip("\r\n"), highlight_words)
                                        # Use a non-breaking space if empty so the line height is maintained
                                        line_display = hl_line if hl_line.strip() else "&nbsp;"
                                        formatted_rows.append(
                                            f'<div style="display: flex; line-height: 1.5; min-height: 1.4em;">'
                                            f'<span style="width: 46px; flex-shrink: 0; color: #666666; user-select: none; text-align: right; padding-right: 14px; font-variant-numeric: tabular-nums;">{line_idx}</span>'
                                            f'<span style="white-space: pre-wrap; word-break: break-word; flex-grow: 1;">{line_display}</span>'
                                            f'</div>'
                                        )

                                    if len(raw_lines) > 500:
                                        formatted_rows.append(
                                            f'<div style="color: #888888; font-style: italic; padding: 8px 0 0 50px;">'
                                            f'... [Showing first 500 of {len(raw_lines)} lines] ...'
                                            f'</div>'
                                        )

                                    st.markdown(
                                        f'<div style="background-color: #1e1e1e; color: #d4d4d4; padding: 12px 14px; border-radius: 6px; font-family: Consolas, \'Courier New\', monospace; font-size: 0.84rem; max-height: 460px; overflow-y: auto; border: 1px solid rgba(128, 128, 128, 0.25);">'
                                        + "".join(formatted_rows)
                                        + '</div>',
                                        unsafe_allow_html=True
                                    )
                                except Exception as e:
                                    st.error(f"Cannot read file: {e}")
                            else:
                                st.warning(f"File path does not exist on disk: {fpath}")

# TAB 2: INDEX STATISTICS
with tab_stats:
    st.subheader("Index Statistics")
    args = ["--load-index", INDEX_FILE, "--stats"] if os.path.exists(INDEX_FILE) else ["--stats"]
    if not os.path.exists(INDEX_FILE):
        for d in target_directories:
            args.extend(["--index", d])

    ok, out = run_engine_command(args)
    if ok:
        stats = parse_stats(out)
        c1, c2, c3 = st.columns(3)
        with c1:
            st.metric("Total Documents", stats.get("documents", 0))
        with c2:
            st.metric("Unique Keywords", stats.get("tokens", 0))
        with c3:
            st.metric("Total Postings", stats.get("postings", 0))

        st.markdown("---")
        st.markdown("**Currently Ingested Directories:**")
        for d in target_directories:
            st.markdown(f"- `{d}`")

        with st.expander("Console Output"):
            st.code(out)
    else:
        st.info("No index available. Click 'Index All Folders' in the sidebar.")

# TAB 3: BROWSE FILES
with tab_browse:
    st.subheader("Inspect Files")
    chosen_folder = st.selectbox("Select Directory", target_directories)
    if os.path.exists(chosen_folder):
        entries = [e for e in os.listdir(chosen_folder) if os.path.isfile(os.path.join(chosen_folder, e))]
        if not entries:
            st.info(f"No files found in '{chosen_folder}'.")
        else:
            for fname in entries:
                fpath = os.path.join(chosen_folder, fname)
                fsize = os.path.getsize(fpath)
                with st.expander(f"{fname} ({fsize:,} bytes)"):
                    try:
                        with open(fpath, "r", encoding="utf-8", errors="replace") as f:
                            content = f.read(5000)
                            st.code(content, language="cpp" if fname.endswith((".cpp", ".h")) else "markdown")
                    except Exception as e:
                        st.error(f"Cannot read file: {e}")
    else:
        st.warning(f"Directory `{chosen_folder}` does not exist on disk.")
