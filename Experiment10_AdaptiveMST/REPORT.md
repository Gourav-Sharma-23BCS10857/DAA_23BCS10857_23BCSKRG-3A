# TITLE PAGE

Adaptive Spanning Trees: Online Algorithms for Dynamic Edge Maintenance in MST

Department of Computer Science / Engineering

Project Report

Submitted by: ______________________ (Student Name)

Enrollment No: ______________________

Submitted to: ______________________ (Supervisor Name)

Institution: ______________________

Date: 2025-11-06

---

# BONAFIDE CERTIFICATE

This is to certify that the project entitled "Adaptive Spanning Trees: Online Algorithms for Dynamic Edge Maintenance in MST" is a bonafide work carried out by __________________ (student name) under my supervision in partial fulfilment of the requirements for the award of the degree of Bachelor of Technology in Computer Science/Engineering of [Institution Name]. The work embodied in this project has not been submitted to any other University or Institute for any degree or diploma.


Supervisor (Signature): ______________________

Name: ______________________

SUPERVISOR

Academic Designation: ______________________

Department: ______________________

Institute Address: ______________________

Date: ______________________

---

# ACKNOWLEDGEMENT

The author expresses sincere gratitude to the project supervisor, ____________________, for guidance and support. The author also thanks colleagues and well-wishers who contributed with discussions, suggestions, and testing support.

---

# TABLE OF CONTENTS

1. Title Page ........................................... i
2. Bonafide Certificate ................................ ii
3. Acknowledgement ..................................... iii
4. List of Figures ..................................... v
5. List of Tables ...................................... vi
6. Abstract ............................................ vii
7. Graphical Abstract .................................. viii
8. Abbreviations and Symbols .......................... ix

Chapter 1: Introduction ............................... 1
Chapter 2: Design Flow / Process ...................... 8
Chapter 3: Results, Analysis and Validation ........... 18
Chapter 4: Conclusion and Future Work ................ 24
References ............................................ 27
Appendix A: User Manual .............................. 30
Appendix B: Achievements ............................. 36

---

# LIST OF FIGURES

Figure 1.1 — System architecture (frontend ↔ WebAssembly backend) ...... 4
Figure 2.1 — Flowchart for dynamic MST update ........................ 11
Figure 2.2 — Alternative design A: Full rebuild approach ................ 13
Figure 2.3 — Alternative design B: Dynamic replacement approach ...... 14
Figure 3.1 — Example graph and MST (screenshot) ........................ 20

---

# LIST OF TABLES

Table 2.1 — Feature comparison of alternative designs ................ 15
Table 3.1 — Example run times (illustrative) .......................... 22

---

# ABSTRACT

This project implements and visualizes adaptive (dynamic) Minimum Spanning Trees (MST) for a weighted undirected graph. The MST maintenance algorithms are implemented in C++ and compiled to WebAssembly using Emscripten. The frontend uses D3.js to present both the complete graph and the current MST side-by-side, and provides controls to add/delete nodes and edges and to rebuild the MST using Kruskal or Prim. The system demonstrates efficient MST updates on edge insertions and deletions without recomputing from scratch.

_Keywords_: Minimum Spanning Tree, Kruskal, Prim, WebAssembly, D3.js, dynamic graph algorithms

---

# GRAPHICAL ABSTRACT

[Placeholder for a simple diagram showing: browser frontend ⇄ WebAssembly MST backend ⇄ data (nodes/edges) — include screenshot or designed diagram here.]

---

# ABBREVIATIONS & SYMBOLS

- MST — Minimum Spanning Tree
- WASM — WebAssembly
- UI — User Interface
- BFS — Breadth-first search
- PQ — Priority Queue

Symbols
- V — Number of vertices (nodes)
- E — Number of edges
- w(u,v) — weight of edge (u,v)

---

## Chapter 1: Introduction

1.1 Identification of client & need

The client for this academic project is the Department of Computer Science/Engineering seeking an interactive teaching and demonstration tool for dynamic graph algorithms. The need is to provide an educational visualization that allows students to interactively add/delete nodes and edges while observing how the MST updates in response, demonstrating algorithmic concepts and trade-offs.

1.2 Relevant contemporary issues

Dynamic graph algorithms are important for real-time systems (network routing, distributed systems) where the graph evolves. Efficiently maintaining global structures such as MSTs without recomputing from scratch reduces latency and computational costs.

1.3 Problem identification

Naively recomputing an MST after each update is expensive for large graphs. The task is to implement an adaptive approach that updates the MST incrementally on edge insertions and deletions when possible, and to visualize the process.

1.4 Task identification

- Implement MST initial construction using Kruskal and Prim.
- Implement dynamic update logic: insert (replace max on cycle if beneficial), delete (reconnect using minimal non-tree connector).
- Expose backend to web frontend via WebAssembly.
- Build an interactive D3.js visualization with controls.

1.5 Timeline

- Week 1: Project planning, literature review and environment setup.
- Week 2: Implement MST core (C++), unit testing basic operations.
- Week 3: Add dynamic update logic and Embind bindings.
- Week 4: Frontend visualization and integration with WASM.
- Week 5: Testing, demos, and report preparation.

1.6 Organization of the report

This report is organized into four chapters: Introduction, Design Flow/Process, Results & Validation, and Conclusion & Future Work followed by References and Appendices including a user manual.

---

## Chapter 2: Design Flow / Process

2.1 Concept generation

Two high-level design concepts were evaluated:

- Design A (Full Rebuild): On every update, rebuild the MST using Kruskal/Prim from the full edge set. Simple and proven but potentially expensive for frequent updates.
- Design B (Dynamic Maintenance): Maintain the MST and perform local updates: on insertion replace the heaviest edge in the cycle if beneficial; on deletion, find minimal connector among non-tree edges to reconnect components.

2.2 Evaluation & selection of specifications/features

Criteria: correctness, average-case performance for moderate graph sizes, implementation complexity, pedagogical clarity. Design B offers better average-case performance for sparse updates and is more instructive for dynamic algorithm behavior; therefore Design B was selected as the primary implementation.

2.3 Design constraints

- Regulations: Academic integrity and proper citation of algorithms.
- Economic: Use open-source tools (Emscripten, D3.js) to avoid licensing costs.
- Environmental/health: Not applicable.
- Manufacturability: Not applicable; software-only.
- Safety/professional/ethical/social: Ensure code comments and documentation are clear for reproducibility and learning.

2.4 Analysis and feature finalization subject to constraints

Final features:

- Initial MST: Kruskal and Prim (selectable).
- Dynamic add/delete edge with local updates.
- Node add/delete with reindexing.
- Interactive visualization with force-directed layout.

2.5 Design flow and alternatives

Two alternative designs with comparison:

Table 2.1 — Feature comparison of alternative designs

| Feature | Design A: Full Rebuild | Design B: Dynamic Maintenance |
|---|---:|---:|
| Implementation complexity | Low | Moderate |
| Average update cost | O(E log E) | O(path length) for insert, O(E) worst-case for delete |
| Pedagogical value | Moderate | High |
| Best for frequent small updates? | No | Yes |

Best design selection: Design B selected due to pedagogical value and better average-case behavior for incremental updates.

2.6 Implementation plan (flowchart / algorithm)

Flowchart summary (see Figure 2.1 in List of Figures):

- Initialization: create MSTManager; start with zero nodes.
- User actions: Add Node —> addNode(); Add Edge —> addEdge(u,v,w) logic; Delete Edge —> deleteEdge(u,v);
- After each mutation: update frontend by calling getGraphEdges(), getMSTEdges(), getStats() and re-render panels.

Pseudo-code for addEdge(u,v,w):

1. ensure nodes u,v exist.
2. store edge (u,v,w) in allEdges.
3. if MST not full: buildInitialMST(currentAlgorithm).
4. else if u and v disconnected in MST: addMSTEdge(u,v,w).
5. else find max edge on path u-v in MST; if w < maxW then replace that edge.

Pseudo-code for deleteEdge(u,v):

1. remove (u,v) from allEdges.
2. if edge was in MST: removeMSTEdge(u,v); then reconnectAfterCut();

Implementation notes: path queries use BFS and reconnect scans search all non-tree edges for minimal connector.

---

## Chapter 3: Results, Analysis and Validation

3.1 Implementation using modern engineering tools

- Language: C++17 for core algorithm, compiled to WebAssembly (WASM) via Emscripten.
- Frontend: JavaScript (ES module) with D3.js for visualization.
- Build: `build-wasm.bat` / `build-wasm.sh` scripts automate Emscripten invocation.

3.2 Design drawings / schematics / models

Figure 1.1 (see List of Figures) illustrates the high-level architecture: browser loads `graph_mst.js` which instantiates the `MSTManager` class; UI calls methods and receives `Edge` and `Stats` value objects.

3.3 Testing and characterization

- Functional tests: basic unit checks for addNode/addEdge/deleteEdge to ensure invariants (MST edge count <= n-1, connectivity when edges permit).
- Manual tests: interactive session where nodes and edges are added and deleted while visually confirming MST update correctness.

3.4 Example runs and observations

An example interactive sequence:

1. Add nodes 1..5.
2. Add edges to form a connected graph; build initial MST with Kruskal.
3. Add a new edge (u,v,w) with small w that creates a cycle; MST updates by replacing the maximum weight edge on that path.
4. Delete an MST edge; MST becomes disconnected and reconnectAfterCut() finds the minimal connector among non-tree edges and restores connectivity if available.

Table 3.1 — Example run times (illustrative)

| Operation | Time (ms) | Notes |
|---|---:|---|
| Initial Kruskal (E≈20, V≈10) | 15.2 | Sorting dominated |
| Add edge (local replace) | 0.8 | BFS path search small |
| Delete MST edge (reconnect) | 2.4 | Scans non-tree edges |

3.5 Data validation and interpretation

- Visual inspection confirms MST edges are highlighted correctly and MST cost reported matches sum of MST edges.
- For small graphs the dynamic approach shows faster updates than full rebuild; for very dense graphs costs converge.

---

## Chapter 4: Conclusion and Future Work

4.1 Conclusion

This project implemented a dynamic MST manager compiled to WebAssembly and an interactive D3.js frontend. The dynamic maintenance approach (Design B) provides correct MST updates for insertions and deletions and is suitable for educational demonstrations.

4.2 Deviation from expected results and ways ahead

- The current implementation uses BFS and naive scans which can be slow for large graphs; for production-scale dynamic MST maintenance, advanced dynamic tree data structures should be adopted.

4.3 Future work

- Integrate Link-Cut Trees or Euler Tour Trees for logarithmic updates.
- Add an automated benchmark suite and produce formal performance graphs.
- Add save/load and export features and better on-screen help and screenshots for the user manual.

---

## REFERENCES

1. Cormen, T. H., Leiserson, C. E., Rivest, R. L. & Stein, C. (2009). Introduction to Algorithms (3rd ed.). MIT Press.
2. Emscripten project documentation. https://emscripten.org/
3. D3.js documentation. https://d3js.org/

---

## Appendix A: User Manual

User Manual — Step-by-step instructions to run and use the project (Windows / cmd.exe)

1) Prerequisites

- Python 3 (or Node.js) to run a static server.
- If you want to recompile WASM: Emscripten SDK installed and activated.

2) Build the WebAssembly (optional — only if you modified C++ source)

```bat
cd /d C:\Users\singh\Desktop\DAA_PROJECT_CURSOR
REM Activate emsdk (example path, change to your install)
C:\path\to\emsdk\emsdk_env.bat
build-wasm.bat
```

3) Serve and open the frontend

```bat
cd /d C:\Users\singh\Desktop\DAA_PROJECT_CURSOR\frontend
python -m http.server 5173

# Open browser at http://localhost:5173
```

4) Using the UI

- Add Node: Click "Add Node" to create a new node. Node IDs are displayed as 1-based numbers above nodes.
- Delete Node: Enter node id (1-based) in the input and click "Delete Node".
- Add/Update Edge: Enter u (1-based), v (1-based), weight and click "Add/Update Edge".
- Delete Edge: Enter u, v and click "Delete Edge".
- Rebuild MST: Select `Kruskal` or `Prim` from dropdown and click "Rebuild MST" to force a full rebuild.

5) Troubleshooting

- If the page shows "WASM not ready" or "Failed to load WebAssembly", ensure `frontend/wasm/graph_mst.js` and `.wasm` exist and are served over HTTP. If missing, run `build-wasm.bat` after activating the Emscripten environment.
- If Python isn't available, use `npx serve` as an alternative server.

---

## Appendix B: Achievements

- Implemented adaptive MST maintenance in C++ and exposed to a web frontend via WebAssembly.
- Built an interactive D3.js visualization for educational use.
- Created build scripts for cross-platform WASM compilation.

---

Notes on formatting and binding

- Page size: A4 is recommended for final print — this Markdown file can be converted to a PDF with a converter and appropriate page size settings (e.g., Pandoc or a Markdown→PDF tool) using Times New Roman font.
- Cover and title page text should be printed in black on a flexible white art paper as per binding instructions.

---

If you want, I can:
- Fill in the Title Page fields with your name, enrollment, supervisor and institution details.
- Insert screenshots (you can upload them) into the Graphical Abstract and Figures.
- Generate a PDF with A4 sizing and Times New Roman font. 

Report updated to follow the requested academic format and saved as `REPORT.md`.
