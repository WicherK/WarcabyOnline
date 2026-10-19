# Warcaby Online

A 3D multiplayer checkers game built with Unreal Engine and C++20 as a university project at semester 4. It features online matchmaking over WAN, Steam integration for progress tracking, and classic checkers rules set in a tavern-themed environment.

---

## Media & Documentation

* **Gameplay Video:** [Watch Project Showcase / Demo](https://www.youtube.com/watch?v=CBwt97jMAmA)
* **Project Documentation:** [View Full Report / Spec]()

---

## Features

### Gameplay & Rules
* **Classic Rules:** Standard checkers rules with mandatory captures (both forward and backward).
* **Promotions:** Pawns are automatically promoted to Kings upon reaching the opponent's back row.
* **Turn Timers:** Each player has 5 minutes per match, tracked by an in-game table timer.
* **End Conditions:** Matches end by complete piece elimination, running out of time, or surrender.
* **Progression System:** Players earn or lose XP and ranking points based on match results.

### Multiplayer & Integration
* **WAN Matchmaking:** Pressing Play searches for an open game. If none is found, the player automatically hosts a session for others to join.
* **Steam Integration:** Requires an active Steam client to authenticate players and save progress, stats, and rank to the database.

### Menu & Interface
* **3D Tavern Scene:** 3D main menu with ambient background music.
* **Player Dashboard:** UI panel displaying saved player progress.
* **Persistent Settings:** Audio and volume controls are saved to disk and reloaded on launch.