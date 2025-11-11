import createMSTModule from './wasm/graph_mst.js';

const svgGraph = d3.select('#graph');
const svgMST = d3.select('#mst');
const width = +svgGraph.attr('width');
const height = +svgGraph.attr('height');

let Module = null;
let manager = null;
const statusEl = () => document.getElementById('status');

function showStatus(msg, ok = true) {
	const el = statusEl();
	if (!el) return;
	el.textContent = msg;
	el.className = 'status show ' + (ok ? 'ok' : 'err');
}

const state = {
	nodes: [],       // {id}
	edges: [],       // {u,v,w}
	mstEdges: [],
};

function updateStats() {
	if (!manager) return;
	const s = manager.getStats();
	const lines = [
		`Algorithm: ${s.algorithm}`,
		`Nodes: ${s.numNodes}`,
		`Edges: ${s.numEdges}`,
		`MST edges: ${s.numMSTEdges}`,
		`MST cost: ${s.mstCost.toFixed(2)}`,
	];
	document.getElementById('stats').textContent = lines.join('\n');
}

// Single simulation (graph positions drive both panels)
const simGraph = d3.forceSimulation()
	.force('link', d3.forceLink().id(d => d.id).distance(80))
	.force('charge', d3.forceManyBody().strength(-200))
	.force('center', d3.forceCenter(width / 2, height / 2));

// Graph SVG groups (left panel - full graph)
const gGraphLinks = svgGraph.append('g').attr('class', 'links');
const gGraphNodes = svgGraph.append('g').attr('class', 'nodes');
const gGraphLabels = svgGraph.append('g').attr('class', 'node-labels');
const gGraphWeights = svgGraph.append('g').attr('class', 'weight-labels');

// MST SVG groups (right panel - MST only)
const gMSTLinks = svgMST.append('g').attr('class', 'links');
const gMSTNodes = svgMST.append('g').attr('class', 'nodes');
const gMSTLabels = svgMST.append('g').attr('class', 'node-labels');
const gMSTWeights = svgMST.append('g').attr('class', 'weight-labels');

function render() {
	const mstKey = new Set(state.mstEdges.map(e => `${Math.min(e.u,e.v)}-${Math.max(e.u,e.v)}`));

	// ===== LEFT PANEL: Full Graph =====
	// Properly remove all edges first, then rebind
	const linkSelGraph = gGraphLinks.selectAll('line.edge')
		.data(state.edges, d => `${Math.min(d.u,d.v)}-${Math.max(d.u,d.v)}`);
	
	// Exit selection - remove edges not in data
	linkSelGraph.exit().transition().duration(0).remove();
	
	// Enter selection - add new edges
	const linkEnterGraph = linkSelGraph.enter().append('line').attr('class', 'edge');
	
	// Merge and update
	const linksMergedGraph = linkEnterGraph.merge(linkSelGraph)
		.classed('mst', d => mstKey.has(`${Math.min(d.u,d.v)}-${Math.max(d.u,d.v)}`));

	const nodesSelGraph = gGraphNodes.selectAll('circle.node')
		.data(state.nodes, d => d.id);
	nodesSelGraph.exit().remove();
	const nodesEnterGraph = nodesSelGraph.enter().append('circle').attr('class', 'node').attr('r', 8).call(drag(simGraph));
	const nodesMergedGraph = nodesEnterGraph.merge(nodesSelGraph);

	const labelsSelGraph = gGraphLabels.selectAll('text.label')
		.data(state.nodes, d => `label-${d.id}`);
	labelsSelGraph.exit().remove();
	const labelsEnterGraph = labelsSelGraph.enter().append('text').attr('class', 'label').attr('dy', -12).text(d => d.id + 1);
	const labelsMergedGraph = labelsEnterGraph.merge(labelsSelGraph);

	// Weight labels for graph
	const weightSelGraph = gGraphWeights.selectAll('text.weight-label')
		.data(state.edges, d => `weight-${Math.min(d.u,d.v)}-${Math.max(d.u,d.v)}`);
	
	// Exit selection - remove weights not in data
	weightSelGraph.exit().transition().duration(0).remove();
	
	// Enter selection - add new weights
	const weightEnterGraph = weightSelGraph.enter().append('text').attr('class', 'weight-label');
	
	// Merge and update
	const weightsMergedGraph = weightEnterGraph.merge(weightSelGraph)
		.text(d => d.w.toFixed(1));

	simGraph.nodes(state.nodes).on('tick', () => {
		linksMergedGraph
			.attr('x1', d => getNode(d.u, 'graph').x)
			.attr('y1', d => getNode(d.u, 'graph').y)
			.attr('x2', d => getNode(d.v, 'graph').x)
			.attr('y2', d => getNode(d.v, 'graph').y);

		nodesMergedGraph.attr('cx', d => d.x).attr('cy', d => d.y);
		labelsMergedGraph.attr('x', d => d.x).attr('y', d => d.y).text(d => d.id + 1);

		weightsMergedGraph
			.attr('x', d => {
				const uNode = getNode(d.u, 'graph');
				const vNode = getNode(d.v, 'graph');
				return (uNode.x + vNode.x) / 2;
			})
			.attr('y', d => {
				const uNode = getNode(d.u, 'graph');
				const vNode = getNode(d.v, 'graph');
				return (uNode.y + vNode.y) / 2;
			});

		// Update MST panel to mirror graph positions
		gMSTLinks.selectAll('line.edge')
			.attr('x1', d => getNode(d.u, 'graph').x)
			.attr('y1', d => getNode(d.u, 'graph').y)
			.attr('x2', d => getNode(d.v, 'graph').x)
			.attr('y2', d => getNode(d.v, 'graph').y);

		gMSTNodes.selectAll('circle.node').attr('cx', d => d.x).attr('cy', d => d.y);
		gMSTLabels.selectAll('text.label').attr('x', d => d.x).attr('y', d => d.y).text(d => d.id + 1);

		gMSTWeights.selectAll('text.weight-label')
			.attr('x', d => {
				const uNode = getNode(d.u, 'graph');
				const vNode = getNode(d.v, 'graph');
				return (uNode.x + vNode.x) / 2;
			})
			.attr('y', d => {
				const uNode = getNode(d.u, 'graph');
				const vNode = getNode(d.v, 'graph');
				return (uNode.y + vNode.y) / 2;
			});
	});

	simGraph.force('link').links(state.edges.map(e => ({ source: e.u, target: e.v }))).distance(80);
	simGraph.alpha(0.8).restart();

	// ===== RIGHT PANEL: MST Only (mirror graph layout) =====
	const linkSelMST = gMSTLinks.selectAll('line.edge')
		.data(state.mstEdges, d => `${Math.min(d.u,d.v)}-${Math.max(d.u,d.v)}`);
	
	// Exit selection - remove MST edges not in data
	linkSelMST.exit().transition().duration(0).remove();
	
	// Enter selection - add new MST edges
	const linkEnterMST = linkSelMST.enter().append('line').attr('class', 'edge mst');
	
	// Merge and update
	const linksMergedMST = linkEnterMST.merge(linkSelMST);

	const nodesSelMST = gMSTNodes.selectAll('circle.node')
		.data(state.nodes, d => d.id);
	nodesSelMST.exit().remove();
	const nodesEnterMST = nodesSelMST.enter().append('circle').attr('class', 'node').attr('r', 8);
	const nodesMergedMST = nodesEnterMST.merge(nodesSelMST);

	const labelsSelMST = gMSTLabels.selectAll('text.label')
		.data(state.nodes, d => d.id);
	labelsSelMST.exit().remove();
	const labelsEnterMST = labelsSelMST.enter().append('text').attr('class', 'label').attr('dy', -12).text(d => d.id + 1);
	const labelsMergedMST = labelsEnterMST.merge(labelsSelMST);

	// Weight labels for MST
	const weightSelMST = gMSTWeights.selectAll('text.weight-label')
		.data(state.mstEdges, d => `mst-weight-${Math.min(d.u,d.v)}-${Math.max(d.u,d.v)}`);
	
	// Exit selection - remove weights not in data
	weightSelMST.exit().transition().duration(0).remove();
	
	// Enter selection - add new weights
	const weightEnterMST = weightSelMST.enter().append('text').attr('class', 'weight-label');
	
	// Merge and update
	const weightsMergedMST = weightEnterMST.merge(weightSelMST)
		.text(d => d.w.toFixed(1));

	// Initial positions for MST panel
	linksMergedMST
		.attr('x1', d => getNode(d.u, 'graph').x)
		.attr('y1', d => getNode(d.u, 'graph').y)
		.attr('x2', d => getNode(d.v, 'graph').x)
		.attr('y2', d => getNode(d.v, 'graph').y);

	nodesMergedMST
		.attr('cx', d => d.x)
		.attr('cy', d => d.y);

	labelsMergedMST
		.attr('x', d => d.x)
		.attr('y', d => d.y)
		.text(d => d.id + 1);

	weightsMergedMST
		.attr('x', d => {
			const uNode = getNode(d.u, 'graph');
			const vNode = getNode(d.v, 'graph');
			return (uNode.x + vNode.x) / 2;
		})
		.attr('y', d => {
			const uNode = getNode(d.u, 'graph');
			const vNode = getNode(d.v, 'graph');
			return (uNode.y + vNode.y) / 2;
		});

	updateStats();
}

function getNode(id, panel = 'graph') {
	// Find or create node - share same node object but track positions separately
	let n = state.nodes.find(n => n.id === id);
	if (!n) {
		n = { id, x: Math.random() * width, y: Math.random() * height };
		state.nodes.push(n);
	}
	// For MST panel, use separate position tracking
	if (panel === 'mst') {
		if (!n.mstX) n.mstX = Math.random() * width;
		if (!n.mstY) n.mstY = Math.random() * height;
		return { id: n.id, x: n.mstX, y: n.mstY };
	}
	return n;
}

function drag(simulation) {
	function dragstarted(event, d) {
		if (!event.active) simulation.alphaTarget(0.3).restart();
		d.fx = d.x; d.fy = d.y;
	}
	function dragged(event, d) { d.fx = event.x; d.fy = event.y; }
	function dragended(event, d) { if (!event.active) simulation.alphaTarget(0); d.fx = null; d.fy = null; }
	return d3.drag().on('start', dragstarted).on('drag', dragged).on('end', dragended);
}

function syncFromManager() {
	if (!manager) { showStatus('WASM not ready. Please wait...', false); return; }
	// Sync nodes
	const stats = manager.getStats();
	const n = stats.numNodes;
	for (let i = 0; i < n; i++) {
		let node = state.nodes.find(n => n.id === i);
		if (!node) {
			node = { id: i, x: Math.random() * width, y: Math.random() * height };
			node.mstX = Math.random() * width;
			node.mstY = Math.random() * height;
			state.nodes.push(node);
		}
	}
	// Remove nodes that no longer exist
	state.nodes = state.nodes.filter(n => n.id < stats.numNodes);

	// Sync edges - clear first to ensure proper update
	const edges = manager.getGraphEdges();
	state.edges = [];
	for (let i = 0; i < edges.size(); i++) {
		const e = edges.get(i);
		state.edges.push({ u: e.u, v: e.v, w: e.w });
	}
	
	const mst = manager.getMSTEdges();
	state.mstEdges = [];
	for (let i = 0; i < mst.size(); i++) {
		const e = mst.get(i);
		state.mstEdges.push({ u: e.u, v: e.v, w: e.w });
	}
	
	// Force re-render
	render();
}

function parseIntVal(id) { return parseInt(document.getElementById(id).value, 10); }
function parseFloatVal(id) { return parseFloat(document.getElementById(id).value); }

function bindUI() {
	document.getElementById('btnAddNode').addEventListener('click', () => {
		if (!manager) { showStatus('Module not loaded.', false); return; }
		const id = manager.addNode();
		getNode(id);
		syncFromManager();
		showStatus(`Node ${id + 1} added.`, true);
	});

	document.getElementById('btnDelNode').addEventListener('click', () => {
		if (!manager) { showStatus('Module not loaded.', false); return; }
		const nodeId = parseIntVal('nodeId');
		if (Number.isNaN(nodeId)) { showStatus('Enter valid node ID.', false); return; }
		if (nodeId < 1) { showStatus('Node ID must start from 1.', false); return; }
		
		const stats = manager.getStats();
		if (nodeId > stats.numNodes) {
			showStatus(`Node ${nodeId} does not exist.`, false);
			return;
		}
		
		// Convert 1-based to 0-based
		const nodeId0 = nodeId - 1;

		// Adjust frontend node list to preserve positions and avoid large layout jumps
		// Backend shifts node IDs down after deletion; replicate that change locally
		for (let i = state.nodes.length - 1; i >= 0; --i) {
			const n = state.nodes[i];
			if (n.id === nodeId0) {
				// remove the deleted node but keep its position available for nearby nodes
				state.nodes.splice(i, 1);
			} else if (n.id > nodeId0) {
				// shift IDs down to match backend reindexing
				n.id = n.id - 1;
			}
		}

		// Now call backend delete (IDs already adjusted locally)
		manager.deleteNode(nodeId0);

		// Clear input field
		document.getElementById('nodeId').value = '';

		// Refresh state from manager and re-render (this will reuse positions kept above)
		syncFromManager();
		showStatus(`Node ${nodeId} and all its edges deleted.`, true);
	});

	document.getElementById('btnAddEdge').addEventListener('click', () => {
		if (!manager) { showStatus('Module not loaded.', false); return; }
		const u = parseIntVal('edgeU');
		const v = parseIntVal('edgeV');
		const w = parseFloatVal('edgeW');
		if (Number.isNaN(u) || Number.isNaN(v) || Number.isNaN(w)) { showStatus('Enter valid u, v, weight.', false); return; }
		if (u < 1 || v < 1) { showStatus('Nodes must start from 1.', false); return; }
		// Convert 1-based input to 0-based for backend
		manager.addEdge(u - 1, v - 1, w);
		syncFromManager();
	});

	document.getElementById('btnDelEdge').addEventListener('click', () => {
		if (!manager) { showStatus('Module not loaded.', false); return; }
		const u = parseIntVal('edgeU');
		const v = parseIntVal('edgeV');
		if (Number.isNaN(u) || Number.isNaN(v)) { showStatus('Enter valid u and v.', false); return; }
		if (u < 1 || v < 1) { showStatus('Nodes must start from 1.', false); return; }
		// Convert 1-based input to 0-based for backend
		const u0 = u - 1;
		const v0 = v - 1;
		
		// Check if edge exists before deletion
		const edges = manager.getGraphEdges();
		let edgeExists = false;
		for (let i = 0; i < edges.size(); i++) {
			const e = edges.get(i);
			if ((e.u === u0 && e.v === v0) || (e.u === v0 && e.v === u0)) {
				edgeExists = true;
				break;
			}
		}
		
		if (!edgeExists) {
			showStatus(`Edge between nodes ${u} and ${v} does not exist.`, false);
			return;
		}
		
		manager.deleteEdge(u0, v0);
		// Clear input fields
		document.getElementById('edgeU').value = '';
		document.getElementById('edgeV').value = '';
		syncFromManager();
		showStatus(`Edge between nodes ${u} and ${v} deleted.`, true);
	});

	document.getElementById('btnReset').addEventListener('click', () => {
		if (!manager) { showStatus('Module not loaded.', false); return; }
		state.nodes = []; state.edges = []; state.mstEdges = [];
		manager.reset(0);
		syncFromManager();
	});

	document.getElementById('btnRebuild').addEventListener('click', () => {
		if (!manager) { showStatus('Module not loaded.', false); return; }
		const algo = document.getElementById('algo').value;
		manager.setAlgorithm(algo);
		const t0 = performance.now();
		manager.buildInitialMST(algo);
		const t1 = performance.now();
		document.getElementById('stats').textContent += `\nInitial build time: ${(t1 - t0).toFixed(2)} ms`;
		syncFromManager();
	});
}

async function init() {
	try {
		Module = await createMSTModule();
		const MSTManager = Module.MSTManager;
		manager = new MSTManager(0);
		showStatus('WASM loaded.', true);
		bindingReady();
	} catch (e) {
		console.error('WASM load error', e);
		showStatus('Failed to load WebAssembly. Check console.', false);
	}
}

function bindingReady() {
	bindUI();
	// Initial render
	syncFromManager();
}

init();


