# SmartCharge Twin (3D dashboard)
Open `index.html` in Chrome or Edge. No install needed (needs internet once for the 3D library and fonts).

- Simulation: pick a strategy, press Start charging, or press Compare all 3.
- Manual: sliders act as the potentiometer and temperature sensor. The adaptive controller picks the current.
- ESP32: live readings from the board. The ESP32 must serve GET /data returning JSON
  {"soc":0.4,"temp":31.2,"current":4.1} with header Access-Control-Allow-Origin: *.
  Open the page from your own computer (not a shared https link), because browsers block http calls from https pages.

The model equations match src/battery_model.py and src/strategies.py. Keep them in sync if you change one.
Parameters are toy values. The stress score is an estimate, not measured ageing.
