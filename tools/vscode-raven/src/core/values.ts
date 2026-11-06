/**
 * Checks a value the way the parsers of Raven read it (xml_attributes.c).
 * Returns a message when Raven would refuse it, null otherwise.
 */
import { AttributeInfo, BUILTIN_ACTIONS, COLOR_NAMES } from "./schema";

const NUMBER = /^\s*[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?\s*$/;
const BOOLEANS = ["true", "false", "yes", "no", "1", "0"];

export function isNumber(value: string): boolean {
	return NUMBER.test(value);
}

export function isBool(value: string): boolean {
	return BOOLEANS.includes(value.toLowerCase()) && (value === "1" || value === "0" || /[a-z]/i.test(value));
}

export function isColor(value: string): boolean {
	const text = value.replace(/^\s+/, "");
	if (text[0] !== "#") return COLOR_NAMES.includes(text.toLowerCase());
	return /^#([0-9A-Fa-f]{3,4}|[0-9A-Fa-f]{6}|[0-9A-Fa-f]{8})$/.test(text);
}

/** Numbers separated by commas or spaces (rvn_parse_floats). */
export function numberList(value: string): number | null {
	const parts = value.trim().split(/\s*,\s*|\s+/);
	if (!parts.length || parts.some((p) => !isNumber(p))) return null;
	return parts.length;
}

/** Values that are not checked: computed ones, translations. */
export function isDynamic(value: string): boolean {
	return value.includes("{") || value.startsWith("@");
}

export function checkValue(attribute: AttributeInfo, value: string): string | null {
	/* an empty value is one being typed */
	if (isDynamic(value) || value === "") return null;
	const name = attribute.name;
	switch (attribute.kind) {
		case "bool":
			return isBool(value) ? null : `${name} takes a boolean: true or false (also yes / no, 1 / 0), not "${value}"`;
		case "number":
			return isNumber(value) ? null : `${name} takes a number (with a dot: 0.5), not "${value}"`;
		case "color":
			return isColor(value) ? null : `${name} takes a color: #RGB, #RGBA, #RRGGBB, #RRGGBBAA or ${COLOR_NAMES.slice(0, -1).join(", ")}; not "${value}"`;
		case "numbers": {
			const count = numberList(value);
			if (count === null) return `${name} takes numbers separated by commas, not "${value}"`;
			if (/padding|margin/i.test(name) && ![1, 2, 4].includes(count)) return `${name} takes 1 number (all sides), 2 (horizontal, vertical) or 4 (left, top, right, bottom), not ${count}`;
			if (count > 4) return `${name} takes at most 4 numbers, not ${count}`;
			return null;
		}
		case "choice": {
			const choices = attribute.choices || [];
			/* a theme can also be a theme file or a name given to Rtheme_load */
			if (/^theme$/i.test(name)) return null;
			/* "Close,Minimize": a list of choices */
			const items = choices.some((ch) => ch.includes(",")) ? value.split(",").map((s) => s.trim()) : [value.trim()];
			const allowed = new Set(choices.flatMap((ch) => ch.split(",")).map((ch) => ch.toLowerCase()));
			const wrong = items.find((item) => !allowed.has(item.toLowerCase()));
			return wrong === undefined ? null : `"${wrong}" is not a value of ${name}: ${choices.join(", ")}`;
		}
		case "action":
			return checkAction(value);
		default:
			return null;
	}
}

/** The form of the built-in actions; any other name is the one of the application. */
export function checkAction(value: string): string | null {
	const colon = value.indexOf(":");
	if (colon < 0) return null;
	const prefix = value.slice(0, colon + 1).toLowerCase();
	if (!BUILTIN_ACTIONS.some((a) => a.name === prefix)) return null;
	const argument = value.slice(colon + 1);
	if (!argument) return `${prefix} needs what it applies to: ${prefix}NAME`;
	if (prefix === "page:" && !/^[^=]+=([+-]?\d+)$/.test(argument)) return "page: is written page:NAME=2, page:NAME=+1 or page:NAME=-1";
	return null;
}

/** The widget named by a built-in action (show:NAME, page:NAME=+1), with its place in the value. */
export function actionTarget(value: string): { name: string; start: number } | null {
	const m = /^(show|hide|toggle|enable|disable|focus|page):([^=]+)/i.exec(value);
	return m ? { name: m[2], start: m[1].length + 1 } : null;
}
