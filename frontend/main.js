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
const rouletteBall = document.getElementById('rouletteBall');

let currentUserId = null;
let currentUsername = "";
let isLoginMode = false;
let selectedGameType = 'Roulette';
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
  chipEl.textContent = totalOnCell;
  
  if (totalOnCell >= 100) chipEl.style.background = '#111';
  else if (totalOnCell >= 50) chipEl.style.background = '#10b981';
  else if (totalOnCell >= 10) chipEl.style.background = '#feca3b';
  else if (totalOnCell >= 5) chipEl.style.background = '#d946ef';
  else chipEl.style.background = '#3b9fff';
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
      rouletteGameContainer.classList.remove('hidden');
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
function showGamePanel() { authOverlay.classList.add('hidden'); gamePanel.classList.remove('hidden'); lobbyView.classList.remove('hidden'); activeGameView.classList.add('hidden'); currentUserDisplay.textContent = currentUsername; }
function showAuthPanel() { gamePanel.classList.add('hidden'); authOverlay.classList.remove('hidden'); currentUserId = null; currentUsername = ""; authLog.textContent = ""; setAuthMode(false); }
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

window.onload = function () {
  if (typeof google !== 'undefined') { google.accounts.id.initialize({ client_id: "824007922604-pqvviecs5t3cf377s4o8qrirhsdl8gho.apps.googleusercontent.com", callback: handleGoogleAuth }); }
};

document.querySelector('.social-btn.google').addEventListener('click', (e) => {
  e.preventDefault();
  if (typeof google !== 'undefined') { google.accounts.id.prompt((notification) => { if (notification.isNotDisplayed() || notification.isSkippedMoment()) { authLog.textContent = 'Google popup blocked.'; authLog.style.color = '#ffb84d'; } }); }
});

document.querySelector('.social-btn.facebook').addEventListener('click', (e) => { e.preventDefault(); authLog.textContent = 'Facebook OAuth requires App ID.'; authLog.style.color = '#ffb84d'; });
