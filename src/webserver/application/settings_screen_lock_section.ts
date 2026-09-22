import type { ControlsFieldsFeature } from "./controls_fields";
import type { ControlsShellFeature } from "./controls_shell";
import type { SettingsPageHelpersFeature } from "./settings_page_helpers";
import type { ScreenLockPinPostApiFeature } from "./screen_lock_pin_post_api";

export interface SettingsScreenLockSectionFeature {
    buildScreenLockSettingsCard(...args: any[]): any;
}

// Digits 1-9 only, no 0: the on-device keypad is a 3x3 grid of just those
// digits, so a PIN containing 0 could never be entered on the panel itself.
const SCREEN_LOCK_PIN_PATTERN = /^[1-9]{4}$/;

export function createSettingsScreenLockSectionFeature(
    pinApi: ScreenLockPinPostApiFeature,
    fields: Pick<ControlsFieldsFeature, "fieldLabel" | "makeCollapsibleCard" | "textInput">,
    shell: Pick<ControlsShellFeature, "createActionButton">,
    helpers: Pick<SettingsPageHelpersFeature, "infoPanel">,
): SettingsScreenLockSectionFeature {
    const { fieldLabel, makeCollapsibleCard, textInput } = fields;
    const { createActionButton } = shell;
    const { infoPanel } = helpers;
    const { postScreenLockPinSet, postScreenLockPinClear, getScreenLockPinStatus } = pinApi;
    // ── Settings Screen Lock Section ────────────────────────────────────
    function buildScreenLockSettingsCard(this: any) {
        var body: any = document.createElement("div");
        body.appendChild(infoPanel("sp-screen-lock-pin-info",
            "Ask for a 4-digit PIN before the touchscreen unlocks after a Screen Lock card, the screensaver, or a power cycle. " +
            "This protects against accidental taps on a shared panel -- it is not a replacement for Home Assistant's own security."));

        var statusLine: any = document.createElement("div");
        statusLine.className = "sp-field";
        var statusText: any = document.createElement("span");
        statusText.textContent = "Checking…";
        statusLine.appendChild(statusText);
        body.appendChild(statusLine);

        var pinField: any = document.createElement("div");
        pinField.className = "sp-field";
        pinField.appendChild(fieldLabel("New PIN", "sp-set-screen-lock-pin"));
        var pinInput: any = textInput("sp-set-screen-lock-pin", "", "4 digits, 1-9");
        pinInput.type = "password";
        pinInput.inputMode = "numeric";
        pinInput.autocomplete = "off";
        pinInput.maxLength = 4;
        pinField.appendChild(pinInput);
        body.appendChild(pinField);

        var confirmField: any = document.createElement("div");
        confirmField.className = "sp-field";
        confirmField.appendChild(fieldLabel("Confirm PIN", "sp-set-screen-lock-pin-confirm"));
        var confirmInput: any = textInput("sp-set-screen-lock-pin-confirm", "", "4 digits, 1-9");
        confirmInput.type = "password";
        confirmInput.inputMode = "numeric";
        confirmInput.autocomplete = "off";
        confirmInput.maxLength = 4;
        confirmField.appendChild(confirmInput);
        body.appendChild(confirmField);

        var errorText: any = document.createElement("div");
        errorText.className = "sp-field";
        errorText.style.color = "#c62828";
        errorText.style.fontSize = "0.85em";
        body.appendChild(errorText);

        var actions: any = document.createElement("div");
        actions.className = "sp-backup-btns";
        var saveBtn: any = createActionButton("sp-backup-btn", "Save PIN");
        var removeBtn: any = createActionButton("sp-backup-btn", "Remove PIN");
        actions.appendChild(saveBtn);
        actions.appendChild(removeBtn);
        body.appendChild(actions);

        function refreshStatus(this: any) {
            getScreenLockPinStatus(function (isSet: boolean) {
                statusText.textContent = isSet
                    ? "A PIN is currently set."
                    : "No PIN is set — Screen Lock unlocks with a tap.";
                removeBtn.style.display = isSet ? "" : "none";
            });
        }

        saveBtn.addEventListener("click", function (this: any) {
            errorText.textContent = "";
            var pin: any = pinInput.value.trim();
            var confirmValue: any = confirmInput.value.trim();
            if (!SCREEN_LOCK_PIN_PATTERN.test(pin)) {
                errorText.textContent = "PIN must be exactly 4 digits, each between 1 and 9.";
                return;
            }
            if (pin !== confirmValue) {
                errorText.textContent = "PINs do not match.";
                return;
            }
            postScreenLockPinSet(pin).then(function (this: any) {
                pinInput.value = "";
                confirmInput.value = "";
                refreshStatus();
            });
        });

        removeBtn.addEventListener("click", function (this: any) {
            if (!window.confirm("Remove the Screen Lock PIN? The screen will unlock with a tap again."))
                return;
            postScreenLockPinClear().then(function (this: any) {
                pinInput.value = "";
                confirmInput.value = "";
                refreshStatus();
            });
        });

        removeBtn.style.display = "none";
        refreshStatus();

        return makeCollapsibleCard("Screen Lock PIN", body, true);
    }
    return {
        buildScreenLockSettingsCard,
    };
}
