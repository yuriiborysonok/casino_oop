# Dual Currency System Walkthrough

The platform has successfully been migrated to a fully functional Dual Currency System consisting of **Gold Coins (GC)** and **Sweep Coins (SC)**!

## Database Migration & Schema
- Added `balance_gold` (default 75,000) and `balance_sweep` (default 2) columns to the `users` table via safe `ALTER TABLE` commands so existing users won't be deleted.
- Added a `currency` column to the `transactions` table to accurately track exactly what type of coin was bet and won.
- `PostgresDatabase.hpp` and `IDatabase.hpp` were fully refactored to handle `std::pair<double, double>` for balances and pass `currency` during transactions.

## API Endpoints Update
- **Auth Endpoints:** Registration and login endpoints (`/api/register`, `/api/login`, `/api/social_login`) now return `balance_gold` and `balance_sweep` simultaneously.
- **Spin Engine (`/api/spin`):** Now accepts a `currency` parameter (e.g. "gold" or "sweep"). The backend dynamically resolves the correct balance, validates if the user has enough of the selected currency, and updates only that specific balance on win/loss.

## Frontend Experience
- **Sleek Toggle Switch:** Added a premium toggle switch in the navbar to swap between Gold and Sweep coins seamlessly. It's animated and styled distinctly (Yellow gradient for Gold, Blue gradient for Sweep).
- **Dynamic Interface:** When you toggle currencies, the UI dynamically changes the balance amount and the shield icon color to match the selected currency.
- **Game Integration:** Roulette spins now accurately pull from the selected currency and check for sufficient funds of *that specific currency* before spinning.

**Everything is compiled and running. You can now register a new user to instantly see your 75,000 GC and 2 SC balance!**
