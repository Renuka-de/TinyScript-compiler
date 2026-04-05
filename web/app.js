const source = document.getElementById('source');
const lineNumbers = document.getElementById('lineNumbers');
const editorPreview = document.getElementById('editorPreview');
const compileButton = document.getElementById('compile');
const runButton = document.getElementById('run');
const pauseButton = document.getElementById('pause');
const stepButton = document.getElementById('step');
const resetButton = document.getElementById('reset');
const statusPill = document.getElementById('statusPill');
const errorPanel = document.getElementById('errorPanel');
const errorMessage = document.getElementById('errorMessage');
const astTree = document.getElementById('astTree');
const irRaw = document.getElementById('irRaw');
const irStructured = document.getElementById('irStructured');
const rawViewBtn = document.getElementById('rawViewBtn');
const structuredViewBtn = document.getElementById('structuredViewBtn');
const currentLineValue = document.getElementById('currentLine');
const nodeCountValue = document.getElementById('nodeCount');
const irCountValue = document.getElementById('irCount');
const deviceState = document.getElementById('deviceState');
const variableState = document.getElementById('variableState');
const stepLog = document.getElementById('stepLog');

const appState = {
  ast: null,
  ir: null,
  steps: [],
  pointer: 0,
  running: false,
  intervalId: null,
  status: 'Ready',
  state: {
    devices: {},
    variables: {},
    logs: []
  }
};

const demoCode = `DEVICE fan
DEVICE led
LOG "Thermostat controller starting"
IF temperature > 30 THEN
  fan ON
  led ON
ELSE
  fan OFF
  led OFF
END`;

function setStatus(text, tone = 'neutral') {
  statusPill.textContent = text;
  statusPill.style.background = tone === 'danger' ? 'rgba(248,113,113,.15)' : tone === 'success' ? 'rgba(34,197,94,.14)' : 'rgba(255,255,255,.06)';
  statusPill.style.color = tone === 'danger' ? '#fca5a5' : tone === 'success' ? '#22c55e' : 'var(--text)';
}

function parseTinyScript(code) {
  const lines = code.split(/\r?\n/).map(line => line.replace(/;\s*$/,'').trim());
  const root = { type: 'PROGRAM', label: 'Program', children: [], line: 0 };
  const stack = [root];

  function current() { return stack[stack.length - 1]; }
  function push(node) {
    const parent = current();
    if (parent && parent.type === 'IF') {
      if (parent.inElse) parent.elseChildren.push(node);
      else parent.thenChildren.push(node);
    } else if (parent && parent.children) {
      parent.children.push(node);
    } else {
      root.children.push(node);
    }
  }

  lines.forEach((line, index) => {
    const lineNumber = index + 1;
    if (!line) return;
    const deviceMatch = line.match(/^DEVICE\s+(.+)$/i);
    const logMatch = line.match(/^LOG\s+"?(.*?"?)"?$/i);
    const ifMatch = line.match(/^IF\s+(.+)\s+THEN$/i);
    const elseMatch = /^ELSE$/i.test(line);
    const endMatch = /^END$/i.test(line);
    const loopMatch = line.match(/^LOOP\s+(\d+)\s+TIMES\s+DO$/i);
    const eventMatch = line.match(/^ON\s+(.+)\s+THEN$/i);
    const scheduleMatch = line.match(/^SCHEDULE\s+AT\s+([0-2]?\d:[0-5]\d)\s+DO$/i);
    const actionMatch = line.match(/^(\w+)\s+(ON|OFF)$/i);

    if (deviceMatch) {
      push({ type: 'DEVICE', label: `DEVICE ${deviceMatch[1]}`, value: deviceMatch[1], line: lineNumber });
      return;
    }
    if (logMatch) {
      const message = logMatch[1].replace(/^"|"$/g, '').trim();
      push({ type: 'LOG', label: `LOG "${message}"`, value: message, line: lineNumber });
      return;
    }
    if (ifMatch) {
      const condition = ifMatch[1].trim();
      const node = { type: 'IF', label: `IF ${condition}`, condition, thenChildren: [], elseChildren: [], line: lineNumber, inElse: false };
      push(node);
      stack.push(node);
      return;
    }
    if (elseMatch) {
      const node = current();
      if (node && node.type === 'IF') {
        node.inElse = true;
      }
      return;
    }
    if (endMatch) {
      const popped = stack.pop();
      if (popped && popped.type === 'IF') popped.inElse = false;
      return;
    }
    if (loopMatch) {
      const times = Number(loopMatch[1]);
      const node = { type: 'LOOP', label: `LOOP ${times} TIMES`, times, children: [], line: lineNumber };
      push(node);
      stack.push(node);
      return;
    }
    if (eventMatch) {
      const condition = eventMatch[1].trim();
      const node = { type: 'EVENT', label: `ON ${condition}`, condition, children: [], line: lineNumber };
      push(node);
      stack.push(node);
      return;
    }
    if (scheduleMatch) {
      const time = scheduleMatch[1];
      const node = { type: 'SCHEDULE', label: `SCHEDULE AT ${time}`, time, children: [], line: lineNumber };
      push(node);
      stack.push(node);
      return;
    }
    if (actionMatch) {
      const name = actionMatch[1];
      const action = actionMatch[2];
      const node = { type: 'ACTION', label: `${name} ${action}`, target: name, action, line: lineNumber };
      push(node);
      return;
    }
    push({ type: 'UNKNOWN', label: line, line: lineNumber });
  });

  return root;
}

function generateIR(ast) {
  const ir = [];

  function walk(node) {
    switch (node.type) {
      case 'PROGRAM':
        node.children.forEach(walk);
        break;
      case 'DEVICE':
        ir.push({ type: 'DEVICE', text: `DEVICE ${node.value}` });
        break;
      case 'LOG':
        ir.push({ type: 'LOG', text: `LOG "${node.value}"` });
        break;
      case 'ACTION':
        ir.push({ type: 'ACTION', text: `${node.target} ${node.action}` });
        break;
      case 'IF':
        ir.push({ type: 'IF', text: `IF ${node.condition}` });
        (node.thenChildren || []).forEach(walk);
        if ((node.elseChildren || []).length) {
          ir.push({ type: 'ELSE', text: 'ELSE' });
          node.elseChildren.forEach(walk);
        }
        ir.push({ type: 'END', text: 'END' });
        break;
      case 'LOOP':
        ir.push({ type: 'LOOP', text: `LOOP ${node.times} TIMES` });
        node.children.forEach(walk);
        ir.push({ type: 'END', text: 'END' });
        break;
      case 'EVENT':
        ir.push({ type: 'EVENT', text: `ON ${node.condition}` });
        node.children.forEach(walk);
        ir.push({ type: 'END', text: 'END' });
        break;
      case 'SCHEDULE':
        ir.push({ type: 'SCHEDULE', text: `SCHEDULE AT ${node.time}` });
        node.children.forEach(walk);
        ir.push({ type: 'END', text: 'END' });
        break;
      default:
        ir.push({ type: 'RAW', text: node.label });
    }
  }

  walk(ast);
  return ir;
}

function createSteps(ast) {
  const steps = [];
  const state = { devices: {}, variables: {}, logs: [] };

  function pushStep(title, detail, line = 1, execute = () => {}) {
    steps.push({ title, detail, line, execute });
  }

  function evalCondition(condition) {
    if (/temperature\s*>\s*30/i.test(condition)) return true;
    if (/motion\s*==\s*1/i.test(condition)) return true;
    return false;
  }

  function walk(node) {
    switch (node.type) {
      case 'DEVICE':
        pushStep('Register device', node.value, node.line, () => { state.devices[node.value] = 'OFF'; });
        break;
      case 'LOG':
        pushStep('Log message', node.value, node.line, () => { state.logs.push(`[LOG] ${node.value}`); });
        break;
      case 'ACTION':
        pushStep('Action', node.label, node.line, () => { state.devices[node.target] = node.action; state.logs.push(`[ACTION] ${node.label}`); });
        break;
      case 'IF': {
        const cond = evalCondition(node.condition);
        pushStep('Evaluate condition', node.condition, node.line, () => { state.logs.push(`[IF] ${node.condition} => ${cond}`); });
        const targetNodes = cond ? (node.thenChildren || []) : (node.elseChildren || []);
        targetNodes.forEach(walk);
        break;
      }
      case 'LOOP':
        pushStep('Enter loop', `${node.times} iterations`, () => { state.logs.push(`[LOOP] ${node.times} times`); });
        for (let i = 0; i < node.times; i += 1) {
          node.children.forEach(walk);
        }
        break;
      case 'EVENT':
        pushStep('Register event', node.condition, () => { state.logs.push(`[EVENT] ${node.condition}`); });
        node.children.forEach(walk);
        break;
      case 'SCHEDULE':
        pushStep('Register schedule', node.time, () => { state.logs.push(`[SCHEDULE] ${node.time}`); });
        node.children.forEach(walk);
        break;
      default:
        if (node.children) node.children.forEach(walk);
        break;
    }
  }

  walk(ast);
  return { steps, initial: state };
}

function renderAst(node, container) {
  container.innerHTML = '';

  function createNode(item) {
    const nodeEl = document.createElement('div');
    nodeEl.className = 'tree-node collapsed';

    const label = document.createElement('div');
    label.className = 'tree-label';
    label.innerHTML = `<strong>${item.type}</strong> <span class="tree-badge">${item.label}</span>`;
    nodeEl.appendChild(label);

    const children = document.createElement('div');
    children.className = 'tree-children';

    const childNodes = [];
    if (item.children) childNodes.push(...item.children);
    if (item.thenChildren) childNodes.push(...item.thenChildren);
    if (item.elseChildren) childNodes.push({ type: 'ELSE', label: 'ELSE', children: item.elseChildren, line: item.line });

    if (childNodes.length === 0) {
      nodeEl.classList.remove('collapsed');
      children.style.display = 'none';
    } else {
      childNodes.forEach(child => children.appendChild(createNode(child)));
    }

    label.addEventListener('click', () => {
      const isCollapsed = nodeEl.classList.toggle('collapsed');
      children.style.display = isCollapsed ? 'none' : 'block';
    });

    nodeEl.appendChild(children);
    return nodeEl;
  }

  container.appendChild(createNode(node));
}

function highlightKeywords(text) {
  return text.replace(/\b(IF|ELSE|END|LOOP|DEVICE|ON|OFF|SCHEDULE|AT|LOG)\b/g, '<span class="keyword">$1</span>');
}

function renderIr(ir) {
  irRaw.innerHTML = ir.map((item, idx) => `${idx + 1}. ${highlightKeywords(item.text)}`).join('\n');
  irStructured.innerHTML = ir.map((item, index) => {
    return `<div class="ir-structure-item"><h4>${item.type}</h4><p>${item.text}</p><p class="muted">Step ${index + 1}</p></div>`;
  }).join('');
}

function renderState() {
  deviceState.innerHTML = Object.keys(appState.state.devices).length === 0
    ? '<div class="state-item"><span>No devices yet</span></div>'
    : Object.entries(appState.state.devices).map(([name, value]) => `<div class="state-item"><span>${name}</span><strong>${value}</strong></div>`).join('');

  variableState.innerHTML = Object.keys(appState.state.variables).length === 0
    ? '<div class="state-item"><span>No variables yet</span></div>'
    : Object.entries(appState.state.variables).map(([name, value]) => `<div class="state-item"><span>${name}</span><strong>${value}</strong></div>`).join('');

  stepLog.innerHTML = appState.state.logs.map((entry, index) => `
    <div class="log-entry"><strong>Step ${index + 1}</strong><span>${entry}</span></div>
  `).join('');
}

function renderSummary() {
  nodeCountValue.textContent = countAstNodes(appState.ast || {});
  irCountValue.textContent = appState.ir ? appState.ir.length : 0;
}

function countAstNodes(node) {
  if (!node) return 0;
  let count = 1;
  if (node.children) count += node.children.reduce((sum, child) => sum + countAstNodes(child), 0);
  if (node.elseChildren) count += node.elseChildren.reduce((sum, child) => sum + countAstNodes(child), 0);
  return count;
}

function updateLineNumbers() {
  const lines = source.value.split(/\r?\n/);
  lineNumbers.innerHTML = lines.map((_, index) => `<div class="line-number">${index + 1}</div>`).join('');
}

source.addEventListener('scroll', () => {
  editorPreview.scrollTop = source.scrollTop;
  lineNumbers.scrollTop = source.scrollTop;
});

function highlightEditorLine(lineNumber) {
  const lines = source.value.split(/\r?\n/);
  editorPreview.innerHTML = lines.map((text, index) => {
    const lineIndex = index + 1;
    const escaped = text.replace(/</g, '&lt;').replace(/>/g, '&gt;');
    if (lineIndex === lineNumber) {
      return `<div class="preview-line preview-highlight">${escaped || ' '}</div>`;
    }
    return `<div class="preview-line">${escaped || ' '}</div>`;
  }).join('');
}

function syncPreview() {
  updateLineNumbers();
  highlightEditorLine(appState.pointerLine || 1);
}

function compile() {
  const code = source.value;
  if (!code.trim()) {
    showError('TinyScript code is required to compile.');
    return;
  }
  clearError();
  appState.ast = parseTinyScript(code);
  appState.ir = generateIR(appState.ast);
  const execution = createSteps(appState.ast);
  appState.steps = execution.steps;
  appState.pointer = 0;
  appState.state = { devices: {}, variables: {}, logs: [] };
  renderAst(appState.ast, astTree);
  renderIr(appState.ir);
  renderState();
  renderSummary();
  setStatus('Compiled', 'success');
  appState.pointerLine = 1;
  syncPreview();
}

function showError(message) {
  errorPanel.hidden = false;
  errorMessage.textContent = message;
  setStatus('Error', 'danger');
}

function clearError() {
  errorPanel.hidden = true;
  errorMessage.textContent = '';
}

function resetExecution() {
  if (appState.intervalId) {
    clearInterval(appState.intervalId);
    appState.intervalId = null;
  }
  appState.running = false;
  appState.pointer = 0;
  appState.state = { devices: {}, variables: {}, logs: [] };
  setStatus('Ready');
  renderState();
  stepLog.innerHTML = '';
  currentLineValue.textContent = 1;
  appState.pointerLine = 1;
  syncPreview();
}

function applyStep() {
  if (appState.pointer >= appState.steps.length) {
    setStatus('Completed', 'success');
    return;
  }

  const step = appState.steps[appState.pointer];
  step.execute();
  appState.state.logs.push(`[STEP] ${step.title}: ${step.detail}`);
  appState.pointer += 1;
  currentLineValue.textContent = appState.pointer;
  renderState();
  setStatus(`Executing: ${step.title}`);
  appState.pointerLine = step.line || 1;
  syncPreview();
}

function runExecution() {
  if (appState.running) return;
  if (appState.pointer >= appState.steps.length) {
    appState.pointer = 0;
  }
  appState.running = true;
  setStatus('Running');
  appState.intervalId = setInterval(() => {
    if (appState.pointer >= appState.steps.length) {
      clearInterval(appState.intervalId);
      appState.intervalId = null;
      appState.running = false;
      setStatus('Completed', 'success');
      return;
    }
    applyStep();
  }, 800);
}

function pauseExecution() {
  if (appState.intervalId) {
    clearInterval(appState.intervalId);
    appState.intervalId = null;
  }
  appState.running = false;
  setStatus('Paused');
}

rawViewBtn.addEventListener('click', () => {
  rawViewBtn.classList.add('active');
  structuredViewBtn.classList.remove('active');
  irRaw.classList.remove('hidden');
  irStructured.classList.add('hidden');
});

structuredViewBtn.addEventListener('click', () => {
  rawViewBtn.classList.remove('active');
  structuredViewBtn.classList.add('active');
  irRaw.classList.add('hidden');
  irStructured.classList.remove('hidden');
});

compileButton.addEventListener('click', compile);
runButton.addEventListener('click', () => {
  if (!appState.ir) compile();
  runExecution();
});
pauseButton.addEventListener('click', pauseExecution);
stepButton.addEventListener('click', () => {
  if (!appState.ir) compile();
  applyStep();
});
resetButton.addEventListener('click', resetExecution);

source.addEventListener('input', () => {
  appState.pointerLine = 1;
  updateLineNumbers();
  syncPreview();
});

function init() {
  source.value = demoCode;
  updateLineNumbers();
  compile();
  syncPreview();
}

init();
