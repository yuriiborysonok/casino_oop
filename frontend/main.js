const API_URL = '/api';

const authOverlay = document.getElementById('authOverlay');
const gamePanel = document.getElementById('gamePanel');
const authTitle = document.getElementById('authTitle');
const submitAuthBtn = document.getElementById('submitAuthBtn');
const toggleAuthMode = document.getElementById('toggleAuthMode');
const authToggleText = document.getElementById('authToggleText');
const signupTerms = document.getElementById('signupTerms');
const usernameInput = document.getElementById('usernameInput');
const passwordInput = document.getElementById('passwordInput');
const authLog = document.getElementById('authLog');

const balanceDisplay = document.getElementById('balanceDisplay');
const currentUserDisplay = document.getElementById('currentUser');
const logoutBtn = document.getElementById('logoutBtn');

const lobbyView = document.getElementById('lobbyView');
const activeGameView = document.getElementById('activeGameView');
const backToLobbyBtn = document.getElementById('backToLobbyBtn');

// Slots
const slotsGameContainer = document.getElementById('slotsGameContainer');
const activeGameTitle = document.getElementById('activeGameTitle');
const activeGameImg = document.getElementById('activeGameImg');
const betInput = document.getElementById('betInput');
const spinBtn = document.getElementById('spinBtn');
const resultLog = document.getElementById('resultLog');

// Roulette
const rouletteGameContainer = document.getElementById('rouletteGameContainer');
const rouletteTable = document.getElementById('rouletteTable');
const chipSelector = document.getElementById('chipSelector');
const totalBetDisplay = document.getElementById('totalBetDisplay');
const spinRouletteBtn = document.getElementById('spinRouletteBtn');
const clearBetsBtn = document.getElementById('clearBetsBtn');
const rouletteWheel = document.getElementById('rouletteWheel');
const rouletteResultLog = document.getElementById('rouletteResultLog');
const ballWrapper = document.getElementById('ballWrapper');
// Blackjack
const blackjackGameContainer = document.getElementById('blackjackGameContainer');
const bjDealerScore = document.getElementById('bjDealerScore');
const bjDealerCards = document.getElementById('bjDealerCards');
const bjPlayerScore = document.getElementById('bjPlayerScore');
const bjPlayerCards = document.getElementById('bjPlayerCards');
const bjChipSelector = document.getElementById('bjChipSelector');
const bjBetDisplay = document.getElementById('bjBetDisplay');
const bjDealBtn = document.getElementById('bjDealBtn');
const bjHitBtn = document.getElementById('bjHitBtn');
const bjStandBtn = document.getElementById('bjStandBtn');
const bjResultLog = document.getElementById('bjResultLog');

let blackjackPlayerCards = [];
let blackjackDealerCards = [];
let blackjackBetAmount = 0;
let blackjackCurrentChip = 1;

let currentUserId = null;
let currentUsername = "";
let isLoginMode = false;
let selectedGameType = 'Blackjack';
let balanceGold = 0;
let balanceSweep = 0;
let currentCurrency = 'gold'; // 'gold' or 'sweep'

// ROULETTE LOGIC
let currentChipValue = 1;
let currentBets = []; 
let totalBetAmount = 0;
let currentRotationWheel = 0;
let currentRotationBall = 0;

function renderRouletteBoard() {
  rouletteTable.innerHTML = '';
  const zero = document.createElement('div');
  zero.className = 'bet-area board-zero bg-green';
  zero.textContent = '0';
  zero.onclick = () => placeBet('number', '0', zero);
  rouletteTable.appendChild(zero);

  const order = [
    3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36,
    2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35,
    1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34
  ];
  const reds = [1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36];
  
  order.forEach((num, index) => {
    const row = Math.floor(index / 12) + 1;
    const col = (index % 12) + 2; 
    const cell = document.createElement('div');
    cell.className = `bet-area ${reds.includes(num) ? 'bg-red' : 'bg-black'}`;
    cell.style.gridRow = `${row} / ${row + 1}`;
    cell.style.gridColumn = `${col} / ${col + 1}`;
    cell.textContent = num;
    cell.onclick = () => placeBet('number', num.toString(), cell);
    rouletteTable.appendChild(cell);
  });

  for(let i=1; i<=3; i++) {
    const cell = document.createElement('div');
    cell.className = 'bet-area';
    cell.style.gridRow = '4 / 5';
    cell.style.gridColumn = `${(i-1)*4 + 2} / ${i*4 + 2}`;
    cell.textContent = `${i}st 12`;
    cell.onclick = () => placeBet('dozen', i.toString(), cell);
    rouletteTable.appendChild(cell);
  }

  const outside = [
    { label: 'EVEN', type: 'evenodd', value: 'even', span: 3 },
    { label: 'RED', type: 'color', value: 'red', span: 3, cls: 'bg-red' },
    { label: 'BLACK', type: 'color', value: 'black', span: 3, cls: 'bg-black' },
    { label: 'ODD', type: 'evenodd', value: 'odd', span: 3 }
  ];
  let currentOutCol = 2;
  outside.forEach(out => {
    const cell = document.createElement('div');
    cell.className = `bet-area ${out.cls || ''}`;
    cell.style.gridRow = '5 / 6';
    cell.style.gridColumn = `${currentOutCol} / ${currentOutCol + out.span}`;
    cell.textContent = out.label;
    cell.onclick = () => placeBet(out.type, out.value, cell);
    rouletteTable.appendChild(cell);
    currentOutCol += out.span;
  });
}

function placeBet(type, value, cellElement) {
  let existingBet = currentBets.find(b => b.type === type && b.value === value);
  if (existingBet) {
    existingBet.amount += currentChipValue;
  } else {
    currentBets.push({ type, value, amount: currentChipValue });
  }
  
  totalBetAmount += currentChipValue;
  totalBetDisplay.textContent = totalBetAmount;
  
  let chipEl = cellElement.querySelector('.placed-chip');
  if (!chipEl) {
    chipEl = document.createElement('div');
    chipEl.className = 'placed-chip';
    cellElement.appendChild(chipEl);
  }
  
  let totalOnCell = existingBet ? existingBet.amount : currentChipValue;
  
  // Format the text representation
  if (totalOnCell >= 1000000) {
    chipEl.textContent = `${Math.floor(totalOnCell / 1000000)}kk`;
    chipEl.style.fontSize = '0.65rem';
  } else if (totalOnCell >= 100000) {
    chipEl.textContent = `${Math.floor(totalOnCell / 1000)}k`;
    chipEl.style.fontSize = '0.65rem';
  } else if (totalOnCell >= 10000) {
    chipEl.textContent = `${Math.floor(totalOnCell / 1000)}k`;
    chipEl.style.fontSize = '0.75rem';
  } else if (totalOnCell >= 1000) {
    const formatted = totalOnCell / 1000;
    chipEl.textContent = formatted % 1 === 0 ? `${formatted}k` : `${formatted.toFixed(1)}k`;
    chipEl.style.fontSize = '0.75rem';
  } else {
    chipEl.textContent = totalOnCell;
    chipEl.style.fontSize = '0.8rem';
  }
  
  // Apply visual colors
  if (totalOnCell >= 1000000) {
    chipEl.style.background = 'radial-gradient(circle, #f59e0b 0%, #78350f 100%)';
    chipEl.style.borderColor = '#fff';
    chipEl.style.color = '#fff';
    chipEl.style.boxShadow = '0 0 8px #f59e0b';
  }
  else if (totalOnCell >= 100000) {
    chipEl.style.background = '#18181b';
    chipEl.style.borderColor = '#feca3b';
    chipEl.style.color = '#feca3b';
    chipEl.style.boxShadow = 'none';
  }
  else if (totalOnCell >= 10000) {
    chipEl.style.background = '#5b21b6';
    chipEl.style.borderColor = '#c084fc';
    chipEl.style.color = '#fff';
    chipEl.style.boxShadow = 'none';
  }
  else if (totalOnCell >= 1000) {
    chipEl.style.background = '#800020';
    chipEl.style.borderColor = '#feca3b';
    chipEl.style.color = '#fff';
    chipEl.style.boxShadow = 'none';
  }
  else if (totalOnCell >= 100) {
    chipEl.style.background = '#111';
    chipEl.style.borderColor = 'rgba(255,255,255,0.5)';
    chipEl.style.color = '#fff';
    chipEl.style.boxShadow = 'none';
  }
  else if (totalOnCell >= 50) {
    chipEl.style.background = '#10b981';
    chipEl.style.borderColor = 'rgba(255,255,255,0.5)';
    chipEl.style.color = '#fff';
  }
  else if (totalOnCell >= 10) {
    chipEl.style.background = '#feca3b';
    chipEl.style.borderColor = 'rgba(255,255,255,0.5)';
    chipEl.style.color = '#000';
  }
  else if (totalOnCell >= 5) {
    chipEl.style.background = '#d946ef';
    chipEl.style.borderColor = 'rgba(255,255,255,0.5)';
    chipEl.style.color = '#fff';
  }
  else {
    chipEl.style.background = '#3b9fff';
    chipEl.style.borderColor = 'rgba(255,255,255,0.5)';
    chipEl.style.color = '#fff';
  }
}

clearBetsBtn.addEventListener('click', () => {
  currentBets = [];
  totalBetAmount = 0;
  totalBetDisplay.textContent = '0';
  document.querySelectorAll('.placed-chip').forEach(el => el.remove());
  rouletteResultLog.textContent = "Bets cleared. Place your bets!";
  rouletteResultLog.className = 'result-log console-log';
});

chipSelector.addEventListener('click', (e) => {
  if (e.target.classList.contains('chip')) {
    document.querySelectorAll('#chipSelector .chip').forEach(c => c.classList.remove('selected'));
    e.target.classList.add('selected');
    currentChipValue = parseInt(e.target.getAttribute('data-value'));
  }
});

// Map roulette numbers to approximate wheel angles (European)
const wheelNumbers = [0, 32, 15, 19, 4, 21, 2, 25, 17, 34, 6, 27, 13, 36, 11, 30, 8, 23, 10, 5, 24, 16, 33, 1, 20, 14, 31, 9, 22, 18, 29, 7, 28, 12, 35, 3, 26];
function getAngleForNumber(num) {
  const index = wheelNumbers.indexOf(num);
  // 360 / 37 = 9.729 degrees per segment
  // The wheel image has 0 at top. Let's calculate the exact offset
  return 360 - (index * (360 / 37));
}

spinRouletteBtn.addEventListener('click', async () => {
  if (currentBets.length === 0) {
    rouletteResultLog.textContent = 'Please place at least one bet!';
    rouletteResultLog.className = 'result-log console-log error shake';
    setTimeout(() => rouletteResultLog.classList.remove('shake'), 400);
    return;
  }

  const currentBalance = currentCurrency === 'gold' ? balanceGold : balanceSweep;
  if (totalBetAmount > currentBalance) {
    rouletteResultLog.textContent = `Insufficient balance (${currentCurrency.toUpperCase()}).`;
    rouletteResultLog.className = 'result-log console-log error shake';
    setTimeout(() => rouletteResultLog.classList.remove('shake'), 400);
    return;
  }

  spinRouletteBtn.disabled = true;
  clearBetsBtn.disabled = true;
  rouletteResultLog.textContent = 'No more bets! Spinning...';
  rouletteResultLog.className = 'result-log console-log';

  try {
    const response = await fetch(`${API_URL}/spin`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ 
        userId: currentUserId, 
        gameType: 'Roulette', 
        currency: currentCurrency,
        bets: currentBets 
      })
    });

    const data = await response.json();
    if (!response.ok) throw new Error(data.error);

    const winNum = data.winning_number;
    
    // Realistic Physics simulation
    rouletteBall.classList.add('active');

    // Wheel spins backwards (counter-clockwise)
    const extraSpinsWheel = -3 * 360; 
    const targetAngle = getAngleForNumber(winNum);
    currentRotationWheel = currentRotationWheel - (currentRotationWheel % 360) + extraSpinsWheel + targetAngle;
    
    // Ball spins very fast forwards (clockwise)
    const extraSpinsBall = 10 * 360;
    // We want the ball to end up EXACTLY at 0deg relative to its container (the top)
    currentRotationBall = currentRotationBall - (currentRotationBall % 360) + extraSpinsBall;

    rouletteWheel.style.transform = `rotate(${currentRotationWheel}deg)`;
    ballWrapper.style.transform = `rotate(${currentRotationBall}deg)`;

    // Wait for animation to finish (6s)
    setTimeout(() => {
      updateBalanceUI(data.new_balance_gold, data.new_balance_sweep);
      fetchTransactionHistory(); // Refresh transactions
      fetchAppInsights();        // Refresh DB insights
      
      if (data.status === 'win') {
        rouletteResultLog.innerHTML = `Landed on <span style="color:#fff;font-size:1.5rem;">${winNum}</span>! 🎉 YOU WON $${data.won_amount.toFixed(2)}!`;
        rouletteResultLog.className = 'result-log console-log win shake';
      } else {
        rouletteResultLog.innerHTML = `Landed on <span style="color:#fff;font-size:1.5rem;">${winNum}</span>. 💔 You lost your bet.`;
        rouletteResultLog.className = 'result-log console-log lose';
      }
      spinRouletteBtn.disabled = false;
      clearBetsBtn.disabled = false;
      
      // Clear board for next round
      currentBets = [];
      totalBetAmount = 0;
      totalBetDisplay.textContent = '0';
      document.querySelectorAll('.placed-chip').forEach(el => el.remove());
    }, 6000);

  } catch (err) {
    rouletteResultLog.textContent = err.message;
    rouletteResultLog.className = 'result-log console-log error shake';
    spinRouletteBtn.disabled = false;
    clearBetsBtn.disabled = false;
  }
});


// Initialization
renderRouletteBoard();

const gameCards = document.querySelectorAll('.game-card:not(.coming-soon)');
gameCards.forEach(card => {
  card.addEventListener('click', () => {
    selectedGameType = card.getAttribute('data-game');
    lobbyView.classList.add('hidden');
    activeGameView.classList.remove('hidden');

    if (selectedGameType === 'Roulette') {
      slotsGameContainer.classList.add('hidden');
      blackjackGameContainer.classList.add('hidden');
      rouletteGameContainer.classList.remove('hidden');
    } else if (selectedGameType === 'Slots') {
      rouletteGameContainer.classList.add('hidden');
      blackjackGameContainer.classList.add('hidden');
      slotsGameContainer.classList.remove('hidden');
      
      activeGameTitle.textContent = "Classic Slots VIP";
      activeGameImg.src = "/slots.png";
    } else if (selectedGameType === 'Blackjack') {
      rouletteGameContainer.classList.add('hidden');
      slotsGameContainer.classList.add('hidden');
      blackjackGameContainer.classList.remove('hidden');
      
      resetBlackjackUI();
    }
  });
});

backToLobbyBtn.addEventListener('click', () => {
  activeGameView.classList.add('hidden');
  lobbyView.classList.remove('hidden');
});

// Auth Handlers
function setAuthMode(login) {
  isLoginMode = login;
  if (login) {
    authTitle.textContent = "Log in";
    submitAuthBtn.textContent = "LOG IN";
    signupTerms.classList.add('hidden');
    authToggleText.textContent = "Don't have an account?";
    toggleAuthMode.textContent = "Sign up";
  } else {
    authTitle.textContent = "Sign up";
    submitAuthBtn.textContent = "CONTINUE";
    signupTerms.classList.remove('hidden');
    authToggleText.textContent = "Already have an account?";
    toggleAuthMode.textContent = "Log in";
  }
  authLog.textContent = "";
}

toggleAuthMode.addEventListener('click', (e) => {
  e.preventDefault();
  setAuthMode(!isLoginMode);
});

document.querySelector('.toggle-password').addEventListener('click', function () {
  if (passwordInput.type === "password") {
    passwordInput.type = "text";
    this.classList.replace('fa-eye-slash', 'fa-eye');
  } else {
    passwordInput.type = "password";
    this.classList.replace('fa-eye', 'fa-eye-slash');
  }
});

function updateBalanceUI(newGold, newSweep) {
  balanceGold = parseFloat(newGold);
  balanceSweep = parseFloat(newSweep);
  renderBalance();
}

function renderBalance() {
  const balanceDisplay = document.getElementById('balanceDisplay');
  const balanceIcon = document.getElementById('balanceIcon');
  if (!balanceDisplay || !balanceIcon) return;
  
  if (currentCurrency === 'gold') {
    balanceDisplay.textContent = balanceGold.toLocaleString(undefined, {minimumFractionDigits: 0, maximumFractionDigits: 2});
    balanceIcon.innerHTML = '<i class="fa-solid fa-shield-halved" style="color: #feca3b;"></i>';
  } else {
    balanceDisplay.textContent = balanceSweep.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2});
    balanceIcon.innerHTML = '<i class="fa-solid fa-shield-halved" style="color: #3b9fff;"></i>';
  }
}

document.querySelectorAll('input[name="currency"]').forEach(radio => {
  radio.addEventListener('change', (e) => {
    currentCurrency = e.target.value;
    renderBalance();
  });
});
// === MONITORING, TRANSACTION HISTORY & APP INSIGHTS INTEGRATION ===
const txList = document.getElementById('txList');
const statTotalUsers = document.getElementById('statTotalUsers');
const statGoldVol = document.getElementById('statGoldVol');
const statSweepVol = document.getElementById('statSweepVol');
const statWinRatio = document.getElementById('statWinRatio');
const globalActivityList = document.getElementById('globalActivityList');
const healthPulse = document.getElementById('healthPulse');
const healthWalletStatus = document.getElementById('healthWalletStatus');
const healthDbStatus = document.getElementById('healthDbStatus');
const healthUptime = document.getElementById('healthUptime');

// 1. Fetch User Transaction History
async function fetchTransactionHistory() {
  if (!currentUserId) return;
  try {
    const response = await fetch(`${API_URL}/transactions?userId=${currentUserId}`);
    if (!response.ok) throw new Error("Failed to load");
    const data = await response.json();
    
    if (data.length === 0) {
      txList.innerHTML = `<div class="no-tx-placeholder">No transactions found. Make a bet to start!</div>`;
      return;
    }
    
    txList.innerHTML = data.map(tx => {
      let cssClass = "deposit-tx";
      let icon = "fa-money-bill-wave";
      let sign = "+";
      
      if (tx.type.includes("WIN")) {
        cssClass = "win-tx";
        icon = "fa-circle-check";
        sign = "+";
      } else if (tx.type.includes("LOSE") || tx.type.includes("BET")) {
        cssClass = "lose-tx";
        icon = "fa-circle-minus";
        sign = "-";
      }
      
      const currencySymbol = tx.currency === "sweep" ? "SC" : "GC";
      const formattedAmount = `${sign}${tx.amount.toLocaleString()} ${currencySymbol}`;
      const formattedDate = new Date(tx.created_at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
      
      return `
        <div class="tx-item ${cssClass}">
          <div class="tx-info-left">
            <span class="tx-type"><i class="fa-solid ${icon}"></i> ${tx.type}</span>
            <span class="tx-time">${formattedDate}</span>
          </div>
          <span class="tx-amount ${cssClass}">${formattedAmount}</span>
        </div>
      `;
    }).join("");
  } catch (err) {
    txList.innerHTML = `<div class="no-tx-placeholder" style="color:#ff4b4b;">Failed to load transaction history</div>`;
  }
}

// 2. Fetch App Insights (Database Stats)
async function fetchAppInsights() {
  try {
    const response = await fetch(`${API_URL}/insights`);
    if (!response.ok) throw new Error("Failed to load");
    const data = await response.json();
    
    statTotalUsers.textContent = data.total_users;
    statGoldVol.textContent = `${(data.gold_volume || 0).toLocaleString()} GC`;
    statSweepVol.textContent = `${(data.sweep_volume || 0).toLocaleString()} SC`;
    
    const totalGames = (data.total_wins || 0) + (data.total_losses || 0);
    if (totalGames > 0) {
      const winPct = ((data.total_wins / totalGames) * 100).toFixed(0);
      statWinRatio.textContent = `${winPct}% Wins (${data.total_wins}/${data.total_losses})`;
    } else {
      statWinRatio.textContent = "No games yet";
    }

    if (data.recent_transactions && data.recent_transactions.length > 0) {
      globalActivityList.innerHTML = data.recent_transactions.slice(0, 5).map(tx => {
        let typeBadge = tx.type.includes("WIN") ? `<span style="color:#10b981;">WIN</span>` : `<span style="color:#ff4b4b;">LOSE</span>`;
        if (tx.type.includes("INITIAL")) typeBadge = `<span style="color:#3b9fff;">NEW</span>`;
        
        const currencySymbol = tx.currency === "sweep" ? "SC" : "GC";
        return `
          <div class="activity-item">
            <span class="activity-item-type">${typeBadge} | ${tx.type.replace("WIN_","").replace("LOSE_","")}</span>
            <span>${tx.amount.toLocaleString()} ${currencySymbol}</span>
          </div>
        `;
      }).join("");
    } else {
      globalActivityList.innerHTML = `<div style="color:#888;font-size:0.8rem;text-align:center;padding:10px 0;">No global activity recorded yet</div>`;
    }
  } catch (err) {
    statTotalUsers.textContent = "Err";
    statGoldVol.textContent = "Err";
    statSweepVol.textContent = "Err";
    statWinRatio.textContent = "Err";
    globalActivityList.innerHTML = `<div style="color:#ff4b4b;font-size:0.8rem;text-align:center;padding:10px 0;">Insights loading failed</div>`;
  }
}

// 3. Health check for Wallet service & Postgres database
async function checkInfrastructureHealth() {
  try {
    // Note: Gateway forwards /health to https://casino-wallet.fly.dev/health
    const response = await fetch(`/health`);
    if (!response.ok) throw new Error("Offline");
    const data = await response.json();
    
    if (data.status === "ok") {
      healthPulse.className = "pulse-indicator online";
      
      healthWalletStatus.textContent = "ONLINE";
      healthWalletStatus.className = "status-badge success";
      
      if (data.database === "connected") {
        healthDbStatus.textContent = "CONNECTED";
        healthDbStatus.className = "status-badge success";
      } else {
        healthDbStatus.textContent = "DISCONNECTED";
        healthDbStatus.className = "status-badge danger";
      }
      
      const uptimeMin = Math.floor(data.uptime_seconds / 60);
      const uptimeSec = data.uptime_seconds % 60;
      healthUptime.textContent = `${uptimeMin}m ${uptimeSec}s`;
    } else {
      throw new Error("Degraded");
    }
  } catch (err) {
    healthPulse.className = "pulse-indicator offline";
    
    healthWalletStatus.textContent = "OFFLINE";
    healthWalletStatus.className = "status-badge danger";
    
    healthDbStatus.textContent = "UNREACHABLE";
    healthDbStatus.className = "status-badge danger";
    
    healthUptime.textContent = "N/A";
  }
}

// Attach Refresh button listeners
document.getElementById('refreshTxBtn').addEventListener('click', fetchTransactionHistory);
document.getElementById('refreshInsightsBtn').addEventListener('click', fetchAppInsights);

function refreshAllDashboards() {
  fetchTransactionHistory();
  fetchAppInsights();
  checkInfrastructureHealth();
}

// Set up periodic status monitoring
setInterval(checkInfrastructureHealth, 20000); // Check health every 20 seconds
setInterval(fetchAppInsights, 30000);          // Refresh DB insights every 30 seconds

function showGamePanel() { 
  authOverlay.classList.add('hidden'); 
  gamePanel.classList.remove('hidden'); 
  lobbyView.classList.remove('hidden'); 
  activeGameView.classList.add('hidden'); 
  currentUserDisplay.textContent = currentUsername; 
  
  // Cache the session in localStorage
  localStorage.setItem('casino_user_id', currentUserId);
  localStorage.setItem('casino_username', currentUsername);
  
  refreshAllDashboards(); // Trigger loading of dashboards on login
}

function showAuthPanel() { 
  gamePanel.classList.add('hidden'); 
  authOverlay.classList.remove('hidden'); 
  currentUserId = null; 
  currentUsername = ""; 
  authLog.textContent = ""; 
  
  // Clear cached session
  localStorage.removeItem('casino_user_id');
  localStorage.removeItem('casino_username');
  
  setAuthMode(false); 
}
function validateEmail(email) { return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email); }

submitAuthBtn.addEventListener('click', async () => {
  const username = usernameInput.value.trim();
  const password = passwordInput.value.trim();
  if (!username || !password) { authLog.textContent = 'Please enter email and password'; authLog.style.color = '#ff4b4b'; return; }
  if (!validateEmail(username)) { authLog.textContent = 'Please enter a valid email address'; authLog.style.color = '#ff4b4b'; return; }
  if (!isLoginMode && !document.getElementById('ageCheck').checked) { authLog.textContent = 'You must agree to the Terms & Conditions'; authLog.style.color = '#ff4b4b'; return; }

  authLog.textContent = 'Processing...';
  authLog.style.color = '#fff';
  const endpoint = isLoginMode ? 'login' : 'register';

  try {
    const response = await fetch(`${API_URL}/${endpoint}`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ username, password }) });
    const data = await response.json();
    if (response.ok) { currentUserId = data.user_id; currentUsername = username; updateBalanceUI(data.balance_gold, data.balance_sweep); showGamePanel(); }
    else { authLog.textContent = data.error || 'Authentication failed'; authLog.style.color = '#ff4b4b'; }
  } catch (err) { authLog.textContent = 'Server connection error'; authLog.style.color = '#ff4b4b'; }
});

logoutBtn.addEventListener('click', showAuthPanel);

// Generic Slots Spin logic
spinBtn.addEventListener('click', async () => {
  if (!currentUserId) return;
  const gameType = 'Slots';
  const betAmount = parseFloat(betInput.value);

  if (isNaN(betAmount) || betAmount <= 0) {
    resultLog.textContent = 'Enter valid bet amount.';
    resultLog.className = 'result-log console-log error shake';
    setTimeout(() => resultLog.classList.remove('shake'), 400);
    return;
  }
  spinBtn.disabled = true; spinBtn.textContent = '...';
  resultLog.textContent = 'Processing...'; resultLog.className = 'result-log console-log';

  try {
    const response = await fetch(`${API_URL}/spin`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ userId: currentUserId, gameType, betAmount, currency: currentCurrency }) });
    const data = await response.json();
    if (!response.ok) throw new Error(data.error);

    updateBalanceUI(data.new_balance_gold, data.new_balance_sweep);
    fetchTransactionHistory(); // Refresh transactions
    fetchAppInsights();        // Refresh DB insights
    if (data.status === 'win') { resultLog.textContent = `🎉 JACKPOT! +$${data.won_amount.toFixed(2)}`; resultLog.className = 'result-log console-log win shake'; }
    else { resultLog.textContent = `💔 You lost $${betAmount.toFixed(2)}`; resultLog.className = 'result-log console-log lose'; }
  } catch (err) { resultLog.textContent = err.message; resultLog.className = 'result-log console-log error shake'; }
  finally { spinBtn.disabled = false; spinBtn.textContent = 'SPIN NOW'; setTimeout(() => resultLog.classList.remove('shake'), 400); }
});

// Social Login
async function socialLogin(email, provider) {
  try {
    const response = await fetch(`${API_URL}/social_login`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ email, provider }) });
    const data = await response.json();
    if (response.ok) { currentUserId = data.user_id; currentUsername = email; updateBalanceUI(data.balance_gold, data.balance_sweep); showGamePanel(); }
    else { authLog.textContent = 'Social Auth failed'; }
  } catch (err) { authLog.textContent = 'Server error'; }
}

function parseJwt(token) {
  try { return JSON.parse(decodeURIComponent(window.atob(token.split('.')[1].replace(/-/g, '+').replace(/_/g, '/')).split('').map(function (c) { return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2); }).join(''))); } catch (e) { return null; }
}

function handleGoogleAuth(response) {
  const payload = parseJwt(response.credential);
  if (payload && payload.email) { socialLogin(payload.email, "google"); } else { authLog.textContent = 'Google Auth Error'; authLog.style.color = '#ff4b4b'; }
}

// ==========================================================================
// BLACKJACK PRO INTERACTIVE LOGIC
// ==========================================================================
function resetBlackjackUI() {
  blackjackPlayerCards = [];
  blackjackDealerCards = [];
  blackjackBetAmount = 0;
  bjBetDisplay.textContent = '0';
  bjDealerScore.textContent = '0';
  bjPlayerScore.textContent = '0';
  bjDealerCards.innerHTML = '<div class="card-placeholder">Dealer cards go here</div>';
  bjPlayerCards.innerHTML = '<div class="card-placeholder">Your cards go here</div>';
  bjResultLog.textContent = 'Choose a bet and click DEAL HAND to start!';
  bjResultLog.className = 'result-log console-log';
  bjDealBtn.disabled = false;
  bjDealBtn.classList.remove('hidden');
  bjHitBtn.classList.add('hidden');
  bjStandBtn.classList.add('hidden');
}

function bjRenderHand(container, cards, isDealer, isFinished) {
  container.innerHTML = '';
  cards.forEach((card, idx) => {
    const cardEl = document.createElement('div');
    cardEl.className = 'card-item';
    
    // Hidden dealer card logic
    if (isDealer && idx === 1 && !isFinished) {
      cardEl.classList.add('face-down');
      container.appendChild(cardEl);
      return;
    }
    
    const suit = card.suit;
    if (suit === '♥' || suit === '♦') {
      cardEl.classList.add('red-suit');
    } else {
      cardEl.classList.add('black-suit');
    }
    
    cardEl.innerHTML = `
      <div class="card-top">${card.value}<br>${suit}</div>
      <div class="card-suit-center">${suit}</div>
      <div class="card-bottom">${card.value}<br>${suit}</div>
    `;
    container.appendChild(cardEl);
  });
}

// Bind Chip Selector for Blackjack
bjChipSelector.addEventListener('click', (e) => {
  const chip = e.target.closest('.chip');
  if (!chip) return;
  
  bjChipSelector.querySelectorAll('.chip').forEach(el => el.classList.remove('selected'));
  chip.classList.add('selected');
  blackjackCurrentChip = parseInt(chip.getAttribute('data-value'));
  
  blackjackBetAmount += blackjackCurrentChip;
  bjBetDisplay.textContent = blackjackBetAmount.toLocaleString();
});

// Clear Bet on click of the display
bjBetDisplay.parentElement.addEventListener('click', () => {
  blackjackBetAmount = 0;
  bjBetDisplay.textContent = '0';
});

bjDealBtn.addEventListener('click', async () => {
  if (blackjackBetAmount <= 0) {
    bjResultLog.textContent = 'Please select a chip to place a bet first!';
    bjResultLog.className = 'result-log console-log error shake';
    setTimeout(() => bjResultLog.classList.remove('shake'), 400);
    return;
  }
  
  const currentBalance = currentCurrency === 'gold' ? balanceGold : balanceSweep;
  if (blackjackBetAmount > currentBalance) {
    bjResultLog.textContent = `Insufficient balance (${currentCurrency.toUpperCase()}).`;
    bjResultLog.className = 'result-log console-log error shake';
    setTimeout(() => bjResultLog.classList.remove('shake'), 400);
    return;
  }
  
  bjDealBtn.disabled = true;
  bjResultLog.textContent = 'Dealing hands...';
  bjResultLog.className = 'result-log console-log';
  
  try {
    const response = await fetch(`${API_URL}/blackjack/deal`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        userId: currentUserId,
        betAmount: blackjackBetAmount,
        currency: currentCurrency
      })
    });
    
    const data = await response.json();
    if (!response.ok) throw new Error(data.error);
    
    blackjackPlayerCards = data.playerCards;
    blackjackDealerCards = data.dealerCards;
    
    bjRenderHand(bjPlayerCards, blackjackPlayerCards, false, false);
    bjRenderHand(bjDealerCards, blackjackDealerCards, true, false);
    
    bjPlayerScore.textContent = data.playerScore;
    bjDealerScore.textContent = '?'; // First card value only
    
    if (data.status === 'blackjack') {
      updateBalanceUI(data.new_balance_gold, data.new_balance_sweep);
      fetchTransactionHistory();
      fetchAppInsights();
      
      bjRenderHand(bjDealerCards, blackjackDealerCards, true, true);
      bjDealerScore.textContent = data.dealerScore;
      
      bjResultLog.textContent = `🎉 NATURAL BLACKJACK! Payout: $${data.wonAmount.toFixed(2)}`;
      bjResultLog.className = 'result-log console-log win shake';
      bjDealBtn.classList.remove('hidden');
      bjDealBtn.disabled = false;
    } else {
      bjDealBtn.classList.add('hidden');
      bjHitBtn.classList.remove('hidden');
      bjStandBtn.classList.remove('hidden');
      bjResultLog.textContent = 'Hit or Stand?';
    }
    
  } catch (err) {
    bjResultLog.textContent = err.message;
    bjResultLog.className = 'result-log console-log error';
    bjDealBtn.disabled = false;
  }
});

bjHitBtn.addEventListener('click', async () => {
  bjHitBtn.disabled = true;
  bjStandBtn.disabled = true;
  
  try {
    const response = await fetch(`${API_URL}/blackjack/hit`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        userId: currentUserId,
        betAmount: blackjackBetAmount,
        currency: currentCurrency,
        playerCards: blackjackPlayerCards,
        dealerCards: blackjackDealerCards
      })
    });
    
    const data = await response.json();
    if (!response.ok) throw new Error(data.error);
    
    blackjackPlayerCards = data.playerCards;
    bjRenderHand(bjPlayerCards, blackjackPlayerCards, false, false);
    bjPlayerScore.textContent = data.playerScore;
    
    if (data.status === 'lose') {
      updateBalanceUI(data.new_balance_gold, data.new_balance_sweep);
      fetchTransactionHistory();
      fetchAppInsights();
      
      bjRenderHand(bjDealerCards, blackjackDealerCards, true, true);
      bjDealerScore.textContent = data.dealerScore;
      
      bjResultLog.textContent = `💥 BUST! You went over 21. Lost $${blackjackBetAmount.toFixed(2)}`;
      bjResultLog.className = 'result-log console-log lose';
      
      bjHitBtn.classList.add('hidden');
      bjStandBtn.classList.add('hidden');
      bjDealBtn.classList.remove('hidden');
      bjDealBtn.disabled = false;
    } else {
      bjResultLog.textContent = 'Hit or Stand?';
    }
  } catch (err) {
    bjResultLog.textContent = err.message;
    bjResultLog.className = 'result-log console-log error';
  } finally {
    bjHitBtn.disabled = false;
    bjStandBtn.disabled = false;
  }
});

bjStandBtn.addEventListener('click', async () => {
  bjHitBtn.disabled = true;
  bjStandBtn.disabled = true;
  bjResultLog.textContent = "Dealer's turn...";
  
  try {
    const response = await fetch(`${API_URL}/blackjack/stand`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        userId: currentUserId,
        betAmount: blackjackBetAmount,
        currency: currentCurrency,
        playerCards: blackjackPlayerCards,
        dealerCards: blackjackDealerCards
      })
    });
    
    const data = await response.json();
    if (!response.ok) throw new Error(data.error);
    
    blackjackDealerCards = data.dealerCards;
    bjRenderHand(bjDealerCards, blackjackDealerCards, true, true);
    bjDealerScore.textContent = data.dealerScore;
    
    updateBalanceUI(data.new_balance_gold, data.new_balance_sweep);
    fetchTransactionHistory();
    fetchAppInsights();
    
    if (data.status === 'win') {
      bjResultLog.textContent = `🎉 YOU WON! Dealer had ${data.dealerScore}. +$${blackjackBetAmount.toFixed(2)}`;
      bjResultLog.className = 'result-log console-log win shake';
    } else if (data.status === 'lose') {
      bjResultLog.textContent = `💔 Dealer wins with ${data.dealerScore}. Lost $${blackjackBetAmount.toFixed(2)}`;
      bjResultLog.className = 'result-log console-log lose';
    } else {
      bjResultLog.textContent = `🤝 PUSH! It is a tie. Bet returned.`;
      bjResultLog.className = 'result-log console-log';
    }
    
    bjHitBtn.classList.add('hidden');
    bjStandBtn.classList.add('hidden');
    bjDealBtn.classList.remove('hidden');
    bjDealBtn.disabled = false;
    
  } catch (err) {
    bjResultLog.textContent = err.message;
    bjResultLog.className = 'result-log console-log error';
  } finally {
    bjHitBtn.disabled = false;
    bjStandBtn.disabled = false;
  }
});



// 4. Auto-restore session from localStorage on refresh
async function restoreSession() {
  const savedUserId = localStorage.getItem('casino_user_id');
  const savedUsername = localStorage.getItem('casino_username');
  
  if (!savedUserId || !savedUsername) {
    return; // No saved session, stay on login screen
  }
  
  try {
    // Show temporary overlay loading status
    authLog.textContent = 'Restoring session...';
    authLog.style.color = '#feca3b';
    
    const response = await fetch(`${API_URL}/balance?userId=${savedUserId}`);
    if (!response.ok) throw new Error("Session invalid");
    const data = await response.json();
    
    currentUserId = parseInt(savedUserId);
    currentUsername = savedUsername;
    
    updateBalanceUI(data.balance_gold, data.balance_sweep);
    showGamePanel();
  } catch (err) {
    // If restoration failed (server error or deleted user), clear session
    localStorage.removeItem('casino_user_id');
    localStorage.removeItem('casino_username');
    authLog.textContent = '';
  }
}

window.onload = function () {
  if (typeof google !== 'undefined') { 
    google.accounts.id.initialize({ 
      client_id: "824007922604-pqvviecs5t3cf377s4o8qrirhsdl8gho.apps.googleusercontent.com", 
      callback: handleGoogleAuth 
    }); 
  }
  
  // Restore session if available
  restoreSession();
};

document.querySelector('.social-btn.google').addEventListener('click', (e) => {
  e.preventDefault();
  if (typeof google !== 'undefined') { google.accounts.id.prompt((notification) => { if (notification.isNotDisplayed() || notification.isSkippedMoment()) { authLog.textContent = 'Google popup blocked.'; authLog.style.color = '#ffb84d'; } }); }
});

document.querySelector('.social-btn.facebook').addEventListener('click', (e) => { e.preventDefault(); authLog.textContent = 'Facebook OAuth requires App ID.'; authLog.style.color = '#ffb84d'; });
