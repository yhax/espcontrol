---
title: Screen Lock Cards
description:
  How to use screen lock cards on your EspControl panel to lock and unlock local touchscreen controls.
---

# Screen Lock

A Screen Lock card locks and unlocks the panel's touchscreen controls locally. It does not need a Home Assistant entity and does not send a Home Assistant action.

Use this when a panel is in a shared area and you want a quick way to prevent accidental taps.

## Setting Up a Screen Lock Card

1. Select a card and change its type to **Screen Lock**.
2. Save the card and apply the configuration.

Screen Lock does not need an entity or any additional card settings. Its label and icon change automatically to show whether the touchscreen is locked or unlocked.

## How It Works on the Panel

- Tapping the card switches the panel between locked and unlocked states.
- The lock state is local to the panel.
- Other cards are protected while the screen is locked.
- The card can be used on the home screen or inside a subpage.
- It does not depend on Home Assistant availability.

Screen Lock is different from a [Lock](/card-types/locks) card. **Lock** controls a Home Assistant `lock` entity such as a door lock. **Screen Lock** controls the touchscreen's local interaction state.

## Requiring a PIN to Unlock

By default, tapping a locked Screen Lock card unlocks it immediately. If you want more than a tap standing between a locked panel and its controls, set a 4-digit PIN from the panel's settings page:

1. Open the panel's settings page from a browser on your network.
2. Under **Display > Screen Lock PIN**, enter a PIN and confirm it, then save.

Once a PIN is set:

- Unlocking -- whether by tapping the Screen Lock card, waking the panel from the screensaver, or powering it on -- shows a keypad instead of unlocking right away. The keypad is a phone-style dial pad: 0-9, with 0 centered under the 3x3 grid of 1-9. Tapping a digit briefly highlights it in the panel's accent colour to confirm the tap registered.
- Entering the correct PIN dismisses the keypad and returns to the screen that was showing before the panel locked.
- Entering the wrong PIN flashes the keypad red for a moment and clears the attempt so you can try again.
- The locked state survives a reboot or a firmware update: if the panel was locked when it lost power, it comes back locked and asks for the PIN again.

Removing the PIN from the settings page returns Screen Lock to its original tap-to-unlock behaviour.

The PIN is stored on the panel as a salted hash, never as plain text, and it is never sent to Home Assistant or stored in the panel's backup/configuration data -- only the settings page can set or clear it. Because it is only a 4-digit PIN, it is meant to stop a passerby from casually poking at a shared panel, not to withstand someone with sustained physical access to the device.

## When to Use It

Screen Lock is useful for hallway panels, bedside panels, child-accessible panels, or any location where accidental control changes would be annoying.

For security-sensitive actions such as unlocking a door, use the Home Assistant lock's own security features, a Home Assistant script, or card-level confirmation where available. Screen Lock is a local interaction guard, not a replacement for Home Assistant permissions.
