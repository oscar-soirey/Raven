/**
 * Raven - widget/style.c
 * The built-in themes, and how a widget finds its colors and metrics: its
 * own overrides, then the styles of its theme, then the theme. Named themes,
 * styles and theme files are in theme.c.
 */

#include "widget.h"
#include "../render/renderer.h"

#include <math.h>

#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#define RVN_COLOR_INDEX(role) ((int)(role) - (int)R_COLOR_WINDOW)
#define RVN_METRIC_INDEX(metric) ((int)(metric) - (int)R_METRIC_FONT_SIZE)

static int rvn_color_valid(RColorRole role) {
	return role >= R_COLOR_WINDOW && role < R_COLOR_LAST_;
}

static int rvn_metric_valid(RMetric metric) {
	return metric >= R_METRIC_FONT_SIZE && metric < R_METRIC_LAST_;
}

/* ---------------------------------------------------------------------------
 *  Built-in themes
 * ------------------------------------------------------------------------- */

typedef struct rvn_palette_entry_t {
	RColorRole role;
	uint32_t rgb;
	uint32_t alpha;
} rvn_palette_entry_t;

static const rvn_palette_entry_t g_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x1E1E20, 255 },
	{ R_COLOR_PANEL,            0x27272A, 255 },
	{ R_COLOR_BORDER,           0x3F3F46, 255 },
	{ R_COLOR_TEXT,             0xD8D8DC, 255 },
	{ R_COLOR_TEXT_DISABLED,    0x6C6C74, 255 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x7C7C86, 255 },
	{ R_COLOR_ACCENT,           0x3B82F6, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x5A9AFA, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x2B6CD8, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0x35353A, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0x424248, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0x2B2B30, 255 },
	{ R_COLOR_CONTROL_BORDER,   0x4C4C54, 255 },
	{ R_COLOR_INPUT,            0x161618, 255 },
	{ R_COLOR_FOCUS,            0x3B82F6, 255 },
	{ R_COLOR_SELECTION,        0x274B7A, 255 },
	{ R_COLOR_ROW_HOVERED,      0x2E2E33, 255 },
	{ R_COLOR_TRACK,            0x2C2C30, 255 },
	{ R_COLOR_HANDLE,           0x5C5C66, 255 },
	{ R_COLOR_HANDLE_HOVERED,   0x7C7C88, 255 },
	{ R_COLOR_SEPARATOR,        0x3F3F46, 255 },
	{ R_COLOR_HEADER,           0x2D2D31, 255 },
	{ R_COLOR_HEADER_HOVERED,   0x3A3A40, 255 },
	{ R_COLOR_TAB_ACTIVE,       0x1E1E20, 255 },
	{ R_COLOR_POPUP,            0x2B2B2F, 255 },
	{ R_COLOR_POPUP_BORDER,     0x4A4A52, 255 },
	{ R_COLOR_SHADOW,           0x000000, 120 },
	{ R_COLOR_TOOLTIP,          0x3C3C42, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0xE6E6EA, 255 },
	{ R_COLOR_SWITCH,           0x3B82F6, 255 },
	{ R_COLOR_SUCCESS,          0x3FB950, 255 },
	{ R_COLOR_WARNING,          0xD29922, 255 },
	{ R_COLOR_DANGER,           0xF85149, 255 },
	{ R_COLOR_LINK,             0x6CB0FF, 255 },
	{ R_COLOR_CODE,             0x141416, 255 }
};

/* High contrast: every pair of text and background above 7:1 (WCAG AAA). */
static const rvn_palette_entry_t g_hc_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x000000, 255 },
	{ R_COLOR_PANEL,            0x000000, 255 },
	{ R_COLOR_BORDER,           0xFFFFFF, 255 },
	{ R_COLOR_TEXT,             0xFFFFFF, 255 },
	{ R_COLOR_TEXT_DISABLED,    0x3FF23F, 255 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0xC0C0C0, 255 },
	{ R_COLOR_ACCENT,           0x1AEBFF, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x7DF3FF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x00B3C4, 255 },
	{ R_COLOR_ACCENT_TEXT,      0x000000, 255 },
	{ R_COLOR_CONTROL,          0x000000, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0x1A1A1A, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0x333333, 255 },
	{ R_COLOR_CONTROL_BORDER,   0xFFFFFF, 255 },
	{ R_COLOR_INPUT,            0x000000, 255 },
	{ R_COLOR_FOCUS,            0xFFFF00, 255 },
	{ R_COLOR_SELECTION,        0x1AEBFF, 140 },
	{ R_COLOR_ROW_HOVERED,      0x262626, 255 },
	{ R_COLOR_TRACK,            0x000000, 255 },
	{ R_COLOR_HANDLE,           0xFFFFFF, 255 },
	{ R_COLOR_HANDLE_HOVERED,   0xFFFF00, 255 },
	{ R_COLOR_SEPARATOR,        0xFFFFFF, 255 },
	{ R_COLOR_HEADER,           0x000000, 255 },
	{ R_COLOR_HEADER_HOVERED,   0x1A1A1A, 255 },
	{ R_COLOR_TAB_ACTIVE,       0x1AEBFF, 255 },
	{ R_COLOR_POPUP,            0x000000, 255 },
	{ R_COLOR_POPUP_BORDER,     0xFFFFFF, 255 },
	{ R_COLOR_SHADOW,           0x000000, 0 },
	{ R_COLOR_TOOLTIP,          0x000000, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0xFFFFFF, 255 },
	{ R_COLOR_SWITCH,           0x1AEBFF, 255 },
	{ R_COLOR_SUCCESS,          0x3FF23F, 255 },
	{ R_COLOR_WARNING,          0xFFFF00, 255 },
	{ R_COLOR_DANGER,           0xFF6B6B, 255 },
	{ R_COLOR_LINK,             0xFFFF00, 255 },
	{ R_COLOR_CODE,             0x000000, 255 }
};

static const rvn_palette_entry_t g_hc_light_palette[] = {
	{ R_COLOR_WINDOW,           0xFFFFFF, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 255 },
	{ R_COLOR_BORDER,           0x000000, 255 },
	{ R_COLOR_TEXT,             0x000000, 255 },
	{ R_COLOR_TEXT_DISABLED,    0x600000, 255 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x404040, 255 },
	{ R_COLOR_ACCENT,           0x1C00A8, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x2D14D0, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x10006B, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0xE6E6E6, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0xCCCCCC, 255 },
	{ R_COLOR_CONTROL_BORDER,   0x000000, 255 },
	{ R_COLOR_INPUT,            0xFFFFFF, 255 },
	{ R_COLOR_FOCUS,            0x1C00A8, 255 },
	{ R_COLOR_SELECTION,        0x37006E, 90 },
	{ R_COLOR_ROW_HOVERED,      0xE6E6E6, 255 },
	{ R_COLOR_TRACK,            0xFFFFFF, 255 },
	{ R_COLOR_HANDLE,           0x000000, 255 },
	{ R_COLOR_HANDLE_HOVERED,   0x1C00A8, 255 },
	{ R_COLOR_SEPARATOR,        0x000000, 255 },
	{ R_COLOR_HEADER,           0xFFFFFF, 255 },
	{ R_COLOR_HEADER_HOVERED,   0xE6E6E6, 255 },
	{ R_COLOR_TAB_ACTIVE,       0x1C00A8, 255 },
	{ R_COLOR_POPUP,            0xFFFFFF, 255 },
	{ R_COLOR_POPUP_BORDER,     0x000000, 255 },
	{ R_COLOR_SHADOW,           0x000000, 0 },
	{ R_COLOR_TOOLTIP,          0xFFFFFF, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0x000000, 255 },
	{ R_COLOR_SWITCH,           0x1C00A8, 255 },
	{ R_COLOR_SUCCESS,          0x005A00, 255 },
	{ R_COLOR_WARNING,          0x6B4A00, 255 },
	{ R_COLOR_DANGER,           0xA30000, 255 },
	{ R_COLOR_LINK,             0x00009F, 255 },
	{ R_COLOR_CODE,             0xFFFFFF, 255 }
};

static const rvn_palette_entry_t g_light_palette[] = {
	{ R_COLOR_WINDOW,           0xF3F3F5, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 255 },
	{ R_COLOR_BORDER,           0xD0D0D6, 255 },
	{ R_COLOR_TEXT,             0x1F1F23, 255 },
	{ R_COLOR_TEXT_DISABLED,    0xA0A0A8, 255 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x90909A, 255 },
	{ R_COLOR_ACCENT,           0x2563EB, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x3B7BF5, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x1D4FC4, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xE7E7EB, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0xDBDBE1, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0xCBCBD3, 255 },
	{ R_COLOR_CONTROL_BORDER,   0xC2C2CA, 255 },
	{ R_COLOR_INPUT,            0xFFFFFF, 255 },
	{ R_COLOR_FOCUS,            0x2563EB, 255 },
	{ R_COLOR_SELECTION,        0xBBD4FB, 255 },
	{ R_COLOR_ROW_HOVERED,      0xE9EEF6, 255 },
	{ R_COLOR_TRACK,            0xDDDDE2, 255 },
	{ R_COLOR_HANDLE,           0xA9A9B2, 255 },
	{ R_COLOR_HANDLE_HOVERED,   0x8A8A94, 255 },
	{ R_COLOR_SEPARATOR,        0xD0D0D6, 255 },
	{ R_COLOR_HEADER,           0xE9E9ED, 255 },
	{ R_COLOR_HEADER_HOVERED,   0xDDDDE3, 255 },
	{ R_COLOR_TAB_ACTIVE,       0xF3F3F5, 255 },
	{ R_COLOR_POPUP,            0xFFFFFF, 255 },
	{ R_COLOR_POPUP_BORDER,     0xC8C8D0, 255 },
	{ R_COLOR_SHADOW,           0x000000, 60 },
	{ R_COLOR_TOOLTIP,          0x2B2B30, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0xF2F2F5, 255 },
	{ R_COLOR_SWITCH,           0x2563EB, 255 },
	{ R_COLOR_SUCCESS,          0x1A7F37, 255 },
	{ R_COLOR_WARNING,          0x9A6700, 255 },
	{ R_COLOR_DANGER,           0xCF222E, 255 },
	{ R_COLOR_LINK,             0x0B62D6, 255 },
	{ R_COLOR_CODE,             0xE6E6EB, 255 }
};

/*
 * Glass themes. Most surfaces are translucent: the alpha of a color is how
 * much of it is laid over what shows through. Glass surfaces use these colors
 * as their tint (CONTROL, ACCENT, INPUT, PANEL, POPUP: see widgets/common.c).
 */
static const rvn_palette_entry_t g_glass_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x2A2A2E, 255 },
	{ R_COLOR_PANEL,            0x3A3A3F, 110 },
	{ R_COLOR_BORDER,           0xFFFFFF, 34 },
	{ R_COLOR_TEXT,             0xF5F5F7, 255 },
	{ R_COLOR_TEXT_DISABLED,    0xEBEBF5, 80 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0xEBEBF5, 120 },
	{ R_COLOR_ACCENT,           0x0A84FF, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x409CFF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x0A6FDB, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 28 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 46 },
	{ R_COLOR_CONTROL_PRESSED,  0xFFFFFF, 66 },
	{ R_COLOR_CONTROL_BORDER,   0xFFFFFF, 60 },
	{ R_COLOR_INPUT,            0x1A1A1D, 90 },
	{ R_COLOR_FOCUS,            0x0A84FF, 210 },
	{ R_COLOR_SELECTION,        0x0A84FF, 130 },
	{ R_COLOR_ROW_HOVERED,      0xFFFFFF, 20 },
	{ R_COLOR_TRACK,            0xFFFFFF, 40 },
	{ R_COLOR_HANDLE,           0xFFFFFF, 90 },
	{ R_COLOR_HANDLE_HOVERED,   0xFFFFFF, 140 },
	{ R_COLOR_SEPARATOR,        0xFFFFFF, 30 },
	{ R_COLOR_HEADER,           0xFFFFFF, 16 },
	{ R_COLOR_HEADER_HOVERED,   0xFFFFFF, 30 },
	{ R_COLOR_TAB_ACTIVE,       0xFFFFFF, 52 },
	{ R_COLOR_POPUP,            0x333337, 190 },
	{ R_COLOR_POPUP_BORDER,     0xFFFFFF, 40 },
	{ R_COLOR_SHADOW,           0x000000, 110 },
	{ R_COLOR_TOOLTIP,          0x333337, 200 },
	{ R_COLOR_TOOLTIP_TEXT,     0xF5F5F7, 255 },
	{ R_COLOR_SWITCH,           0x30D158, 255 },
	{ R_COLOR_SUCCESS,          0x30D158, 255 },
	{ R_COLOR_WARNING,          0xFFD60A, 255 },
	{ R_COLOR_DANGER,           0xFF453A, 255 },
	{ R_COLOR_LINK,             0x6CB6FF, 255 },
	{ R_COLOR_CODE,             0x1A1A1D, 110 }
};

static const rvn_palette_entry_t g_glass_light_palette[] = {
	{ R_COLOR_WINDOW,           0xFFFFFF, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 105 },
	{ R_COLOR_BORDER,           0x3C3C43, 40 },
	{ R_COLOR_TEXT,             0x1C1C1E, 255 },
	{ R_COLOR_TEXT_DISABLED,    0x3C3C43, 90 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x3C3C43, 130 },
	{ R_COLOR_ACCENT,           0x007AFF, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x2E90FF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x0066D6, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 120 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 165 },
	{ R_COLOR_CONTROL_PRESSED,  0xFFFFFF, 205 },
	{ R_COLOR_CONTROL_BORDER,   0x3C3C43, 46 },
	{ R_COLOR_INPUT,            0xFFFFFF, 110 },
	{ R_COLOR_FOCUS,            0x007AFF, 210 },
	{ R_COLOR_SELECTION,        0x007AFF, 80 },
	{ R_COLOR_ROW_HOVERED,      0x3C3C43, 16 },
	{ R_COLOR_TRACK,            0x787880, 52 },
	{ R_COLOR_HANDLE,           0x3C3C43, 80 },
	{ R_COLOR_HANDLE_HOVERED,   0x3C3C43, 130 },
	{ R_COLOR_SEPARATOR,        0x3C3C43, 46 },
	{ R_COLOR_HEADER,           0x3C3C43, 12 },
	{ R_COLOR_HEADER_HOVERED,   0x3C3C43, 26 },
	{ R_COLOR_TAB_ACTIVE,       0xFFFFFF, 215 },
	{ R_COLOR_POPUP,            0xFAFAFA, 160 },
	{ R_COLOR_POPUP_BORDER,     0x3C3C43, 30 },
	{ R_COLOR_SHADOW,           0x000000, 46 },
	{ R_COLOR_TOOLTIP,          0xFAFAFA, 185 },
	{ R_COLOR_TOOLTIP_TEXT,     0x1C1C1E, 255 },
	{ R_COLOR_SWITCH,           0x34C759, 255 },
	{ R_COLOR_SUCCESS,          0x248A3D, 255 },
	{ R_COLOR_WARNING,          0xB25000, 255 },
	{ R_COLOR_DANGER,           0xD70015, 255 },
	{ R_COLOR_LINK,             0x0066CC, 255 },
	{ R_COLOR_CODE,             0x3C3C43, 24 }
};

/*
 * Windows 11 (the Fluent tokens of WinUI: TextFillColorPrimary,
 * ControlFillColorDefault, ControlStrokeColorDefault...). Many of them are
 * translucent: they are made to be laid over the background of the window
 * (Mica: #F3F3F3, #202020) or over a card. The window color is that base.
 */
static const rvn_palette_entry_t g_windows_light_palette[] = {
	{ R_COLOR_WINDOW,           0xF3F3F3, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 179 },  /* CardBackgroundFillColorDefault */
	{ R_COLOR_BORDER,           0x000000, 15 },   /* CardStrokeColorDefault */
	{ R_COLOR_TEXT,             0x1B1B1B, 255 },  /* TextFillColorPrimary (#E4000000) */
	{ R_COLOR_TEXT_DISABLED,    0x000000, 92 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x000000, 158 },  /* TextFillColorSecondary */
	{ R_COLOR_ACCENT,           0x005FB8, 255 },  /* AccentFillColorDefault (SystemAccentColorDark1) */
	{ R_COLOR_ACCENT_HOVERED,   0x005FB8, 230 },
	{ R_COLOR_ACCENT_PRESSED,   0x005FB8, 204 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 179 },  /* ControlFillColorDefault */
	{ R_COLOR_CONTROL_HOVERED,  0xF9F9F9, 128 },
	{ R_COLOR_CONTROL_PRESSED,  0xF9F9F9, 77 },
	{ R_COLOR_CONTROL_BORDER,   0x000000, 15 },   /* ControlStrokeColorDefault */
	{ R_COLOR_INPUT,            0xFFFFFF, 179 },
	{ R_COLOR_FOCUS,            0x000000, 228 },  /* FocusStrokeColorOuter */
	{ R_COLOR_SELECTION,        0x005FB8, 90 },
	{ R_COLOR_ROW_HOVERED,      0x000000, 9 },    /* SubtleFillColorSecondary */
	{ R_COLOR_TRACK,            0x000000, 114 },  /* ControlStrongFillColorDefault */
	{ R_COLOR_HANDLE,           0x000000, 114 },
	{ R_COLOR_HANDLE_HOVERED,   0x000000, 158 },
	{ R_COLOR_SEPARATOR,        0x000000, 15 },   /* DividerStrokeColorDefault */
	{ R_COLOR_HEADER,           0x000000, 0 },
	{ R_COLOR_HEADER_HOVERED,   0x000000, 9 },
	{ R_COLOR_TAB_ACTIVE,       0xF9F9F9, 255 },  /* SolidBackgroundFillColorTertiary */
	{ R_COLOR_POPUP,            0xF9F9F9, 255 },  /* the acrylic of flyouts, opaque */
	{ R_COLOR_POPUP_BORDER,     0x000000, 15 },   /* SurfaceStrokeColorFlyout */
	{ R_COLOR_SHADOW,           0x000000, 36 },
	{ R_COLOR_TOOLTIP,          0xF9F9F9, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0x1B1B1B, 255 },
	{ R_COLOR_SWITCH,           0x005FB8, 255 },
	{ R_COLOR_SUCCESS,          0x0F7B0F, 255 },
	{ R_COLOR_WARNING,          0x9D5D00, 255 },
	{ R_COLOR_DANGER,           0xC42B1C, 255 },
	{ R_COLOR_LINK,             0x003E92, 255 },  /* AccentTextFillColorPrimary */
	{ R_COLOR_CODE,             0x000000, 13 }
};

static const rvn_palette_entry_t g_windows_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x202020, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 13 },
	{ R_COLOR_BORDER,           0x000000, 25 },
	{ R_COLOR_TEXT,             0xFFFFFF, 255 },
	{ R_COLOR_TEXT_DISABLED,    0xFFFFFF, 93 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0xFFFFFF, 197 },
	{ R_COLOR_ACCENT,           0x60CDFF, 255 },  /* SystemAccentColorLight2 */
	{ R_COLOR_ACCENT_HOVERED,   0x60CDFF, 230 },
	{ R_COLOR_ACCENT_PRESSED,   0x60CDFF, 204 },
	{ R_COLOR_ACCENT_TEXT,      0x000000, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 15 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 21 },
	{ R_COLOR_CONTROL_PRESSED,  0xFFFFFF, 8 },
	{ R_COLOR_CONTROL_BORDER,   0xFFFFFF, 18 },
	{ R_COLOR_INPUT,            0xFFFFFF, 15 },
	{ R_COLOR_FOCUS,            0xFFFFFF, 255 },
	{ R_COLOR_SELECTION,        0x60CDFF, 80 },
	{ R_COLOR_ROW_HOVERED,      0xFFFFFF, 15 },
	{ R_COLOR_TRACK,            0xFFFFFF, 139 },
	{ R_COLOR_HANDLE,           0xFFFFFF, 139 },
	{ R_COLOR_HANDLE_HOVERED,   0xFFFFFF, 197 },
	{ R_COLOR_SEPARATOR,        0xFFFFFF, 21 },
	{ R_COLOR_HEADER,           0x000000, 0 },
	{ R_COLOR_HEADER_HOVERED,   0xFFFFFF, 15 },
	{ R_COLOR_TAB_ACTIVE,       0x282828, 255 },
	{ R_COLOR_POPUP,            0x2C2C2C, 255 },
	{ R_COLOR_POPUP_BORDER,     0x000000, 51 },
	{ R_COLOR_SHADOW,           0x000000, 66 },
	{ R_COLOR_TOOLTIP,          0x2C2C2C, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0xFFFFFF, 255 },
	{ R_COLOR_SWITCH,           0x60CDFF, 255 },
	{ R_COLOR_SUCCESS,          0x6CCB5F, 255 },
	{ R_COLOR_WARNING,          0xFCE100, 255 },
	{ R_COLOR_DANGER,           0xFF99A4, 255 },
	{ R_COLOR_LINK,             0x99EBFF, 255 },
	{ R_COLOR_CODE,             0xFFFFFF, 15 }
};

/*
 * macOS Sequoia (the colors of AppKit: windowBackgroundColor, labelColor,
 * controlAccentColor...). Controls do not change under the pointer.
 */
static const rvn_palette_entry_t g_macos_light_palette[] = {
	{ R_COLOR_WINDOW,           0xECECEC, 255 },  /* windowBackgroundColor */
	{ R_COLOR_PANEL,            0xFFFFFF, 255 },  /* controlBackgroundColor */
	{ R_COLOR_BORDER,           0x000000, 26 },   /* separatorColor */
	{ R_COLOR_TEXT,             0x000000, 217 },  /* labelColor */
	{ R_COLOR_TEXT_DISABLED,    0x000000, 64 },   /* tertiaryLabelColor */
	{ R_COLOR_TEXT_PLACEHOLDER, 0x000000, 64 },   /* placeholderTextColor */
	{ R_COLOR_ACCENT,           0x007AFF, 255 },  /* controlAccentColor (blue) */
	{ R_COLOR_ACCENT_HOVERED,   0x007AFF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x0062CC, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0xE0E0E0, 255 },
	{ R_COLOR_CONTROL_BORDER,   0x000000, 38 },
	{ R_COLOR_INPUT,            0xFFFFFF, 255 },  /* textBackgroundColor */
	{ R_COLOR_FOCUS,            0x007AFF, 128 },  /* keyboardFocusIndicatorColor */
	{ R_COLOR_SELECTION,        0xB3D7FF, 255 },  /* selectedTextBackgroundColor */
	{ R_COLOR_ROW_HOVERED,      0x000000, 0 },
	{ R_COLOR_TRACK,            0x000000, 26 },
	{ R_COLOR_HANDLE,           0x000000, 128 },
	{ R_COLOR_HANDLE_HOVERED,   0x000000, 166 },
	{ R_COLOR_SEPARATOR,        0x000000, 26 },
	{ R_COLOR_HEADER,           0x000000, 0 },
	{ R_COLOR_HEADER_HOVERED,   0x000000, 15 },
	{ R_COLOR_TAB_ACTIVE,       0xFFFFFF, 255 },
	{ R_COLOR_POPUP,            0xF2F2F2, 255 },  /* the vibrancy of menus, opaque */
	{ R_COLOR_POPUP_BORDER,     0x000000, 38 },
	{ R_COLOR_SHADOW,           0x000000, 89 },
	{ R_COLOR_TOOLTIP,          0xF0F0F0, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0x000000, 217 },
	{ R_COLOR_SWITCH,           0x007AFF, 255 },
	{ R_COLOR_SUCCESS,          0x28CD41, 255 },
	{ R_COLOR_WARNING,          0xFF9500, 255 },
	{ R_COLOR_DANGER,           0xFF3B30, 255 },
	{ R_COLOR_LINK,             0x0068DA, 255 },  /* linkColor */
	{ R_COLOR_CODE,             0x000000, 13 }
};

static const rvn_palette_entry_t g_macos_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x323232, 255 },
	{ R_COLOR_PANEL,            0x1E1E1E, 255 },
	{ R_COLOR_BORDER,           0xFFFFFF, 26 },
	{ R_COLOR_TEXT,             0xFFFFFF, 217 },
	{ R_COLOR_TEXT_DISABLED,    0xFFFFFF, 64 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0xFFFFFF, 64 },
	{ R_COLOR_ACCENT,           0x0A84FF, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x0A84FF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x3D9BFF, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 64 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 64 },
	{ R_COLOR_CONTROL_PRESSED,  0xFFFFFF, 92 },
	{ R_COLOR_CONTROL_BORDER,   0xFFFFFF, 26 },
	{ R_COLOR_INPUT,            0xFFFFFF, 13 },
	{ R_COLOR_FOCUS,            0x0A84FF, 128 },
	{ R_COLOR_SELECTION,        0x3F638B, 255 },
	{ R_COLOR_ROW_HOVERED,      0xFFFFFF, 0 },
	{ R_COLOR_TRACK,            0xFFFFFF, 38 },
	{ R_COLOR_HANDLE,           0xFFFFFF, 128 },
	{ R_COLOR_HANDLE_HOVERED,   0xFFFFFF, 166 },
	{ R_COLOR_SEPARATOR,        0xFFFFFF, 26 },
	{ R_COLOR_HEADER,           0x000000, 0 },
	{ R_COLOR_HEADER_HOVERED,   0xFFFFFF, 20 },
	{ R_COLOR_TAB_ACTIVE,       0xFFFFFF, 51 },
	{ R_COLOR_POPUP,            0x2D2D2D, 255 },
	{ R_COLOR_POPUP_BORDER,     0xFFFFFF, 38 },
	{ R_COLOR_SHADOW,           0x000000, 128 },
	{ R_COLOR_TOOLTIP,          0x3A3A3A, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0xFFFFFF, 217 },
	{ R_COLOR_SWITCH,           0x0A84FF, 255 },
	{ R_COLOR_SUCCESS,          0x32D74B, 255 },
	{ R_COLOR_WARNING,          0xFFD60A, 255 },
	{ R_COLOR_DANGER,           0xFF453A, 255 },
	{ R_COLOR_LINK,             0x419CFF, 255 },
	{ R_COLOR_CODE,             0xFFFFFF, 20 }
};

/*
 * macOS 26 (Tahoe, Liquid Glass). The colors of macOS; the window a little
 * lighter, and the surfaces that are glass (bars, menus, tooltips) given as
 * the tint of the glass: what is behind shows through them.
 */
static const rvn_palette_entry_t g_macos_glass_light_palette[] = {
	{ R_COLOR_WINDOW,           0xF2F2F2, 255 },  /* windowBackgroundColor */
	{ R_COLOR_PANEL,            0xFFFFFF, 255 },  /* controlBackgroundColor; the tint of bars */
	{ R_COLOR_BORDER,           0x000000, 20 },   /* separatorColor */
	{ R_COLOR_TEXT,             0x000000, 217 },  /* labelColor */
	{ R_COLOR_TEXT_DISABLED,    0x000000, 64 },   /* tertiaryLabelColor */
	{ R_COLOR_TEXT_PLACEHOLDER, 0x000000, 64 },   /* placeholderTextColor */
	{ R_COLOR_ACCENT,           0x007AFF, 255 },  /* controlAccentColor (blue) */
	{ R_COLOR_ACCENT_HOVERED,   0x007AFF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x0062CC, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0xE4E4E4, 255 },
	{ R_COLOR_CONTROL_BORDER,   0x000000, 26 },
	{ R_COLOR_INPUT,            0xFFFFFF, 255 },  /* textBackgroundColor */
	{ R_COLOR_FOCUS,            0x007AFF, 128 },  /* keyboardFocusIndicatorColor */
	{ R_COLOR_SELECTION,        0xB3D7FF, 255 },  /* selectedTextBackgroundColor */
	{ R_COLOR_ROW_HOVERED,      0x000000, 0 },
	{ R_COLOR_TRACK,            0x000000, 26 },
	{ R_COLOR_HANDLE,           0x000000, 128 },
	{ R_COLOR_HANDLE_HOVERED,   0x000000, 166 },
	{ R_COLOR_SEPARATOR,        0x000000, 20 },
	{ R_COLOR_HEADER,           0x000000, 0 },
	{ R_COLOR_HEADER_HOVERED,   0x000000, 15 },
	{ R_COLOR_TAB_ACTIVE,       0xFFFFFF, 255 },
	{ R_COLOR_POPUP,            0xF7F7F7, 170 },  /* menus: regular glass */
	{ R_COLOR_POPUP_BORDER,     0xFFFFFF, 140 },
	{ R_COLOR_SHADOW,           0x000000, 64 },
	{ R_COLOR_TOOLTIP,          0xF7F7F7, 190 },
	{ R_COLOR_TOOLTIP_TEXT,     0x000000, 217 },
	{ R_COLOR_SWITCH,           0x007AFF, 255 },
	{ R_COLOR_SUCCESS,          0x28CD41, 255 },
	{ R_COLOR_WARNING,          0xFF9500, 255 },
	{ R_COLOR_DANGER,           0xFF3B30, 255 },
	{ R_COLOR_LINK,             0x0068DA, 255 },  /* linkColor */
	{ R_COLOR_CODE,             0x000000, 13 }
};

static const rvn_palette_entry_t g_macos_glass_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x262626, 255 },
	{ R_COLOR_PANEL,            0x1E1E1E, 255 },
	{ R_COLOR_BORDER,           0xFFFFFF, 26 },
	{ R_COLOR_TEXT,             0xFFFFFF, 217 },
	{ R_COLOR_TEXT_DISABLED,    0xFFFFFF, 64 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0xFFFFFF, 64 },
	{ R_COLOR_ACCENT,           0x0A84FF, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x0A84FF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x3D9BFF, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 46 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 46 },
	{ R_COLOR_CONTROL_PRESSED,  0xFFFFFF, 77 },
	{ R_COLOR_CONTROL_BORDER,   0xFFFFFF, 20 },
	{ R_COLOR_INPUT,            0xFFFFFF, 15 },
	{ R_COLOR_FOCUS,            0x0A84FF, 128 },
	{ R_COLOR_SELECTION,        0x3F638B, 255 },
	{ R_COLOR_ROW_HOVERED,      0xFFFFFF, 0 },
	{ R_COLOR_TRACK,            0xFFFFFF, 38 },
	{ R_COLOR_HANDLE,           0xFFFFFF, 128 },
	{ R_COLOR_HANDLE_HOVERED,   0xFFFFFF, 166 },
	{ R_COLOR_SEPARATOR,        0xFFFFFF, 26 },
	{ R_COLOR_HEADER,           0x000000, 0 },
	{ R_COLOR_HEADER_HOVERED,   0xFFFFFF, 20 },
	{ R_COLOR_TAB_ACTIVE,       0xFFFFFF, 51 },
	{ R_COLOR_POPUP,            0x2A2A2C, 175 },
	{ R_COLOR_POPUP_BORDER,     0xFFFFFF, 46 },
	{ R_COLOR_SHADOW,           0x000000, 115 },
	{ R_COLOR_TOOLTIP,          0x333335, 200 },
	{ R_COLOR_TOOLTIP_TEXT,     0xFFFFFF, 217 },
	{ R_COLOR_SWITCH,           0x0A84FF, 255 },
	{ R_COLOR_SUCCESS,          0x32D74B, 255 },
	{ R_COLOR_WARNING,          0xFFD60A, 255 },
	{ R_COLOR_DANGER,           0xFF453A, 255 },
	{ R_COLOR_LINK,             0x419CFF, 255 },
	{ R_COLOR_CODE,             0xFFFFFF, 20 }
};

/*
 * GNOME (the named colors of libadwaita: window_bg_color, view_fg_color,
 * accent_bg_color...). Controls are the color of the text, seen through
 * (alpha(currentColor, .1)), without borders.
 */
static const rvn_palette_entry_t g_gnome_light_palette[] = {
	{ R_COLOR_WINDOW,           0xFAFAFA, 255 },  /* window_bg_color */
	{ R_COLOR_PANEL,            0xFFFFFF, 255 },  /* card_bg_color */
	{ R_COLOR_BORDER,           0x000006, 18 },   /* card_shade_color */
	{ R_COLOR_TEXT,             0x000006, 204 },  /* window_fg_color */
	{ R_COLOR_TEXT_DISABLED,    0x000006, 102 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x000006, 112 },  /* .dim-label */
	{ R_COLOR_ACCENT,           0x3584E4, 255 },  /* accent_bg_color */
	{ R_COLOR_ACCENT_HOVERED,   0x4990E6, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x2A6AB6, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0x000006, 26 },   /* alpha(currentColor, .1) */
	{ R_COLOR_CONTROL_HOVERED,  0x000006, 38 },
	{ R_COLOR_CONTROL_PRESSED,  0x000006, 77 },
	{ R_COLOR_CONTROL_BORDER,   0x000006, 0 },
	{ R_COLOR_INPUT,            0x000006, 26 },
	{ R_COLOR_FOCUS,            0x1C71D8, 128 },  /* alpha(accent_color, .5) */
	{ R_COLOR_SELECTION,        0x3584E4, 64 },
	{ R_COLOR_ROW_HOVERED,      0x000006, 10 },
	{ R_COLOR_TRACK,            0x000006, 38 },
	{ R_COLOR_HANDLE,           0x000006, 51 },
	{ R_COLOR_HANDLE_HOVERED,   0x000006, 102 },
	{ R_COLOR_SEPARATOR,        0x000006, 26 },
	{ R_COLOR_HEADER,           0xEBEBED, 255 },  /* headerbar_bg_color */
	{ R_COLOR_HEADER_HOVERED,   0x000006, 26 },
	{ R_COLOR_TAB_ACTIVE,       0xFAFAFA, 255 },
	{ R_COLOR_POPUP,            0xFFFFFF, 255 },  /* popover_bg_color */
	{ R_COLOR_POPUP_BORDER,     0x000006, 36 },
	{ R_COLOR_SHADOW,           0x000000, 56 },
	{ R_COLOR_TOOLTIP,          0x000006, 204 },
	{ R_COLOR_TOOLTIP_TEXT,     0xFFFFFF, 255 },
	{ R_COLOR_SWITCH,           0x3584E4, 255 },
	{ R_COLOR_SUCCESS,          0x1B8553, 255 },
	{ R_COLOR_WARNING,          0x9C6E03, 255 },
	{ R_COLOR_DANGER,           0xE01B24, 255 },  /* destructive_bg_color */
	{ R_COLOR_LINK,             0x1C71D8, 255 },  /* accent_color */
	{ R_COLOR_CODE,             0x000006, 15 }
};

static const rvn_palette_entry_t g_gnome_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x222226, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 20 },
	{ R_COLOR_BORDER,           0x000006, 36 },
	{ R_COLOR_TEXT,             0xFFFFFF, 255 },
	{ R_COLOR_TEXT_DISABLED,    0xFFFFFF, 128 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0xFFFFFF, 140 },
	{ R_COLOR_ACCENT,           0x3584E4, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x4990E6, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x2A6AB6, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 26 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 38 },
	{ R_COLOR_CONTROL_PRESSED,  0xFFFFFF, 77 },
	{ R_COLOR_CONTROL_BORDER,   0xFFFFFF, 0 },
	{ R_COLOR_INPUT,            0xFFFFFF, 26 },
	{ R_COLOR_FOCUS,            0x78AEED, 128 },
	{ R_COLOR_SELECTION,        0x3584E4, 64 },
	{ R_COLOR_ROW_HOVERED,      0xFFFFFF, 10 },
	{ R_COLOR_TRACK,            0xFFFFFF, 38 },
	{ R_COLOR_HANDLE,           0xFFFFFF, 51 },
	{ R_COLOR_HANDLE_HOVERED,   0xFFFFFF, 102 },
	{ R_COLOR_SEPARATOR,        0xFFFFFF, 26 },
	{ R_COLOR_HEADER,           0x2E2E32, 255 },
	{ R_COLOR_HEADER_HOVERED,   0xFFFFFF, 26 },
	{ R_COLOR_TAB_ACTIVE,       0x222226, 255 },
	{ R_COLOR_POPUP,            0x36363A, 255 },
	{ R_COLOR_POPUP_BORDER,     0xFFFFFF, 20 },
	{ R_COLOR_SHADOW,           0x000000, 128 },
	{ R_COLOR_TOOLTIP,          0x000006, 204 },
	{ R_COLOR_TOOLTIP_TEXT,     0xFFFFFF, 255 },
	{ R_COLOR_SWITCH,           0x3584E4, 255 },
	{ R_COLOR_SUCCESS,          0x8FF0A4, 255 },
	{ R_COLOR_WARNING,          0xF8E45C, 255 },
	{ R_COLOR_DANGER,           0xFF7B63, 255 },
	{ R_COLOR_LINK,             0x78AEED, 255 },
	{ R_COLOR_CODE,             0xFFFFFF, 20 }
};

/* Same order as RMetric. */
static const float g_flat_metrics[R_METRIC_COUNT] = {
	13.0f, /* font size */
	4.0f,  /* corner radius */
	1.0f,  /* border width */
	26.0f, /* control height */
	10.0f, /* padding x */
	4.0f,  /* padding y */
	6.0f,  /* spacing */
	10.0f, /* scrollbar size */
	22.0f, /* row height */
	16.0f, /* indent */
	5.0f,  /* splitter size */
	16.0f, /* check size */
	14.0f, /* slider handle size */
	6.0f,  /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* High contrast: the flat metrics with thick borders. */
static const float g_hc_metrics[R_METRIC_COUNT] = {
	15.0f, /* font size */
	3.0f,  /* corner radius */
	2.0f,  /* border width */
	30.0f, /* control height */
	10.0f, /* padding x */
	5.0f,  /* padding y */
	6.0f,  /* spacing */
	14.0f, /* scrollbar size */
	26.0f, /* row height */
	18.0f, /* indent */
	6.0f,  /* splitter size */
	18.0f, /* check size */
	18.0f, /* slider handle size */
	3.0f,  /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* Larger, rounder, with more air: controls are capsules. */
static const float g_glass_metrics[R_METRIC_COUNT] = {
	14.0f, /* font size */
	12.0f, /* corner radius */
	1.0f,  /* border width */
	32.0f, /* control height */
	15.0f, /* padding x */
	6.0f,  /* padding y */
	8.0f,  /* spacing */
	10.0f, /* scrollbar size */
	30.0f, /* row height */
	18.0f, /* indent */
	7.0f,  /* splitter size */
	22.0f, /* check size */
	22.0f, /* slider handle size */
	16.0f, /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* Windows 11: the sizes of WinUI (ControlCornerRadius 4, OverlayCornerRadius 8, controls 32 high). */
static const float g_windows_metrics[R_METRIC_COUNT] = {
	14.0f, /* font size */
	4.0f,  /* corner radius */
	1.0f,  /* border width */
	32.0f, /* control height */
	11.0f, /* padding x */
	5.0f,  /* padding y */
	8.0f,  /* spacing */
	12.0f, /* scrollbar size */
	32.0f, /* row height */
	16.0f, /* indent */
	5.0f,  /* splitter size */
	20.0f, /* check size */
	20.0f, /* slider handle size */
	8.0f,  /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* macOS: regular controls, 22 high, SF at 13. */
static const float g_macos_metrics[R_METRIC_COUNT] = {
	13.0f, /* font size */
	5.0f,  /* corner radius */
	1.0f,  /* border width */
	22.0f, /* control height */
	10.0f, /* padding x */
	2.0f,  /* padding y */
	8.0f,  /* spacing */
	11.0f, /* scrollbar size */
	24.0f, /* row height */
	16.0f, /* indent */
	5.0f,  /* splitter size */
	14.0f, /* check size */
	20.0f, /* slider handle size */
	7.0f,  /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/*
 * macOS 26: the controls grew (medium ones 24 high) and rounded; menus and
 * popovers have large corners, rows of lists are taller. (Apple publishes no
 * sizes: these are measured on the system.)
 */
static const float g_macos_glass_metrics[R_METRIC_COUNT] = {
	13.0f, /* font size */
	8.0f,  /* corner radius */
	1.0f,  /* border width */
	24.0f, /* control height */
	12.0f, /* padding x */
	3.0f,  /* padding y */
	8.0f,  /* spacing */
	11.0f, /* scrollbar size */
	28.0f, /* row height */
	16.0f, /* indent */
	5.0f,  /* splitter size */
	14.0f, /* check size */
	16.0f, /* slider handle size (the knob is a capsule half as long again) */
	12.0f, /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* GNOME: controls 34 high, radius 6, popovers 12, the font at 11 points. */
static const float g_gnome_metrics[R_METRIC_COUNT] = {
	14.67f, /* font size */
	6.0f,  /* corner radius */
	1.0f,  /* border width */
	34.0f, /* control height */
	10.0f, /* padding x */
	5.0f,  /* padding y */
	6.0f,  /* spacing */
	12.0f, /* scrollbar size */
	34.0f, /* row height */
	18.0f, /* indent */
	5.0f,  /* splitter size */
	14.0f, /* check size */
	20.0f, /* slider handle size */
	12.0f, /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* ---------------------------------------------------------------------------
 *  The looks
 * ------------------------------------------------------------------------- */

typedef struct rvn_look_info_t {
	RLook look;
	/** Its name in theme files and for Rtheme_use, and the one of Raven 0.2 (or NULL). */
	const char* name;
	const char* old_name;
	int family;
	int dark;
	int glass;
	int contrast;
	const rvn_palette_entry_t* palette;
	size_t palette_count;
	const float* metrics;
} rvn_look_info_t;

#define RVN_PALETTE(p) p, RVN_COUNT_OF(p)

static const rvn_look_info_t g_looks[] = {
	{ R_LOOK_RAVEN_DARK, "RavenDark", "Dark", RVN_FAMILY_RAVEN, 1, 0, 0, RVN_PALETTE(g_dark_palette), g_flat_metrics },
	{ R_LOOK_RAVEN_LIGHT, "RavenLight", "Light", RVN_FAMILY_RAVEN, 0, 0, 0, RVN_PALETTE(g_light_palette), g_flat_metrics },
	{ R_LOOK_RAVEN_GLASS_DARK, "RavenGlassDark", "GlassDark", RVN_FAMILY_RAVEN, 1, 1, 0, RVN_PALETTE(g_glass_dark_palette), g_glass_metrics },
	{ R_LOOK_RAVEN_GLASS_LIGHT, "RavenGlassLight", "GlassLight", RVN_FAMILY_RAVEN, 0, 1, 0, RVN_PALETTE(g_glass_light_palette), g_glass_metrics },
	{ R_LOOK_RAVEN_HIGH_CONTRAST_DARK, "RavenHighContrastDark", "HighContrastDark", RVN_FAMILY_RAVEN, 1, 0, 1, RVN_PALETTE(g_hc_dark_palette), g_hc_metrics },
	{ R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT, "RavenHighContrastLight", "HighContrastLight", RVN_FAMILY_RAVEN, 0, 0, 1, RVN_PALETTE(g_hc_light_palette), g_hc_metrics },
	{ R_LOOK_WINDOWS_DARK, "WindowsDark", NULL, RVN_FAMILY_WINDOWS, 1, 0, 0, RVN_PALETTE(g_windows_dark_palette), g_windows_metrics },
	{ R_LOOK_WINDOWS_LIGHT, "WindowsLight", NULL, RVN_FAMILY_WINDOWS, 0, 0, 0, RVN_PALETTE(g_windows_light_palette), g_windows_metrics },
	{ R_LOOK_MACOS_DARK, "MacOSDark", NULL, RVN_FAMILY_MACOS, 1, 0, 0, RVN_PALETTE(g_macos_dark_palette), g_macos_metrics },
	{ R_LOOK_MACOS_LIGHT, "MacOSLight", NULL, RVN_FAMILY_MACOS, 0, 0, 0, RVN_PALETTE(g_macos_light_palette), g_macos_metrics },
	{ R_LOOK_GNOME_DARK, "GnomeDark", NULL, RVN_FAMILY_GNOME, 1, 0, 0, RVN_PALETTE(g_gnome_dark_palette), g_gnome_metrics },
	{ R_LOOK_GNOME_LIGHT, "GnomeLight", NULL, RVN_FAMILY_GNOME, 0, 0, 0, RVN_PALETTE(g_gnome_light_palette), g_gnome_metrics },
	/* (`glass` of a look of a platform: its bars, menus and floating surfaces are glass) */
	{ R_LOOK_MACOS_GLASS_DARK, "MacOSGlassDark", NULL, RVN_FAMILY_MACOS, 1, 1, 0, RVN_PALETTE(g_macos_glass_dark_palette), g_macos_glass_metrics },
	{ R_LOOK_MACOS_GLASS_LIGHT, "MacOSGlassLight", NULL, RVN_FAMILY_MACOS, 0, 1, 0, RVN_PALETTE(g_macos_glass_light_palette), g_macos_glass_metrics }
};

#if defined(__APPLE__)
/*
 * The major version of macOS: 26 for Tahoe. A program built with an SDK
 * older than macOS 26 is told 16 by the system (its compatibility version),
 * which means the same.
 */
static int rvn_macos_version(void) {
	char text[32];
	size_t size = sizeof(text);
	int major;
	if (sysctlbyname("kern.osproductversion", text, &size, NULL, 0) != 0) return 0;
	text[sizeof(text) - 1] = '\0';
	major = atoi(text);
	return major == 16 ? 26 : major;
}
#endif

int rvn_look_valid(RLook look) {
	return look >= R_LOOK_RAVEN_DARK && look <= R_LOOK_MACOS_GLASS_LIGHT;
}

RLook rvn_look_resolve(RLook look) {
	if (look == R_LOOK_NATIVE_DARK || look == R_LOOK_NATIVE_LIGHT) {
		int dark = look == R_LOOK_NATIVE_DARK;
#if defined(_WIN32)
		return dark ? R_LOOK_WINDOWS_DARK : R_LOOK_WINDOWS_LIGHT;
#elif defined(__APPLE__)
		/* Liquid Glass from macOS 26 on, as the applications of the system */
		if (rvn_macos_version() >= 26) return dark ? R_LOOK_MACOS_GLASS_DARK : R_LOOK_MACOS_GLASS_LIGHT;
		return dark ? R_LOOK_MACOS_DARK : R_LOOK_MACOS_LIGHT;
#else
		/* (Linux and the other Unix: GNOME, the desktop most of them have) */
		return dark ? R_LOOK_GNOME_DARK : R_LOOK_GNOME_LIGHT;
#endif
	}
	return rvn_look_valid(look) ? look : R_LOOK_RAVEN_DARK;
}

static const rvn_look_info_t* rvn_look_info(RLook look) {
	size_t i;
	look = rvn_look_resolve(look);
	for (i = 0; i < RVN_COUNT_OF(g_looks); i++) {
		if (g_looks[i].look == look) return &g_looks[i];
	}
	return &g_looks[0];
}

const char* rvn_look_name(RLook look) {
	if (look == R_LOOK_NATIVE_DARK) return "NativeDark";
	if (look == R_LOOK_NATIVE_LIGHT) return "NativeLight";
	return rvn_look_info(look)->name;
}

const char* rvn_look_old_name(RLook look) {
	return rvn_look_valid(look) && look <= R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT ? rvn_look_info(look)->old_name : NULL;
}

int rvn_look_by_name(const char* name, RLook* look) {
	size_t i;
	if (!name) return R_FALSE;
	if (rvn_stricmp(name, "NativeDark") == 0 || rvn_stricmp(name, "Native") == 0) {
		*look = R_LOOK_NATIVE_DARK;
		return R_TRUE;
	}
	if (rvn_stricmp(name, "NativeLight") == 0) {
		*look = R_LOOK_NATIVE_LIGHT;
		return R_TRUE;
	}
	for (i = 0; i < RVN_COUNT_OF(g_looks); i++) {
		if (rvn_stricmp(g_looks[i].name, name) == 0 || (g_looks[i].old_name && rvn_stricmp(g_looks[i].old_name, name) == 0)) {
			*look = g_looks[i].look;
			return R_TRUE;
		}
	}
	/* "Windows", "macOS", "GNOME" alone: the light one, as the systems start */
	if (rvn_stricmp(name, "Windows") == 0) *look = R_LOOK_WINDOWS_LIGHT;
	else if (rvn_stricmp(name, "macOS") == 0 || rvn_stricmp(name, "Mac") == 0) *look = R_LOOK_MACOS_LIGHT;
	else if (rvn_stricmp(name, "MacOSGlass") == 0 || rvn_stricmp(name, "Tahoe") == 0 || rvn_stricmp(name, "LiquidGlass") == 0) *look = R_LOOK_MACOS_GLASS_LIGHT;
	else if (rvn_stricmp(name, "TahoeDark") == 0 || rvn_stricmp(name, "LiquidGlassDark") == 0) *look = R_LOOK_MACOS_GLASS_DARK;
	else if (rvn_stricmp(name, "TahoeLight") == 0 || rvn_stricmp(name, "LiquidGlassLight") == 0) *look = R_LOOK_MACOS_GLASS_LIGHT;
	else if (rvn_stricmp(name, "GNOME") == 0 || rvn_stricmp(name, "Adwaita") == 0 || rvn_stricmp(name, "Linux") == 0) *look = R_LOOK_GNOME_LIGHT;
	else if (rvn_stricmp(name, "AdwaitaDark") == 0 || rvn_stricmp(name, "LinuxDark") == 0) *look = R_LOOK_GNOME_DARK;
	else if (rvn_stricmp(name, "AdwaitaLight") == 0 || rvn_stricmp(name, "LinuxLight") == 0) *look = R_LOOK_GNOME_LIGHT;
	else return R_FALSE;
	return R_TRUE;
	return R_FALSE;
}

RLook rvn_look_dark_counterpart(RLook look) {
	const rvn_look_info_t* info = rvn_look_info(look);
	if (info->dark) return info->look;
	if (info->family == RVN_FAMILY_WINDOWS) return R_LOOK_WINDOWS_DARK;
	if (info->family == RVN_FAMILY_MACOS) return info->glass ? R_LOOK_MACOS_GLASS_DARK : R_LOOK_MACOS_DARK;
	if (info->family == RVN_FAMILY_GNOME) return R_LOOK_GNOME_DARK;
	if (info->contrast) return R_LOOK_RAVEN_HIGH_CONTRAST_DARK;
	return info->glass ? R_LOOK_RAVEN_GLASS_DARK : R_LOOK_RAVEN_DARK;
}

int rvn_look_dark(RLook look) {
	return rvn_look_info(look)->dark;
}

/* ---------------------------------------------------------------------------
 *  Wallpaper of the glass themes
 * ------------------------------------------------------------------------- */

#define RVN_WALLPAPER_SIZE 256

typedef struct rvn_blob_t {
	float x, y, radius;
	float r, g, b;
} rvn_blob_t;

static Rid rvn_wallpaper_render(const float base[3], const rvn_blob_t* blobs, int count) {
	uint8_t* pixels = (uint8_t*)rvn_alloc((size_t)RVN_WALLPAPER_SIZE * RVN_WALLPAPER_SIZE * 4);
	int x, y, i;

	for (y = 0; y < RVN_WALLPAPER_SIZE; y++) {
		for (x = 0; x < RVN_WALLPAPER_SIZE; x++) {
			float u = ((float)x + 0.5f) / RVN_WALLPAPER_SIZE, v = ((float)y + 0.5f) / RVN_WALLPAPER_SIZE;
			float color[3];
			uint8_t* out = pixels + ((size_t)y * RVN_WALLPAPER_SIZE + (size_t)x) * 4;
			color[0] = base[0];
			color[1] = base[1];
			color[2] = base[2];
			for (i = 0; i < count; i++) {
				float dx = u - blobs[i].x, dy = v - blobs[i].y;
				float weight = expf(-(dx * dx + dy * dy) / (blobs[i].radius * blobs[i].radius * 0.5f)) * 0.9f;
				color[0] += (blobs[i].r - color[0]) * weight;
				color[1] += (blobs[i].g - color[1]) * weight;
				color[2] += (blobs[i].b - color[2]) * weight;
			}
			out[0] = (uint8_t)(rvn_clampf(color[0], 0.0f, 1.0f) * 255.0f + 0.5f);
			out[1] = (uint8_t)(rvn_clampf(color[1], 0.0f, 1.0f) * 255.0f + 0.5f);
			out[2] = (uint8_t)(rvn_clampf(color[2], 0.0f, 1.0f) * 255.0f + 0.5f);
			out[3] = 255;
		}
	}
	/* the texture takes ownership of the pixels */
	return rvn_texture_create_raw(RVN_WALLPAPER_SIZE, RVN_WALLPAPER_SIZE, R_TEXTURE_RGBA8, R_FILTER_LINEAR, pixels);
}

/* Where the blobs lie, and how large they are: the same for every wallpaper. */
static const rvn_blob_t g_blob_places[5] = {
	{ 0.15f, 0.20f, 0.55f, 0.0f, 0.0f, 0.0f },
	{ 0.88f, 0.12f, 0.50f, 0.0f, 0.0f, 0.0f },
	{ 0.80f, 0.86f, 0.58f, 0.0f, 0.0f, 0.0f },
	{ 0.12f, 0.92f, 0.46f, 0.0f, 0.0f, 0.0f },
	{ 0.50f, 0.50f, 0.34f, 0.0f, 0.0f, 0.0f }
};

Rid rvn_wallpaper_create_colors(uint32_t base, const uint32_t* colors, int count) {
	rvn_blob_t blobs[5];
	float base_color[3];
	int i;
	if (count > 5) count = 5;
	base_color[0] = (float)((base >> 16) & 0xFFu) / 255.0f;
	base_color[1] = (float)((base >> 8) & 0xFFu) / 255.0f;
	base_color[2] = (float)(base & 0xFFu) / 255.0f;
	for (i = 0; i < count; i++) {
		blobs[i] = g_blob_places[i];
		blobs[i].r = (float)((colors[i] >> 16) & 0xFFu) / 255.0f;
		blobs[i].g = (float)((colors[i] >> 8) & 0xFFu) / 255.0f;
		blobs[i].b = (float)(colors[i] & 0xFFu) / 255.0f;
	}
	return rvn_wallpaper_render(base_color, blobs, count);
}

/*
 * The background of the glass themes: plain black for the dark one, plain
 * white for the light one. It is still a wallpaper (a texture under the whole
 * window), so that Rwindow_set_background and the Wallpaper of a theme file
 * ("#101020, #3040C0, #C02080": colored blobs) replace it the same way.
 */
Rid rvn_theme_builtin_wallpaper(int dark) {
	Rid* wallpaper = &g_rvn.wallpapers[dark ? 0 : 1];
	if (!rvn_texture_get(*wallpaper)) {
		/* (dark: a graphite grey, not black: the glass reads as frosted, not as a hole) */
		*wallpaper = rvn_wallpaper_create_colors(dark ? 0x2A2A2E : 0xFFFFFF, NULL, 0);
	}
	return *wallpaper;
}

void rvn_theme_build_look(rvn_theme_t* theme, RLook look) {
	const rvn_look_info_t* info = rvn_look_info(look);
	size_t i;
	char* name = theme->name;
	char* path = theme->path;
	Rid font = theme->font;
	int look_font = theme->look_font;

	theme->name = NULL;
	theme->path = NULL;
	rvn_theme_clear(theme);
	theme->name = name;
	theme->path = path;

	for (i = 0; i < info->palette_count; i++) {
		theme->colors[RVN_COLOR_INDEX(info->palette[i].role)] = rvn_hex(info->palette[i].rgb, info->palette[i].alpha);
	}
	memcpy(theme->metrics, info->metrics, sizeof(theme->metrics));

	theme->look = info->look;
	theme->family = info->family;
	theme->motion = info->family == RVN_FAMILY_RAVEN ? RVN_MOTION_SPRINGY : RVN_MOTION_SMOOTH;
	/* (the glass of macOS 26 is drawn by the widgets of the platform where it belongs: their material stays solid) */
	theme->glass = info->glass && info->family == RVN_FAMILY_RAVEN;
	theme->liquid = info->glass && info->family != RVN_FAMILY_RAVEN;
	theme->material = theme->glass ? R_MATERIAL_FROSTED_GLASS : R_MATERIAL_SOLID;
	theme->dark = info->dark;
	theme->wallpaper = theme->glass ? rvn_theme_builtin_wallpaper(info->dark) : R_INVALID_ID;

	/*
	 * The font: the looks of Raven keep the one in use (it is the
	 * application's), the looks of the platforms bring theirs when the
	 * system has it; going back to Raven gives back the font of Raven.
	 */
	if (info->family != RVN_FAMILY_RAVEN && rvn_look_platform_font(info->family) != R_INVALID_ID) {
		theme->font = rvn_look_platform_font(info->family);
		theme->look_font = R_TRUE;
	} else if (look_font) {
		theme->font = theme == &g_rvn.theme ? g_rvn.default_font : R_INVALID_ID;
		theme->look_font = R_FALSE;
	} else {
		theme->font = font;
		theme->look_font = R_FALSE;
	}
	/* the accent the user chose for the system */
	if (info->family != RVN_FAMILY_RAVEN) rvn_look_system_accent(theme);
}

void rvn_theme_build(rvn_theme_t* theme, RTheme builtin) {
	rvn_theme_build_look(theme, (RLook)builtin);
}

void rvn_theme_apply(RTheme theme) {
	rvn_look_apply((RLook)theme, R_FALSE);
}

/* ---------------------------------------------------------------------------
 *  Lookup
 * ------------------------------------------------------------------------- */

rvn_color_t rvn_widget_color(const rvn_widget_t* widget, RColorRole role) {
	int index = RVN_COLOR_INDEX(role);
	uint64_t bit = (uint64_t)1 << index;
	const rvn_style_match_t* match;
	if (widget->style && (widget->style->color_mask & bit)) return widget->style->colors[index];
	match = rvn_widget_match(widget);
	if (match && (match->style.color_mask & bit)) return match->style.colors[index];
	return rvn_widget_theme(widget)->colors[index];
}

static float rvn_widget_metric_raw(const rvn_widget_t* widget, RMetric metric) {
	int index = RVN_METRIC_INDEX(metric);
	uint32_t bit = 1u << index;
	const rvn_style_match_t* match;
	if (widget->style && (widget->style->metric_mask & bit)) return widget->style->metrics[index];
	match = rvn_widget_match(widget);
	if (match && (match->style.metric_mask & bit)) return match->style.metrics[index];
	return rvn_widget_theme(widget)->metrics[index];
}

float rvn_widget_metric(const rvn_widget_t* widget, RMetric metric) {
	float value = rvn_widget_metric_raw(widget, metric);
	float scale = rvn_a11y_text_scale();
	/* larger texts (Raccessibility_set_text_scale): the controls that hold a line of text grow with it */
	if (scale != 1.0f) {
		switch (metric) {
			case R_METRIC_CONTROL_HEIGHT: case R_METRIC_ROW_HEIGHT: case R_METRIC_CHECK_SIZE:
			case R_METRIC_SLIDER_HANDLE_SIZE: case R_METRIC_INDENT: case R_METRIC_PADDING_Y:
				value *= scale;
				break;
			default:
				break;
		}
	}
	return value;
}

int rvn_widget_has_color(const rvn_widget_t* widget, RColorRole role) {
	uint64_t bit = (uint64_t)1 << RVN_COLOR_INDEX(role);
	const rvn_style_match_t* match;
	if (widget->style && (widget->style->color_mask & bit)) return R_TRUE;
	match = rvn_widget_match(widget);
	return match && (match->style.color_mask & bit) != 0;
}

int rvn_widget_has_metric(const rvn_widget_t* widget, RMetric metric) {
	uint32_t bit = 1u << RVN_METRIC_INDEX(metric);
	const rvn_style_match_t* match;
	if (widget->style && (widget->style->metric_mask & bit)) return R_TRUE;
	match = rvn_widget_match(widget);
	return match && (match->style.metric_mask & bit) != 0;
}

static rvn_style_t* rvn_widget_style(rvn_widget_t* widget) {
	if (!widget->style) widget->style = (rvn_style_t*)rvn_calloc(sizeof(rvn_style_t));
	return widget->style;
}

void rvn_widget_set_color(rvn_widget_t* widget, RColorRole role, rvn_color_t color) {
	rvn_style_t* style = rvn_widget_style(widget);
	int index = RVN_COLOR_INDEX(role);
	style->colors[index] = color;
	style->color_mask |= (uint64_t)1 << index;
	rvn_widget_invalidate_paint(widget);
}

void rvn_widget_set_metric(rvn_widget_t* widget, RMetric metric, float value) {
	rvn_style_t* style = rvn_widget_style(widget);
	int index = RVN_METRIC_INDEX(metric);
	style->metrics[index] = value;
	style->metric_mask |= 1u << index;
	rvn_widget_invalidate_layout(widget);
}

rvn_color_t rvn_widget_text_color(const rvn_widget_t* widget, const RPainter* painter) {
	if (!painter->enabled) return rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
	/* the painter already carries the override of this widget or of an ancestor */
	return painter->text_color;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rtheme_apply(RTheme theme) {
	RVN_API_GUARD_VOID();
	if (theme < R_THEME_DARK || theme > R_THEME_HIGH_CONTRAST_LIGHT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_apply: invalid theme");
		return;
	}
	rvn_look_apply((RLook)theme, R_FALSE);
}

void Rlook_set(RLook look) {
	RVN_API_GUARD_VOID();
	if (!rvn_look_valid(look)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rlook_set: invalid look");
		return;
	}
	rvn_look_apply(look, R_TRUE);
}

RLook Rlook_get(void) {
	return g_rvn.initialized ? g_rvn.look : R_LOOK_RAVEN_DARK;
}

void Rtheme_set_color(RColorRole role, float r, float g, float b, float a) {
	RVN_API_GUARD_VOID();
	if (!rvn_color_valid(role)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_color: invalid color role");
		return;
	}
	RVN_THEME_COLOR(role) = rvn_rgba(r, g, b, a);
	rvn_theme_changed();
}

void Rtheme_get_color(RColorRole role, float* r, float* g, float* b, float* a) {
	if (!g_rvn.initialized || !rvn_color_valid(role)) {
		if (r) *r = 0.0f;
		if (g) *g = 0.0f;
		if (b) *b = 0.0f;
		if (a) *a = 0.0f;
		return;
	}
	rvn_color_to_floats(RVN_THEME_COLOR(role), r, g, b, a);
}

void Rtheme_set_metric(RMetric metric, float value) {
	RVN_API_GUARD_VOID();
	if (!rvn_metric_valid(metric)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_metric: invalid metric");
		return;
	}
	RVN_THEME_METRIC(metric) = value;
	rvn_theme_changed();
}

float Rtheme_get_metric(RMetric metric) {
	if (!g_rvn.initialized || !rvn_metric_valid(metric)) return 0.0f;
	return RVN_THEME_METRIC(metric);
}

void Rtheme_set_material(RMaterial material) {
	RVN_API_GUARD_VOID();
	if (material < R_MATERIAL_SOLID || material > R_MATERIAL_TINTED_GLASS) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_material: invalid material");
		return;
	}
	if (g_rvn.theme.family != RVN_FAMILY_RAVEN && material != R_MATERIAL_SOLID) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "Rtheme_set_material: the look %s is opaque; only the looks of Raven are made of glass",
			rvn_look_name(g_rvn.theme.look));
		return;
	}
	g_rvn.theme.material = material;
	g_rvn.theme.glass = material != R_MATERIAL_SOLID;
	/* (the sizes of some controls depend on it) */
	rvn_theme_changed();
}

RMaterial Rtheme_get_material(void) {
	return g_rvn.initialized ? g_rvn.theme.material : R_MATERIAL_SOLID;
}

void Rwidget_set_material(Rid id, RMaterial material) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (material < R_MATERIAL_SOLID || material > R_MATERIAL_DEFAULT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_set_material: invalid material");
		return;
	}
	widget->material = material == R_MATERIAL_DEFAULT ? (RMaterial)0 : material;
	rvn_widget_invalidate_layout(widget);
}

RMaterial Rwidget_get_material(Rid id) {
	RVN_WIDGET(widget, id, 0, R_MATERIAL_SOLID);
	return rvn_widget_material(widget);
}

void Rtheme_set_font(Rid font) {
	RVN_API_GUARD_VOID();
	if (font != R_INVALID_ID && !rvn_pool_get(&g_rvn.fonts, font)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rtheme_set_font: invalid font");
		return;
	}
	g_rvn.theme.font = font == R_INVALID_ID ? g_rvn.default_font : font;
	/* (the application's: it is kept when the look changes) */
	g_rvn.theme.look_font = R_FALSE;
	rvn_theme_changed();
}

Rid Rtheme_get_font(void) {
	return g_rvn.initialized ? g_rvn.theme.font : R_INVALID_ID;
}

void Rwidget_set_color(Rid id, RColorRole role, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_color_valid(role)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_set_color: invalid color role");
		return;
	}
	rvn_widget_set_color(widget, role, rvn_rgba(r, g, b, a));
}

void Rwidget_clear_color(Rid id, RColorRole role) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_color_valid(role)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_clear_color: invalid color role");
		return;
	}
	if (widget->style) widget->style->color_mask &= ~((uint64_t)1 << RVN_COLOR_INDEX(role));
	rvn_widget_invalidate_paint(widget);
}

void Rwidget_set_metric(Rid id, RMetric metric, float value) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_metric_valid(metric)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_set_metric: invalid metric");
		return;
	}
	rvn_widget_set_metric(widget, metric, value);
}

void Rwidget_clear_metric(Rid id, RMetric metric) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_metric_valid(metric)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_clear_metric: invalid metric");
		return;
	}
	if (widget->style) widget->style->metric_mask &= ~(1u << RVN_METRIC_INDEX(metric));
	rvn_widget_invalidate_layout(widget);
}

void Rwidget_get_color(Rid id, RColorRole role, float* r, float* g, float* b, float* a) {
	rvn_widget_t* widget = rvn_widget_check(id, 0, __func__);
	rvn_color_t color = 0;
	if (widget && !rvn_color_valid(role)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_get_color: invalid color role");
	} else if (widget) {
		color = rvn_widget_color(widget, role);
	}
	rvn_color_to_floats(color, r, g, b, a);
}

float Rwidget_get_metric(Rid id, RMetric metric) {
	RVN_WIDGET(widget, id, 0, 0.0f);
	if (!rvn_metric_valid(metric)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_get_metric: invalid metric");
		return 0.0f;
	}
	return rvn_widget_metric(widget, metric);
}
