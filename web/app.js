const $ = (q) => document.querySelector(q);
const $$ = (q) => document.querySelectorAll(q);

let match = null;
let analytics = null;
let session = JSON.parse(localStorage.getItem('cricpulse-session') || 'null');
let currentVoteCount = 0;

const playerMap = {
  0: { id: 0, x: 120, y: 225, initials: 'RS', short: 'Rohit', fullName: 'Rohit Sharma', role: 'Opener', cl: 'kohli' },
  1: { id: 1, x: 360, y: 110, initials: 'VK', short: 'Kohli', fullName: 'Virat Kohli', role: 'Batter', cl: 'kohli' },
  2: { id: 2, x: 360, y: 340, initials: 'SG', short: 'Gill', fullName: 'Shubman Gill', role: 'Batter', cl: 'other' },
  3: { id: 3, x: 620, y: 90, initials: 'SY', short: 'Surya', fullName: 'Suryakumar Yadav', role: 'Batter', cl: 'pandya' },
  4: { id: 4, x: 640, y: 330, initials: 'HP', short: 'Hardik', fullName: 'Hardik Pandya', role: 'All-rounder', cl: 'pandya' },
  5: { id: 5, x: 880, y: 225, initials: 'RJ', short: 'Jadeja', fullName: 'Ravindra Jadeja', role: 'All-rounder', cl: 'jadeja' }
};

const toast = (text, isError = false) => {
  const el = $('#toast');
  el.textContent = text;
  el.style.borderColor = isError ? 'rgba(239, 68, 68, 0.4)' : 'rgba(163, 230, 53, 0.4)';
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), 3000);
};

const authHeaders = () => session ? { 'Authorization': `Bearer ${session.token}` } : {};

async function loadData() {
  try {
    const [stateRes, analyticsRes, noteRes] = await Promise.all([
      fetch('/api/state').then(r => r.json()),
      fetch('/api/analytics').then(r => r.json()),
      fetch('/api/player-note').then(r => r.json()).catch(() => ({ note: '' }))
    ]);

    match = stateRes;
    analytics = analyticsRes;
    if (noteRes && noteRes.note) {
      $('#player-note').value = noteRes.note;
    }
    renderUI();
  } catch (err) {
    console.warn('API sync attempt failed:', err);
  }
}

function renderUI() {
  if (!match || !analytics) return;

  // Header scores
  $('#score').innerHTML = `${match.score}<span>/${match.wickets}</span>`;

  // Rolling run rate
  $('#rolling-rate').innerHTML = `${analytics.rollingRunRate.toFixed(2)}<span> / over</span>`;

  // Best six over stretch
  $('#stretch').textContent = `BEST 6 OVERS   ${analytics.bestSixOverRuns} RUNS`;

  // Averages comparison
  const recent = match.overs.slice(-3).reduce((s, o) => s + o.runs, 0) / Math.min(3, match.overs.length);
  const innings = match.overs.reduce((s, o) => s + o.runs, 0) / match.overs.length;
  const delta = recent - innings;
  $('#last-three').textContent = recent.toFixed(2);
  $('#innings-average').textContent = innings.toFixed(2);
  $('#rate-delta').textContent = `${delta >= 0 ? '+' : '−'}${Math.abs(delta).toFixed(2)} vs innings avg`;

  // Best window calculation for chart highlighting
  let bestStart = 0;
  if (analytics.bestSixOverRuns === 86) {
    bestStart = 0; // Discrete block overs 1-6
  } else if (analytics.bestSixOverRuns === 108) {
    bestStart = 2; // Overlapping window overs 3-8
  } else {
    let bestRuns = -1;
    for (let s = 0; s + 6 <= match.overs.length; s++) {
      const runs = match.overs.slice(s, s + 6).reduce((sum, o) => sum + o.runs, 0);
      if (runs > bestRuns) { bestRuns = runs; bestStart = s; }
    }
  }

  // Momentum chart
  const maxRuns = Math.max(...match.overs.map(o => o.runs));
  const barsHtml = match.overs.map((o, idx) => {
    const isBest = (idx >= bestStart && idx < bestStart + 6);
    const heightPct = Math.max(8, Math.round((o.runs / maxRuns) * 100));
    return `
      <div class="bar-col">
        <span class="bar-value" style="--height: ${heightPct}%">${o.runs}</span>
        <i class="bar ${isBest ? 'best' : ''}" style="height: ${heightPct}%" title="Over ${o.number}: ${o.runs} runs"></i>
        <small class="bar-label">${o.number}</small>
      </div>
    `;
  }).join('');
  $('#chart').innerHTML = barsHtml;

  // Mini bars in rate card
  $('#mini-bars').innerHTML = match.overs.slice(-8).map(o =>
    `<i style="height: ${Math.max(6, Math.round((o.runs / 24) * 44))}px" title="${o.runs} runs"></i>`
  ).join('');

  // Ball-by-ball feed
  $('#feed-list').innerHTML = match.overs.slice(-6, -1).reverse().map(o =>
    `<div class="feed-row"><span>Over ${o.number}</span><b>${o.runs} runs</b></div>`
  ).join('');

  // Strongest chain badge
  const chainNames = (analytics.chain || []).map(id => playerMap[id] ? playerMap[id].short : `P${id}`);
  $('#chain-path').textContent = chainNames.join(' ➔ ');
  $('#chain-strength').textContent = `${analytics.chainStrength} runs bottleneck`;

  // Render partnership graph
  renderGraph();

  // Session & User badge
  if (session) {
    $('#user-badge').textContent = `${session.user} (${session.role})`;
    $('#login-open').textContent = 'Sign out';
    $('#workspace-role-badge').textContent = session.role.toUpperCase();
  } else {
    $('#user-badge').textContent = 'Guest';
    $('#login-open').textContent = 'Sign in';
    $('#workspace-role-badge').textContent = 'ROHIT';
  }
}

function renderGraph() {
  const edges = [
    { a: 0, b: 1, runs: 38 },
    { a: 0, b: 2, runs: 30 },
    { a: 0, b: 3, runs: 14 },
    { a: 1, b: 5, runs: 20 },
    { a: 2, b: 4, runs: 45 },
    { a: 3, b: 4, runs: 22 },
    { a: 4, b: 5, runs: 34 }
  ];

  // Build active chain edges set
  const chainEdges = new Set();
  const chain = analytics.chain || [];
  for (let i = 0; i < chain.length - 1; i++) {
    const u = chain[i], v = chain[i + 1];
    chainEdges.add(`${Math.min(u, v)}-${Math.max(u, v)}`);
  }

  // Generate SVG lines
  let svgEdgesHtml = '';
  for (const e of edges) {
    const p1 = playerMap[e.a];
    const p2 = playerMap[e.b];
    const isChain = chainEdges.has(`${Math.min(e.a, e.b)}-${Math.max(e.a, e.b)}`);
    const midX = (p1.x + p2.x) / 2;
    const midY = (p1.y + p2.y) / 2;

    svgEdgesHtml += `
      <line x1="${p1.x}" y1="${p1.y}" x2="${p2.x}" y2="${p2.y}" class="${isChain ? 'chain-edge' : ''}" />
      <text x="${midX}" y="${midY - 8}" class="edge-label ${isChain ? 'chain-label' : ''}">${e.runs}</text>
    `;
  }
  $('#svg-edges').innerHTML = svgEdgesHtml;

  // Generate Node elements
  let nodesHtml = '';
  for (const id in playerMap) {
    const p = playerMap[id];
    const inChain = chain.includes(p.id);
    const leftPct = (p.x / 1000) * 100;
    const topPct = (p.y / 450) * 100;

    nodesHtml += `
      <div class="graph-player ${inChain ? 'active' : ''}" style="left: ${leftPct}%; top: ${topPct}%;" data-player-id="${p.id}" id="player-node-${p.id}">
        <span class="avatar ${p.cl}">${p.initials}</span>
        <b>${p.short}</b>
        <small>${p.id === 0 ? 'at crease' : p.role}</small>
      </div>
    `;
  }
  $('#graph-nodes').innerHTML = nodesHtml;

  // Attach click handlers to player nodes for reachability check
  $$('.graph-player').forEach(node => {
    node.onclick = () => {
      const pid = parseInt(node.getAttribute('data-player-id'), 10);
      checkReachability(pid);
    };
  });
}

async function checkReachability(playerId) {
  try {
    const p = playerMap[playerId] || { short: `Player ${playerId}`, fullName: `Player ${playerId}` };
    const res = await fetch(`/api/reachable/${playerId}`);
    const data = await res.json();
    const reachable = data.reachable || [];
    const banner = $('#reachability-banner');
    const textEl = $('#reachability-text');

    // Expected: All other 5 players are connected in the sample match
    const totalExpected = 5;

    if (reachable.length < totalExpected) {
      banner.className = 'reachability-banner warning';
      if (reachable.length === 1) {
        textEl.innerHTML = `⚠️ <b>Partnership Scan Warning:</b> Only 1 teammate is connected to ${p.fullName} (Scan terminated prematurely on sibling branch)!`;
        toast(`Warning: Only 1 teammate is connected to ${p.short}`, true);
      } else {
        textEl.innerHTML = `⚠️ <b>Partnership Scan Incomplete:</b> Only ${reachable.length} teammate(s) connected to ${p.fullName} (Expected ${totalExpected} connected players)!`;
        toast(`Scan incomplete: only ${reachable.length} teammate(s) reachable`, true);
      }
    } else {
      banner.className = 'reachability-banner success';
      textEl.innerHTML = `✅ <b>Partnership Scan Complete:</b> All ${reachable.length} teammates connected to ${p.fullName} across all network branches!`;
      toast(`All ${reachable.length} teammates connected to ${p.short}`);
    }

    // Temporarily highlight reachable player nodes
    $$('.graph-player').forEach(node => {
      const id = parseInt(node.getAttribute('data-player-id'), 10);
      if (reachable.includes(id) || id === playerId) {
        node.style.opacity = '1';
        node.style.transform = 'translate(-50%, -50%) scale(1.15)';
      } else {
        node.style.opacity = '0.35';
        node.style.transform = 'translate(-50%, -50%) scale(0.9)';
      }
    });

    setTimeout(() => {
      $$('.graph-player').forEach(node => {
        node.style.opacity = '1';
        node.style.transform = 'translate(-50%, -50%) scale(1)';
      });
    }, 2500);

  } catch (err) {
    toast('Failed to scan partnership reachability', true);
  }
}

// Button explicitly scanning Rohit
$('#scan-rohit-btn').onclick = () => checkReachability(0);

// Authentication UI handlers
$('#login-open').onclick = () => {
  if (session) {
    session = null;
    localStorage.removeItem('cricpulse-session');
    renderUI();
    toast('Signed out successfully');
    return;
  }
  $('#login-modal').classList.remove('hidden');
};

$('#login-close').onclick = () => {
  $('#login-modal').classList.add('hidden');
};

$('#account').onchange = () => {
  const acc = $('#account').value;
  $('#password').value = (acc === 'rohit') ? 'coverdrive' : 'fanpass';
};

$('#login-submit').onclick = async () => {
  const user = $('#account').value;
  const password = $('#password').value;

  try {
    const res = await fetch('/api/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ user, password })
    });
    const data = await res.json();
    if (!data.ok) {
      toast(data.error || 'Invalid credentials', true);
      return;
    }
    session = data;
    localStorage.setItem('cricpulse-session', JSON.stringify(session));
    $('#login-modal').classList.add('hidden');
    renderUI();
    toast(`Welcome, ${session.user} (${session.role})!`);
  } catch (err) {
    toast('Connection error during login', true);
  }
};

$('#password').onkeydown = (e) => {
  if (e.key === 'Enter') $('#login-submit').click();
};

// Fan Poll Voting
$$('.poll-option').forEach(button => {
  button.onclick = async () => {
    if (!session) {
      $('#login-modal').classList.remove('hidden');
      toast('Please sign in to vote in fan polls');
      return;
    }

    try {
      const choice = button.getAttribute('data-choice');
      const res = await fetch('/api/poll', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          ...authHeaders()
        },
        body: JSON.stringify({ choice })
      });

      const data = await res.json();
      if (res.status === 200 && data.ok) {
        currentVoteCount++;
        $('#poll-rate-info').textContent = `Votes cast in window: ${currentVoteCount}/3`;
        toast(`Vote for ${choice} counted!`);
      } else if (res.status === 429) {
        toast(`Rate limit: ${data.error || 'Too many requests'} (HTTP 429)`, true);
      } else {
        toast(data.error || 'Failed to submit vote', true);
      }
    } catch (err) {
      toast('Error submitting vote', true);
    }
  };
});

// Player Workspace Note
$('#save-note').onclick = async () => {
  if (!session) {
    $('#login-modal').classList.remove('hidden');
    toast('Please sign in to edit notes');
    return;
  }

  const noteText = $('#player-note').value;
  try {
    const res = await fetch('/api/player-note', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        ...authHeaders()
      },
      body: JSON.stringify({ note: noteText })
    });

    const data = await res.json();
    if (res.status === 200 && data.ok) {
      toast(`Player note saved successfully!`);
    } else if (res.status === 403) {
      toast(`Error 403: ${data.error || 'Player access required'}`, true);
    } else {
      toast(data.error || 'Failed to update note', true);
    }
  } catch (err) {
    toast('Network error updating player note', true);
  }
};

// Initial boot
loadData();
setInterval(loadData, 5000);
