# Dual Currency System Task List

## Backend: Database & Interfaces
- `[ ]` Update `include/IDatabase.hpp` to support `getBalances` and `saveTransaction` with currency types.
- `[ ]` Update `include/PostgresDatabase.hpp` schema creation for `balance_gold` and `balance_sweep`.
- `[ ]` Add `ALTER TABLE` logic in `PostgresDatabase.hpp` for safe migration.
- `[ ]` Update `registerUser` and `socialLoginUser` to initialize 75000 GC and 2 SC.
- `[ ]` Update `transactions` table to include a `currency` column.

## Backend: API Endpoints (`src/main.cpp`)
- `[ ]` Update `/api/register`, `/api/login`, `/api/social_login` responses to return `balance_gold` and `balance_sweep`.
- `[ ]` Update `/api/balance` to return both balances.
- `[ ]` Refactor `/api/spin` to accept `"currency"` parameter and process bets using the selected currency balance.
- `[ ]` Update `/api/spin` to log transactions properly with the selected currency.

## Frontend: UI & Logic
- `[ ]` Add Gold/Sweep toggle switch UI to `frontend/index.html`.
- `[ ]` Add CSS styles for the toggle switch in `frontend/style.css`.
- `[ ]` Update `frontend/main.js` state to track `currentCurrency`.
- `[ ]` Update `frontend/main.js` to render the correct balance and send `currency` flag to backend API.
