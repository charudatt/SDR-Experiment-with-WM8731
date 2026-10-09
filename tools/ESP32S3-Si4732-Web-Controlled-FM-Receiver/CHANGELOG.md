# Changelog

## Stage 1B — Standalone FM + Rotary + Web Control

- Added a browser-based interface to the independently validated Si4732 FM tuning test.
- Added frequency entry and SET, plus STEP + / STEP − controls.
- Added live status polling so the displayed frequency follows physical encoder tuning without a full-page reload.
- Retained rotary encoder tuning.
- Kept this sketch separate from the WM8731/FT8 host project.
- Hardware validation reported by the project owner: receiver initialization, tuning/readback, Wi-Fi, web page, web SET, step controls, and encoder-to-browser frequency updates passed.
- RF reception quality remains untested because no antenna was connected.
