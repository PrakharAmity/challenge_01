// CricPulse Live Analytics Hub Client
(function () {
  'use strict';

  // API Base path resolver compatible with relative paths and reverse proxies
  function getApiBase() {
    let path = window.location.pathname;
    if (!path.endsWith('/')) {
      path = path.substring(0, path.lastIndexOf('/') + 1);
    }
    return path;
  }

  const API_BASE = getApiBase();

  let g_bootId = null;
  let g_uiRevision = null;
  let g_currentMatchData = null;
  let g_cooldownTimerInterval = null;
  let g_cooldownRemainingMs = 0;
  let g_lastCooldownFetchTime = Date.now();
  let g_selectedPollOption = 'opt-1';
  let g_activeAuthToken = null;

  // Retrieve stored token safely from localStorage
  try {
    g_activeAuthToken = localStorage.getItem('cricpulse_player_token');
  } catch (e) {
    g_activeAuthToken = null;
  }

  // Toast notification helper
  function showToast(message) {
    const container = document.getElementById('toastContainer');
    if (!container) return;
    const toast = document.createElement('div');
    toast.className = 'toast';
    toast.innerHTML = `
      <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="#10b981" stroke-width="2">
        <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/>
        <polyline points="22 4 12 14.01 9 11.01"/>
      </svg>
      <span>${message}</span>
    `;
    container.appendChild(toast);
    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transform = 'translateY(10px)';
      toast.style.transition = 'all 0.3s ease';
      setTimeout(() => toast.remove(), 300);
    }, 4000);
  }

  // Health check polling loop (runs every 1.5 seconds)
  async function pollHealth() {
    try {
      const res = await fetch(`${API_BASE}api/health`, { cache: 'no-store' });
      if (!res.ok) {
        throw new Error(`Health status ${res.status}`);
      }
      const data = await res.json();
      const reconnectBanner = document.getElementById('reconnectBanner');
      if (reconnectBanner) reconnectBanner.classList.add('hidden');

      if (g_bootId === null) {
        // Initial connection
        g_bootId = data.boot_id;
        g_uiRevision = data.ui_revision;
        const reloadText = document.getElementById('lastReloadText');
        if (reloadText) reloadText.textContent = `Started ${data.started_at}`;
      } else if (g_bootId !== data.boot_id) {
        // Server restarted: reload notification and data refresh
        g_bootId = data.boot_id;
        showToast(`Changes applied, reloaded at ${data.started_at}`);
        const reloadText = document.getElementById('lastReloadText');
        if (reloadText) reloadText.textContent = `Reloaded ${data.started_at}`;
        await fetchMatchData();
      }

      if (g_uiRevision !== null && g_uiRevision !== data.ui_revision) {
        // Frontend asset changed: hot reload browser window
        g_uiRevision = data.ui_revision;
        window.location.reload();
      }
    } catch (err) {
      // Server is restarting or unreachable
      const reconnectBanner = document.getElementById('reconnectBanner');
      if (reconnectBanner) reconnectBanner.classList.remove('hidden');
    }
  }

  // Fetch full match state and analytics
  async function fetchMatchData() {
    try {
      let url = `${API_BASE}api/match`;
      const params = [];
      if (g_activeAuthToken) {
        params.push(`token=${encodeURIComponent(g_activeAuthToken)}`);
      }
      if (params.length > 0) {
        url += '?' + params.join('&');
      }

      const res = await fetch(url, { cache: 'no-store' });
      if (!res.ok) return;
      const data = await res.json();
      g_currentMatchData = data;
      renderMatchCenter(data);
    } catch (e) {
      // Connection failure handled gracefully by retry banner
    }
  }

  // Render complete dashboard interface
  function renderMatchCenter(data) {
    if (!data) return;

    // 1. Live Match Score Header
    const scoreRuns = document.getElementById('liveScoreRuns');
    if (scoreRuns) scoreRuns.textContent = `${data.totalRuns}/${data.wickets}`;

    const scoreOvers = document.getElementById('liveScoreOvers');
    if (scoreOvers) scoreOvers.textContent = `(${data.oversFormatted} Ov)`;

    const ribbonCrr = document.getElementById('ribbonCrr');
    if (ribbonCrr) ribbonCrr.textContent = Number(data.currentRunRate).toFixed(2);

    const ribbonStretch = document.getElementById('ribbonBestStretch');
    if (ribbonStretch && data.bestStretch) {
      ribbonStretch.textContent = `Overs ${data.bestStretch.startOver}–${data.bestStretch.endOver} • ${data.bestStretch.totalRuns} Runs`;
    }

    // 2. Card 1: Best 6-Over Continuous Stretch
    const stretchOvers = document.getElementById('bestStretchOvers');
    if (stretchOvers && data.bestStretch) {
      stretchOvers.textContent = `Overs ${data.bestStretch.startOver} – ${data.bestStretch.endOver}`;
    }

    const stretchRuns = document.getElementById('bestStretchRuns');
    if (stretchRuns && data.bestStretch) {
      stretchRuns.textContent = data.bestStretch.totalRuns;
    }

    renderOversChart(data.overRuns, data.bestStretch);

    // 3. Card 2: Current Run Rate & Form
    const crrDisplay = document.getElementById('crrDisplay');
    if (crrDisplay) crrDisplay.textContent = Number(data.currentRunRate).toFixed(2);

    const ballsDisplay = document.getElementById('legalBallsDisplay');
    if (ballsDisplay) ballsDisplay.textContent = data.legalBalls;

    const oversDisplay = document.getElementById('oversBowledDisplay');
    if (oversDisplay) oversDisplay.textContent = data.oversFormatted;

    renderRateTrendChart(data.overRuns);

    // 4. Card 3: Partnership Network Route
    renderPlayerSelectors(data.players);
    renderOptimalRoute(data.optimalRoute, data.players);
    renderPartnershipGraphSvg(data.players, data.graphLinks, data.optimalRoute);

    // 5. Card 4: Recursive Partnership Chain
    renderPartnershipChain(data.longestChain, data.players);

    // 6. Card 5: Player Workspace Access & Auth
    renderAuthCard(data.auth);

    // 7. Card 6: Fan Zone Poll
    renderPollCard(data.pollOptions, data.pollStatus);

    // 8. Card 7: Ball-by-Ball Feed
    renderBallFeed(data.ballFeed);
  }

  // Render Best 6-Overs bar chart
  function renderOversChart(overRuns, bestStretch) {
    const container = document.getElementById('oversChartSvgContainer');
    if (!container || !overRuns || overRuns.length === 0) return;

    const width = container.clientWidth || 480;
    const height = container.clientHeight || 140;
    const maxVal = Math.max(...overRuns, 25);
    const n = overRuns.length;
    const barWidth = Math.max(14, (width - 40) / n - 8);

    const startIdx = (bestStretch && bestStretch.startOver) ? (bestStretch.startOver - 1) : -1;
    const endIdx = (bestStretch && bestStretch.endOver) ? (bestStretch.endOver - 1) : -1;

    let barsSvg = '';
    for (let i = 0; i < n; i++) {
      const val = overRuns[i];
      const h = ((val / maxVal) * (height - 45));
      const x = 20 + i * ((width - 40) / n);
      const y = height - 25 - h;
      const isSelected = (i >= startIdx && i <= endIdx);

      const fillColor = isSelected ? '#38bdf8' : 'rgba(255, 255, 255, 0.15)';
      const strokeColor = isSelected ? '#0284c7' : 'none';

      barsSvg += `
        <g>
          <rect x="${x}" y="${y}" width="${barWidth}" height="${h}" rx="3" fill="${fillColor}" stroke="${strokeColor}" stroke-width="1.5" />
          <text x="${x + barWidth / 2}" y="${y - 4}" text-anchor="middle" fill="${isSelected ? '#38bdf8' : '#64748b'}" font-size="9" font-weight="700">${val}</text>
          <text x="${x + barWidth / 2}" y="${height - 8}" text-anchor="middle" fill="#64748b" font-size="9" font-weight="600">Ov ${i + 1}</text>
        </g>
      `;
    }

    container.innerHTML = `
      <svg width="100%" height="100%" viewBox="0 0 ${width} ${height}">
        <line x1="10" y1="${height - 24}" x2="${width - 10}" y2="${height - 24}" stroke="rgba(255,255,255,0.1)" stroke-width="1" />
        ${barsSvg}
      </svg>
    `;
  }

  // Render Over-by-Over Trend Line
  function renderRateTrendChart(overRuns) {
    const container = document.getElementById('rateTrendSvgContainer');
    if (!container || !overRuns || overRuns.length === 0) return;

    const width = container.clientWidth || 480;
    const height = container.clientHeight || 140;
    const maxVal = Math.max(...overRuns, 25);
    const n = overRuns.length;

    let points = [];
    for (let i = 0; i < n; i++) {
      const val = overRuns[i];
      const x = 30 + (i / (n - 1)) * (width - 60);
      const y = height - 30 - ((val / maxVal) * (height - 50));
      points.push({ x, y, val, over: i + 1 });
    }

    let pathD = `M ${points[0].x} ${points[0].y}`;
    for (let i = 1; i < points.length; i++) {
      pathD += ` L ${points[i].x} ${points[i].y}`;
    }

    let areaD = `${pathD} L ${points[points.length - 1].x} ${height - 20} L ${points[0].x} ${height - 20} Z`;

    let dotsSvg = '';
    for (const pt of points) {
      dotsSvg += `
        <circle cx="${pt.x}" cy="${pt.y}" r="4" fill="#10b981" stroke="#064e3b" stroke-width="2" />
        <text x="${pt.x}" y="${pt.y - 8}" text-anchor="middle" fill="#34d399" font-size="9" font-weight="700">${pt.val}</text>
        <text x="${pt.x}" y="${height - 6}" text-anchor="middle" fill="#64748b" font-size="8">${pt.over}</text>
      `;
    }

    container.innerHTML = `
      <svg width="100%" height="100%" viewBox="0 0 ${width} ${height}">
        <defs>
          <linearGradient id="areaGrad" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="rgba(16, 185, 129, 0.3)"/>
            <stop offset="100%" stop-color="rgba(16, 185, 129, 0.0)"/>
          </linearGradient>
        </defs>
        <path d="${areaD}" fill="url(#areaGrad)" />
        <path d="${pathD}" fill="none" stroke="#10b981" stroke-width="2.5" />
        ${dotsSvg}
      </svg>
    `;
  }

  // Render Player Dropdown Selectors
  function renderPlayerSelectors(players) {
    const startSelect = document.getElementById('routeStartSelect');
    const endSelect = document.getElementById('routeEndSelect');
    const chainSelect = document.getElementById('chainStartSelect');

    if (!startSelect || !players) return;

    if (startSelect.options.length === 0) {
      players.forEach(p => {
        const opt1 = new Option(`${p.name} (${p.role.split(' ')[0]})`, p.id);
        const opt2 = new Option(`${p.name} (${p.role.split(' ')[0]})`, p.id);
        const opt3 = new Option(`${p.name} (${p.role.split(' ')[0]})`, p.id);

        startSelect.add(opt1);
        endSelect.add(opt2);
        chainSelect.add(opt3);
      });

      startSelect.value = '0';
      endSelect.value = '4';
      chainSelect.value = '0';

      startSelect.addEventListener('change', onRouteSelectChange);
      endSelect.addEventListener('change', onRouteSelectChange);
      chainSelect.addEventListener('change', onChainSelectChange);
    }
  }

  async function onRouteSelectChange() {
    const start = document.getElementById('routeStartSelect').value;
    const end = document.getElementById('routeEndSelect').value;
    try {
      const res = await fetch(`${API_BASE}api/partnership/route?start=${start}&end=${end}`, { cache: 'no-store' });
      if (res.ok) {
        const data = await res.json();
        renderOptimalRoute(data, g_currentMatchData ? g_currentMatchData.players : []);
        renderPartnershipGraphSvg(
          g_currentMatchData ? g_currentMatchData.players : [],
          g_currentMatchData ? g_currentMatchData.graphLinks : [],
          data
        );
      }
    } catch (e) {}
  }

  async function onChainSelectChange() {
    const start = document.getElementById('chainStartSelect').value;
    try {
      const res = await fetch(`${API_BASE}api/partnership/chain?start=${start}`, { cache: 'no-store' });
      if (res.ok) {
        const data = await res.json();
        renderPartnershipChain(data, g_currentMatchData ? g_currentMatchData.players : []);
      }
    } catch (e) {}
  }

  // Render Optimal Partnership Route Chips
  function renderOptimalRoute(route, players) {
    const container = document.getElementById('routeChipsContainer');
    const costDisplay = document.getElementById('routeTotalCost');
    if (!container || !route) return;

    if (costDisplay) costDisplay.textContent = route.totalCost;

    const playerMap = {};
    if (players) {
      players.forEach(p => { playerMap[p.id] = p.name; });
    }

    const path = route.playerPath || [];
    container.innerHTML = path.map((id, index) => {
      const name = playerMap[id] || `Player ${id}`;
      const isLast = (index === path.length - 1);
      return `
        <span class="player-chip">${name}</span>
        ${!isLast ? '<span class="chip-arrow">➔</span>' : ''}
      `;
    }).join('');
  }

  // Render SVG Interactive Partnership Graph
  function renderPartnershipGraphSvg(players, links, route) {
    const container = document.getElementById('partnershipNetworkSvg');
    if (!container || !players) return;

    const width = container.clientWidth || 500;
    const height = container.clientHeight || 200;

    // Preset node coordinates for visual layout
    const coords = [
      { x: 50,  y: 100 }, // 0: Rohit
      { x: 180, y: 40  }, // 1: Kohli
      { x: 150, y: 160 }, // 2: Rahul
      { x: 280, y: 160 }, // 3: Gill
      { x: 330, y: 60  }, // 4: Jadeja
      { x: 420, y: 130 }, // 5: Pandya
      { x: 480, y: 70  }  // 6: Bumrah
    ];

    const activePath = (route && route.playerPath) ? route.playerPath : [];
    const activeEdges = new Set();
    for (let i = 0; i + 1 < activePath.length; i++) {
      activeEdges.add(`${activePath[i]}-${activePath[i+1]}`);
      activeEdges.add(`${activePath[i+1]}-${activePath[i]}`);
    }

    let linksSvg = '';
    if (links) {
      for (const link of links) {
        const p1 = coords[link.source];
        const p2 = coords[link.target];
        if (!p1 || !p2) continue;

        const isHighlighted = activeEdges.has(`${link.source}-${link.target}`);
        const strokeColor = isHighlighted ? '#38bdf8' : 'rgba(255,255,255,0.12)';
        const strokeWidth = isHighlighted ? 3 : 1.5;

        const midX = (p1.x + p2.x) / 2;
        const midY = (p1.y + p2.y) / 2;

        linksSvg += `
          <line x1="${p1.x}" y1="${p1.y}" x2="${p2.x}" y2="${p2.y}" stroke="${strokeColor}" stroke-width="${strokeWidth}" />
          <circle cx="${midX}" cy="${midY}" r="9" fill="#0f172a" stroke="${strokeColor}" stroke-width="1" />
          <text x="${midX}" y="${midY + 3}" text-anchor="middle" fill="${isHighlighted ? '#38bdf8' : '#94a3b8'}" font-size="8" font-weight="700">${link.weight}</text>
        `;
      }
    }

    let nodesSvg = '';
    for (let i = 0; i < players.length; i++) {
      const p = players[i];
      const pos = coords[i] || { x: 50 + i * 60, y: 100 };
      const inPath = activePath.includes(p.id);

      const fillColor = inPath ? '#0284c7' : '#1e293b';
      const strokeColor = inPath ? '#38bdf8' : 'rgba(255,255,255,0.2)';
      const initials = p.name.split(' ').map(s => s[0]).join('');

      nodesSvg += `
        <g style="cursor: pointer" onclick="window.selectGraphPlayer(${p.id})">
          <circle cx="${pos.x}" cy="${pos.y}" r="15" fill="${fillColor}" stroke="${strokeColor}" stroke-width="2" />
          <text x="${pos.x}" y="${pos.y + 4}" text-anchor="middle" fill="#fff" font-size="9" font-weight="800">${initials}</text>
          <text x="${pos.x}" y="${pos.y + 24}" text-anchor="middle" fill="${inPath ? '#38bdf8' : '#94a3b8'}" font-size="8" font-weight="600">${p.name.split(' ')[0]}</text>
        </g>
      `;
    }

    container.innerHTML = `
      <svg width="100%" height="100%" viewBox="0 0 540 200">
        ${linksSvg}
        ${nodesSvg}
      </svg>
    `;
  }

  window.selectGraphPlayer = function(id) {
    const endSelect = document.getElementById('routeEndSelect');
    if (endSelect) {
      endSelect.value = String(id);
      onRouteSelectChange();
    }
  };

  // Render Recursive Partnership Chain
  function renderPartnershipChain(chainData, players) {
    const seqContainer = document.getElementById('chainSequenceContainer');
    const lengthDisplay = document.getElementById('chainLengthDisplay');
    const vizContainer = document.getElementById('chainVisualizer');

    if (!seqContainer || !chainData) return;

    const path = chainData.playerPath || [];
    if (lengthDisplay) lengthDisplay.textContent = chainData.length || path.length;

    const playerMap = {};
    if (players) {
      players.forEach(p => { playerMap[p.id] = p.name; });
    }

    seqContainer.innerHTML = path.map((id, index) => {
      const name = playerMap[id] || `Player ${id}`;
      const isLast = (index === path.length - 1);
      return `
        <span class="chain-node-badge">${name}</span>
        ${!isLast ? '<span class="chain-arrow">➔</span>' : ''}
      `;
    }).join('');

    if (vizContainer) {
      vizContainer.innerHTML = path.map((id, index) => {
        const name = playerMap[id] || `Player ${id}`;
        return `
          <div class="chain-step-card">
            <span class="chain-step-num">STEP ${index + 1}</span>
            <span class="chain-step-name">${name}</span>
          </div>
        `;
      }).join('');
    }
  }

  // Render Player Workspace Access & Auth State
  function renderAuthCard(auth) {
    if (!auth) return;

    const nameEl = document.getElementById('authPlayerName');
    const roleEl = document.getElementById('authPlayerRole');
    const idEl = document.getElementById('authPlayerId');
    const badgeEl = document.getElementById('sessionStatusBadge');
    const ribbonBadge = document.getElementById('ribbonAuthBadge');
    const expiryEl = document.getElementById('authExpiryText');
    const avatarEl = document.getElementById('authAvatar');

    if (nameEl) nameEl.textContent = auth.playerName || 'Virat Kohli';
    if (roleEl) roleEl.textContent = `${auth.role || 'captain'} • Verified Workspace`;
    if (idEl) idEl.textContent = auth.playerId || 'player-18';
    if (avatarEl && auth.playerName) {
      avatarEl.textContent = auth.playerName.split(' ').map(s => s[0]).join('');
    }

    const isExpired = auth.isExpired;
    const isValid = auth.isValid;

    if (badgeEl) {
      if (!isExpired && isValid) {
        badgeEl.className = 'session-badge status-active';
        badgeEl.textContent = 'Active Session';
      } else {
        badgeEl.className = 'session-badge status-expired';
        badgeEl.textContent = 'Session Expired';
      }
    }

    if (ribbonBadge) {
      if (!isExpired && isValid) {
        ribbonBadge.className = 'stat-badge';
        ribbonBadge.textContent = 'Active';
      } else {
        ribbonBadge.className = 'stat-badge expired';
        ribbonBadge.textContent = 'Expired';
      }
    }

    if (expiryEl) {
      if (!isExpired && isValid) {
        expiryEl.textContent = 'Active (Valid)';
      } else {
        expiryEl.textContent = 'Expired (Timeout)';
      }
    }
  }

  // Render Fan Zone Poll Options and Cooldown Countdown
  function renderPollCard(options, status) {
    const container = document.getElementById('pollOptionsContainer');
    if (!container || !options) return;

    container.innerHTML = options.map(opt => {
      const isChecked = (opt.id === g_selectedPollOption);
      return `
        <div class="poll-option-item ${isChecked ? 'selected' : ''}" onclick="window.selectPollOption('${opt.id}')">
          <div class="option-left">
            <input type="radio" name="pollOpt" value="${opt.id}" class="option-radio" ${isChecked ? 'checked' : ''}>
            <span class="option-name">${opt.name}</span>
          </div>
          <span class="option-votes">${opt.votes} votes</span>
        </div>
      `;
    }).join('');

    if (status) {
      startCooldownCountdown(status.remainingCooldownMs);
    }
  }

  window.selectPollOption = function(optId) {
    g_selectedPollOption = optId;
    if (g_currentMatchData) {
      renderPollCard(g_currentMatchData.pollOptions, null);
    }
  };

  // Cooldown timer handler
  function startCooldownCountdown(remainingMs) {
    clearInterval(g_cooldownTimerInterval);
    g_cooldownRemainingMs = Math.max(0, remainingMs);
    g_lastCooldownFetchTime = Date.now();

    updateCooldownDisplay();

    g_cooldownTimerInterval = setInterval(() => {
      const elapsed = Date.now() - g_lastCooldownFetchTime;
      g_lastCooldownFetchTime = Date.now();
      g_cooldownRemainingMs = Math.max(0, g_cooldownRemainingMs - elapsed);
      updateCooldownDisplay();
      if (g_cooldownRemainingMs <= 0) {
        clearInterval(g_cooldownTimerInterval);
      }
    }, 250);
  }

  function updateCooldownDisplay() {
    const textEl = document.getElementById('cooldownTimerText');
    const pillEl = document.getElementById('cooldownPill');
    const btnVote = document.getElementById('btnSubmitVote');

    if (!textEl || !pillEl) return;

    if (g_cooldownRemainingMs <= 0) {
      pillEl.className = 'cooldown-pill ready';
      textEl.textContent = 'Ready to Vote';
      if (btnVote) btnVote.disabled = false;
    } else {
      pillEl.className = 'cooldown-pill';
      const secondsLeft = Math.ceil(g_cooldownRemainingMs / 1000);
      textEl.textContent = `Cooldown: ${secondsLeft}s remaining`;
    }
  }

  // Cast Fan Vote Handler
  async function submitFanVote() {
    const msgEl = document.getElementById('pollMessage');
    try {
      const res = await fetch(`${API_BASE}api/poll/vote`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          fanId: 'fan-seeded-user',
          optionId: g_selectedPollOption
        })
      });

      const data = await res.json();
      if (res.ok && data.success) {
        if (msgEl) {
          msgEl.className = 'poll-message-area success';
          msgEl.textContent = 'Vote recorded! Thank you for participating.';
        }
        startCooldownCountdown(10000); // 10 second cooldown
        await fetchMatchData();
      } else {
        if (msgEl) {
          msgEl.className = 'poll-message-area error';
          msgEl.textContent = data.error || 'Vote rejected due to cooldown limit.';
        }
        if (data.remainingCooldownMs) {
          startCooldownCountdown(data.remainingCooldownMs);
        }
      }
    } catch (e) {
      if (msgEl) {
        msgEl.className = 'poll-message-area error';
        msgEl.textContent = 'Vote submission failed. Retrying...';
      }
    }
  }

  // Renew Session Handler
  async function renewSession(playerId = 'player-18', playerName = 'Virat Kohli', role = 'captain') {
    try {
      const res = await fetch(`${API_BASE}api/auth/login`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ playerId, playerName, role })
      });
      if (res.ok) {
        const data = await res.json();
        g_activeAuthToken = data.token;
        try {
          localStorage.setItem('cricpulse_player_token', data.token);
        } catch (e) {}
        showToast(`Authenticated as ${data.playerName}`);
        await fetchMatchData();
      }
    } catch (e) {}
  }

  // Render Ball-by-Ball Feed
  function renderBallFeed(balls) {
    const list = document.getElementById('ballFeedList');
    if (!list || !balls) return;

    list.innerHTML = balls.map(b => {
      let badgeClass = 'single';
      let badgeText = String(b.runs);

      if (b.isWicket) {
        badgeClass = 'wicket';
        badgeText = 'W';
      } else if (b.runs === 0) {
        badgeClass = 'dot';
        badgeText = '•';
      } else if (b.runs === 4) {
        badgeClass = 'boundary';
        badgeText = '4';
      } else if (b.runs === 6) {
        badgeClass = 'six';
        badgeText = '6';
      }

      return `
        <div class="ball-feed-item">
          <span class="ball-indicator">${b.over}.${b.ball}</span>
          <div class="ball-badge ${badgeClass}">${badgeText}</div>
          <div class="ball-content">
            <span class="ball-players">${b.bowler} to ${b.batter}</span>
            <span class="ball-commentary">${b.commentary}</span>
          </div>
        </div>
      `;
    }).join('');
  }

  // Interactive Rate Calculator Listener
  function setupRateCalculator() {
    const runsInput = document.getElementById('calcRuns');
    const ballsInput = document.getElementById('calcBalls');
    const resultEl = document.getElementById('calcRateResult');

    async function recalculate() {
      const runs = runsInput ? runsInput.value : 155;
      const balls = ballsInput ? ballsInput.value : 75;
      try {
        const res = await fetch(`${API_BASE}api/analytics/run-rate?runs=${runs}&balls=${balls}`, { cache: 'no-store' });
        if (res.ok) {
          const data = await res.json();
          if (resultEl) resultEl.textContent = Number(data.runRate).toFixed(2);
        }
      } catch (e) {}
    }

    if (runsInput) runsInput.addEventListener('input', recalculate);
    if (ballsInput) ballsInput.addEventListener('input', recalculate);
  }

  // Navigation Tabs Listener
  function setupNavTabs() {
    const tabs = document.querySelectorAll('.nav-tab');
    tabs.forEach(tab => {
      tab.addEventListener('click', () => {
        tabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');
        const target = tab.dataset.tab;
        const cards = document.querySelectorAll('.analytics-card, .feed-card');
        cards.forEach(card => {
          if (target === 'all') {
            card.style.display = '';
          } else if (target === 'momentum') {
            card.style.display = (card.id === 'cardBestStretch' || card.id === 'cardRunRate' || card.id === 'cardFeed') ? '' : 'none';
          } else if (target === 'network') {
            card.style.display = (card.id === 'cardNetworkRoute' || card.id === 'cardChain') ? '' : 'none';
          } else if (target === 'fanzone') {
            card.style.display = (card.id === 'cardAuth' || card.id === 'cardPoll') ? '' : 'none';
          }
        });
      });
    });
  }

  // Attach DOM Listeners on page load
  window.addEventListener('DOMContentLoaded', () => {
    setupNavTabs();
    setupRateCalculator();

    const btnVote = document.getElementById('btnSubmitVote');
    if (btnVote) btnVote.addEventListener('click', submitFanVote);

    const btnRefresh = document.getElementById('btnRefreshSession');
    if (btnRefresh) {
      btnRefresh.addEventListener('click', () => {
        renewSession('player-18', 'Virat Kohli', 'captain');
      });
    }

    const btnSwitch = document.getElementById('btnSwitchPlayer');
    if (btnSwitch) {
      btnSwitch.addEventListener('click', () => {
        renewSession('player-45', 'Rohit Sharma', 'captain');
      });
    }

    const btnTrace = document.getElementById('btnExploreChain');
    if (btnTrace) {
      btnTrace.addEventListener('click', onChainSelectChange);
    }

    // Initial load
    fetchMatchData();
    pollHealth();

    // Health check polling timer
    setInterval(pollHealth, 1500);
  });

})();
