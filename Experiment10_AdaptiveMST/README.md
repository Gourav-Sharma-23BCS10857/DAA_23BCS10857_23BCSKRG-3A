## Adaptive Spanning Trees: Online Algorithms for Dynamic Edge Maintenance in MST

This project visualizes a dynamic Minimum Spanning Tree (MST) on a weighted undirected graph. It updates the MST efficiently on edge insertions and deletions without recomputing from scratch. The MST algorithms and dynamic update logic are implemented in C++ and compiled to WebAssembly (WASM) via Emscripten; the frontend uses D3.js for visualization.

### Features
- Add/delete nodes and edges
- Dynamic MST maintenance on insert/delete
- Visualize graph and MST (MST edges highlighted)
- Live MST stats (cost, edges, nodes, algorithm)
- Reset graph
- Optional comparison between Kruskal and Prim for initial MST

### Tech Stack
- Backend: C++ (compiled to WebAssembly with Emscripten + Embind)
- Frontend: HTML/CSS/JS with D3.js

---

## Getting Started

### 1) Prerequisites
- Emscripten SDK installed and activated
  - Install: see `https://emscripten.org/docs/getting_started/downloads.html`
  - After install: `emsdk activate latest` and `emsdk_env.bat` (Windows) or `source ./emsdk_env.sh` (Unix)

### 2) Build the WebAssembly

Windows (PowerShell/CMD):
```bash
build-wasm.bat
```

macOS/Linux:
```bash
bash build-wasm.sh
```

This produces `frontend/wasm/graph_mst.js` and `frontend/wasm/graph_mst.wasm`.

### 3) Run the Frontend
Use any static server (examples):
```bash
# Python 3
cd frontend
python -m http.server 5173

# Node serve
npx serve . -l 5173
```
Open `http://localhost:5173` in your browser.

---

## Project Structure

```
DAA_PROJECT_CURSOR/
  backend/
    mst.cpp
  frontend/
    index.html
    main.js
    style.css
    wasm/              # build outputs here (graph_mst.js/.wasm)
  build-wasm.bat
  build-wasm.sh
  README.md
```

---

## Implementation Notes

- Initial MST via Kruskal or Prim (selectable). Default: Kruskal.
- Dynamic insert(u,v,w): find max edge on MST path u↔v. If `w` is smaller, replace that edge. Path search is done with a BFS/DFS over the current MST (adequate for moderate N).
- Dynamic delete(u,v): if non-MST edge, just remove. If MST edge, it splits MST; we scan non-tree edges to find the minimal connector between the two components and add it.
- Complexity: insert O(V+E) worst-case for path scan; delete O(E) for replacement search. Good for visualization and moderate graph sizes.

---

## Optional: Compare Algorithms
Use the dropdown to pick Kruskal or Prim for the initial MST build. The frontend reports the runtime (ms) for that initial computation.

---

## Troubleshooting
- If `graph_mst.js` not found: ensure Emscripten is activated and the build script ran without error.
- Cross-origin issues: always serve via a local HTTP server, do not open index.html via file://.
- Windows: run `emsdk_env.bat` in the same terminal before `build-wasm.bat`.


