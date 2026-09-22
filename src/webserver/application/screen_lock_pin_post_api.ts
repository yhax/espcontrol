import type { ApplicationApiFeature } from "./api";

export interface ScreenLockPinPostApiFeature {
    postScreenLockPinSet(pin?: string): Promise<any>;
    postScreenLockPinClear(): Promise<any>;
    getScreenLockPinStatus(callback: (isSet: boolean) => void): void;
}

// Talks to a small local-only endpoint on the panel itself (see
// screen_lock_pin_endpoint.h), not a Home Assistant entity: the PIN never
// becomes part of the panel configuration document or the entity registry,
// so a browser reading the panel's normal settings state can never read it
// back out. Only "is a PIN currently set" is ever exposed.
export function createScreenLockPinPostApiFeature(
    requestApi: Pick<ApplicationApiFeature, "post" | "getJsonQuietly">,
): ScreenLockPinPostApiFeature {
    // ── Screen Lock PIN Post API ────────────────────────────────────────
    function postScreenLockPinSet(this: any, pin?: any) {
        return requestApi.post("/api/v1/screen_lock_pin/set?pin=" + encodeURIComponent(pin));
    }
    function postScreenLockPinClear(this: any) {
        return requestApi.post("/api/v1/screen_lock_pin/clear");
    }
    function getScreenLockPinStatus(this: any, callback: (isSet: boolean) => void) {
        requestApi.getJsonQuietly("/api/v1/screen_lock_pin/status", function (this: any, data?: any) {
            callback(!!(data && data.is_set));
        });
    }
    return {
        postScreenLockPinSet,
        postScreenLockPinClear,
        getScreenLockPinStatus,
    };
}
