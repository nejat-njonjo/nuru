pragma Singleton

import QtQuick
import Nuru

// Centralised design tokens. Every colour, size and duration used by the UI
// comes from here so the dark/light themes and accent colour stay consistent.
QtObject {
    readonly property bool dark: AppSettings.theme !== "light"

    // Colours
    readonly property color accent: AppSettings.accentColor
    readonly property color accentHover: Qt.lighter(accent, 1.12)
    readonly property color accentSoft: Qt.rgba(accent.r, accent.g, accent.b, dark ? 0.16 : 0.14)
    readonly property color accentText: "#06120A"

    readonly property color background: dark ? "#0A0A0A" : "#F6F6F7"
    readonly property color surface: dark ? "#141414" : "#FFFFFF"
    readonly property color elevated: dark ? "#1C1C1C" : "#EDEDF0"
    readonly property color hover: dark ? "#222222" : "#E6E6EA"
    readonly property color pressed: dark ? "#2A2A2A" : "#DCDCE1"
    readonly property color border: dark ? "#292929" : "#DEDEE3"
    readonly property color track: dark ? "#3A3A3A" : "#CFCFD6"

    readonly property color textPrimary: dark ? "#FAFAFA" : "#111113"
    readonly property color textSecondary: dark ? "#A1A1AA" : "#52525B"
    readonly property color textMuted: "#71717A"

    readonly property color danger: "#F25F5C"
    readonly property color scrim: Qt.rgba(0, 0, 0, 0.55)

    readonly property var accentChoices: ["#41C261", "#3B82F6", "#A855F7", "#F97316", "#EC4899", "#14B8A6", "#EAB308"]

    // Spacing
    readonly property int spaceXs: 4
    readonly property int spaceS: 8
    readonly property int spaceM: 12
    readonly property int spaceL: 16
    readonly property int spaceXl: 24
    readonly property int spaceXxl: 32

    // Radii
    readonly property int radiusS: 6
    readonly property int radiusM: 10
    readonly property int radiusL: 14
    readonly property int radiusXl: 20

    // Typography
    // (system UI font; sizes only)
    readonly property int fontCaption: 11
    readonly property int fontBody: 13
    readonly property int fontBodyLarge: 14
    readonly property int fontTitle: 17
    readonly property int fontHeading: 24
    readonly property int fontDisplay: 34

    // Layout
    readonly property int sidebarWidth: 240
    readonly property int playbackBarHeight: 92
    readonly property int pageMargin: 32
    readonly property int rowHeight: 56

    // Motion
    readonly property int durationFast: 120
    readonly property int durationNormal: 200
    readonly property int durationSlow: 320

    function formatDuration(ms) {
        if (!ms || ms <= 0)
            return "–:––"
        const total = Math.floor(ms / 1000)
        const h = Math.floor(total / 3600)
        const m = Math.floor((total % 3600) / 60)
        const s = total % 60
        const ss = s < 10 ? "0" + s : "" + s
        if (h > 0)
            return h + ":" + (m < 10 ? "0" + m : m) + ":" + ss
        return m + ":" + ss
    }

    function formatTotal(ms) {
        const minutes = Math.round(ms / 60000)
        if (minutes < 60)
            return minutes + " min"
        return Math.floor(minutes / 60) + " h " + (minutes % 60) + " min"
    }

    // Deterministic placeholder hue for items without artwork.
    function placeholderColor(seed) {
        let hash = 0
        const text = seed || ""
        for (let i = 0; i < text.length; ++i)
            hash = (hash * 31 + text.charCodeAt(i)) & 0xffffff
        return Qt.hsla((hash % 360) / 360, 0.35, dark ? 0.22 : 0.78, 1)
    }
}
