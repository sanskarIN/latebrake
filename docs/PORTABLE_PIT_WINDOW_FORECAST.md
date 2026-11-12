# Portable pit-stop window forecast

\`Racing/PortablePitWindowForecast.h\` is a source-only C++20 numerical
foundation to compare an optional or externally mandatory pit stop against
no stop, from **user-supplied** future lap-time and wear-rate parameters.

Each candidate pit interval is evaluated with arithmetic-series wear,
bounded by a 150-lap maximum and 604800 seconds. An optional pit that
cannot improve the forecast is never recommended. A mandatory pit
reports a negative benefit rather than claiming fake speed gains.

The independent strict portable CTest exercises minimum-lap eligibility,
zero-loss ties, narrow windows, overflow/nonfinite input, larger pit costs
and high-lap-count deterministic results. No actual UE race service,
fuel, tire wear, pit timing, garage, lap score, save data or AI control
is changed. **A model-based time suggestion is not an authorized pit
transaction or playable feature.** Real integration requires authentic
per-lap measurements and Windows UE5.8 game QA.
