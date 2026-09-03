/**
 * @fileoverview This file contains the configuration for the camera.
 * @author Sebastian Romero
 */

const CAMERA_CONFIG_STORAGE_KEY = 'webserial-camera-config-v1';

const DEFAULT_CAMERA_CONFIG = {
    resolutionKey: '1',
    modeKey: '2'
};

function getStoredCameraConfig() {
    try {
        const raw = localStorage.getItem(CAMERA_CONFIG_STORAGE_KEY);
        if (!raw) return { ...DEFAULT_CAMERA_CONFIG };
        const parsed = JSON.parse(raw);
        if (!parsed || typeof parsed !== 'object') return { ...DEFAULT_CAMERA_CONFIG };
        return {
            resolutionKey: String(parsed.resolutionKey ?? DEFAULT_CAMERA_CONFIG.resolutionKey),
            modeKey: String(parsed.modeKey ?? DEFAULT_CAMERA_CONFIG.modeKey)
        };
    } catch (error) {
        console.warn('⚠️ Unable to read camera settings from localStorage:', error);
        return { ...DEFAULT_CAMERA_CONFIG };
    }
}

function persistCameraConfig(config) {
    localStorage.setItem(CAMERA_CONFIG_STORAGE_KEY, JSON.stringify(config));
}

function getCameraConfig() {
    const config = getStoredCameraConfig();
    const resolution = CAMERA_RESOLUTIONS[config.resolutionKey];
    const mode = CAMERA_MODES[config.modeKey];
    if (!resolution || !mode) {
        return {
            ...DEFAULT_CAMERA_CONFIG,
            width: CAMERA_RESOLUTIONS[DEFAULT_CAMERA_CONFIG.resolutionKey].width,
            height: CAMERA_RESOLUTIONS[DEFAULT_CAMERA_CONFIG.resolutionKey].height,
            mode: CAMERA_MODES[DEFAULT_CAMERA_CONFIG.modeKey]
        };
    }

    return {
        resolutionKey: String(config.resolutionKey),
        modeKey: String(config.modeKey),
        width: resolution.width,
        height: resolution.height,
        mode: mode,
        bytesPerPixel: mode === 'GRAYSCALE' ? 1 : 2,
        frameBytes: resolution.width * resolution.height * (mode === 'GRAYSCALE' ? 1 : 2)
    };
}

/**
 * The available camera (color) modes.
 * The Arduino sketch uses the same values to communicate which mode should be used.
 **/
const CAMERA_MODES = {
    0: "GRAYSCALE",
    1: "BAYER",
    2: "RGB565"
};

/**
 * The available camera resolutions.
 * The Arduino sketch uses the same values to communicate which resolution should be used.
 */
const CAMERA_RESOLUTIONS = {
    0: {
        "name": "QQVGA",
        "width": 160,
        "height": 120
    },
    1: {
        "name": "QVGA",
        "width": 320,
        "height": 240
    },
    2: {
        "name": "320x320",
        "width": 320,
        "height": 320
    },
    3: {
        "name": "VGA",
        "width": 640,
        "height": 480
    },
    5: {
        "name": "SVGA",
        "width": 800,
        "height": 600
    },
    6: {
        "name": "UXGA",
        "width": 1600,
        "height": 1200
    }
};