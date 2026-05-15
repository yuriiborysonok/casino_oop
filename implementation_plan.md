# Dual Currency System Implementation Plan (Gold & Sweep Coins)

This plan outlines the steps to transition the Casino Platform from a single "balance" to a dual-currency system: **Gold Coins (GC)** and **Sweep Coins (SC)**. 

## Goal
- Give users 75,000 GC and 2 SC upon registration.
- Add a toggle switch in the UI to seamlessly switch between playing with GC or SC.
- Update the database schema, C++ interfaces, and API endpoints to track and process bets for the specific currency.

## User Review Required
> [!WARNING]
> **Database Migration:** 
> The `users` table currently has a single `balance` column. To prevent breaking existing data, I will add `balance_gold` and `balance_sweep` via `ALTER TABLE`. The old `balance` column will be ignored. If you prefer a completely fresh database, let me know, and I can `DROP` the tables instead.
>
> **Transactions:** 
> I will add a `currency` column to the `transactions` table to track whether a win/loss was in GC or SC.

## Proposed Changes

---

### Backend: Database & Interfaces

#### [MODIFY] `include/IDatabase.hpp`
- Change `virtual double getBalance(int userId) = 0;` to `virtual std::pair<double, double> getBalances(int userId) = 0;` (Returns {gold, sweep}).
- Change `virtual void saveTransaction(...)` to accept a `currency` parameter (e.g., "gold" or "sweep").

#### [MODIFY] `include/PostgresDatabase.hpp`
- Update `CREATE TABLE users` to use `balance_gold DEFAULT 75000.0` and `balance_sweep DEFAULT 2.0`.
- Add automatic `ALTER TABLE` statements to migrate existing databases safely.
- Update `registerUser` and `socialLoginUser` to initialize with 75000 GC and 2 SC instead of 1000.
- Implement the new `getBalances` and updated `saveTransaction`.

---

### Backend: API Endpoints

#### [MODIFY] `src/main.cpp`
- **Auth Endpoints:** `/api/register`, `/api/login`, `/api/social_login` will now return both `balance_gold` and `balance_sweep` in their JSON responses.
- **Spin Endpoint:** `/api/spin` will accept a new field `"currency": "gold" | "sweep"`. 
  - It will validate the bet against the specific currency.
  - It will calculate wins and save the transaction under the chosen currency.
  - It will return `new_balance_gold` and `new_balance_sweep`.

---

### Frontend: UI & Logic

#### [MODIFY] `frontend/index.html`
- In the top header (where the balance is displayed), add a stylish **Toggle Switch** (e.g., a pill-shaped radio toggle) to switch between Gold Coins and Sweep Coins.
- Add a visual indicator next to the balance (e.g., a yellow `G` or a blue `S` shield) depending on the active currency.

#### [MODIFY] `frontend/style.css`
- Style the GC/SC toggle switch (glassmorphism/premium look).
- Style the Gold and Sweep coin icons in the balance display.

#### [MODIFY] `frontend/main.js`
- Track `currentCurrency = 'gold'` (default).
- Update the UI balance whenever the toggle is switched.
- Pass `currency: currentCurrency` in the POST request to `/api/spin`.
- Handle the updated auth responses to store both balances locally.

## Verification Plan
### Automated & Manual Verification
1. Recompile the C++ server (`make`).
2. Register a new user and verify they receive exactly 75,000 GC and 2 SC.
3. Toggle to Sweep Coins, place a 1 SC bet on Roulette, and spin.
4. Verify that only the Sweep Coin balance decreases, while Gold Coins remain untouched.
5. Check the PostgreSQL database to ensure transactions are accurately logged with their respective currency.
