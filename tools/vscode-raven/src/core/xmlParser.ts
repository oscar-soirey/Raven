/**
 * A tolerant XML reader: it keeps the offsets of everything, and goes on
 * after a mistake, so that a document being typed still has a tree.
 * Tags are matched without regard to case, as Raven does.
 */

export interface XAttribute {
	name: string;
	nameStart: number;
	nameEnd: number;
	/** Raw text between the quotes, null when the attribute has no value. */
	raw: string | null;
	/** The value with its entities replaced. */
	value: string | null;
	/** Offsets inside the quotes. */
	valueStart: number;
	valueEnd: number;
	quoted: boolean;
}

export interface XElement {
	name: string;
	/** Offset of '<'. */
	start: number;
	nameStart: number;
	nameEnd: number;
	attributes: XAttribute[];
	/** Offset just after the '>' of the start tag, or where an unterminated tag stops. */
	startTagEnd: number;
	startTagClosed: boolean;
	selfClosing: boolean;
	children: XElement[];
	parent: XElement | null;
	/** Offset of '</' of the end tag, -1 when there is none. */
	closeStart: number;
	closeNameStart: number;
	closeNameEnd: number;
	/** Offset after the element (after its end tag, or its start tag when self-closing). */
	end: number;
	hasText: boolean;
}

export interface XError {
	start: number;
	end: number;
	message: string;
}

export interface XDocument {
	text: string;
	/** Elements at the top level (Raven takes one, besides its <Widget> declarations). */
	roots: XElement[];
	errors: XError[];
	/** Comments and CDATA, to know where completion must not happen. */
	opaque: { start: number; end: number }[];
}

const NAME_START = /[A-Za-z_:]/;
const NAME_CHAR = /[A-Za-z0-9_:.\-]/;

export function sameName(a: string, b: string): boolean {
	return a.toLowerCase() === b.toLowerCase();
}

const ENTITIES: Record<string, string> = { amp: "&", lt: "<", gt: ">", quot: '"', apos: "'" };

/** Replaces the entities; reports the ampersands that start none. */
export function decodeEntities(raw: string, offset: number, errors?: XError[]): string {
	return raw.replace(/&([^;&\s<]{0,12});?/g, (match: string, body: string, index: number) => {
		if (match.endsWith(";")) {
			if (body in ENTITIES) return ENTITIES[body];
			const numeric = /^#(x[0-9A-Fa-f]+|[0-9]+)$/.exec(body);
			if (numeric) {
				const code = numeric[1][0] === "x" ? parseInt(numeric[1].slice(1), 16) : parseInt(numeric[1], 10);
				try {
					return String.fromCodePoint(code);
				} catch {
					/* reported below */
				}
			}
		}
		errors?.push({
			start: offset + index,
			end: offset + index + match.length,
			message: "'&' starts no entity: write it &amp; (the entities are &amp; &lt; &gt; &quot; &apos; and &#...;)",
		});
		return match;
	});
}

export function parseXml(text: string): XDocument {
	const doc: XDocument = { text, roots: [], errors: [], opaque: [] };
	const stack: XElement[] = [];
	const length = text.length;
	let i = 0;

	const top = (): XElement | null => (stack.length ? stack[stack.length - 1] : null);
	const error = (start: number, end: number, message: string) => doc.errors.push({ start, end: Math.max(end, start + 1), message });

	const readName = (from: number): number => {
		let j = from;
		while (j < length && NAME_CHAR.test(text[j])) j++;
		return j;
	};

	while (i < length) {
		const c = text[i];
		if (c !== "<") {
			const next = text.indexOf("<", i);
			const stop = next < 0 ? length : next;
			const parent = top();
			const chunk = text.slice(i, stop);
			if (/\S/.test(chunk)) {
				if (parent) parent.hasText = true;
				else error(i + chunk.search(/\S/), stop, "text outside of the root element");
				decodeEntities(chunk, i, doc.errors);
			}
			i = stop;
			continue;
		}
		if (text.startsWith("<!--", i)) {
			const close = text.indexOf("-->", i + 4);
			const end = close < 0 ? length : close + 3;
			if (close < 0) error(i, i + 4, "comment never closed (-->)");
			doc.opaque.push({ start: i, end });
			i = end;
			continue;
		}
		if (text.startsWith("<![CDATA[", i)) {
			const close = text.indexOf("]]>", i + 9);
			const end = close < 0 ? length : close + 3;
			if (close < 0) error(i, i + 9, "CDATA never closed (]]>)");
			const parent = top();
			if (parent) parent.hasText = true;
			doc.opaque.push({ start: i, end });
			i = end;
			continue;
		}
		if (text.startsWith("<?", i)) {
			const close = text.indexOf("?>", i + 2);
			i = close < 0 ? length : close + 2;
			continue;
		}
		if (text.startsWith("<!", i)) {
			const close = text.indexOf(">", i + 2);
			i = close < 0 ? length : close + 1;
			continue;
		}
		if (text[i + 1] === "/") {
			/* end tag */
			const nameStart = i + 2;
			const nameEnd = readName(nameStart);
			const name = text.slice(nameStart, nameEnd);
			const gt = text.indexOf(">", nameEnd);
			const lt = text.indexOf("<", nameEnd);
			const closed = gt >= 0 && (lt < 0 || gt < lt);
			const end = closed ? gt + 1 : lt < 0 ? length : lt;
			if (!closed) error(i, nameEnd, `end tag </${name}> is not closed with '>'`);
			let index = stack.length - 1;
			while (index >= 0 && !sameName(stack[index].name, name)) index--;
			if (!name) {
				error(i, i + 2, "end tag without a name");
			} else if (index < 0) {
				error(i, end, `</${name}> closes no open element`);
			} else {
				for (let k = stack.length - 1; k > index; k--) {
					const open = stack[k];
					error(open.nameStart, open.nameEnd, `<${open.name}> is not closed (expected </${open.name}> before </${name}>)`);
					open.end = i;
				}
				const element = stack[index];
				element.closeStart = i;
				element.closeNameStart = nameStart;
				element.closeNameEnd = nameEnd;
				element.end = end;
				stack.length = index;
			}
			i = end;
			continue;
		}
		if (!NAME_START.test(text[i + 1] || "")) {
			/* a '<' alone: being typed, or a mistake */
			error(i, i + 1, "'<' starts no tag: write it &lt;");
			i++;
			continue;
		}

		/* start tag */
		const nameStart = i + 1;
		const nameEnd = readName(nameStart);
		const element: XElement = {
			name: text.slice(nameStart, nameEnd),
			start: i,
			nameStart,
			nameEnd,
			attributes: [],
			startTagEnd: length,
			startTagClosed: false,
			selfClosing: false,
			children: [],
			parent: top(),
			closeStart: -1,
			closeNameStart: -1,
			closeNameEnd: -1,
			end: length,
			hasText: false,
		};
		let j = nameEnd;
		for (;;) {
			while (j < length && /\s/.test(text[j])) j++;
			if (j >= length || text[j] === "<") {
				error(element.start, element.nameEnd, `start tag <${element.name}> is not closed with '>'`);
				element.startTagEnd = j;
				break;
			}
			if (text[j] === ">") {
				element.startTagClosed = true;
				element.startTagEnd = j + 1;
				break;
			}
			if (text[j] === "/" && text[j + 1] === ">") {
				element.startTagClosed = true;
				element.selfClosing = true;
				element.startTagEnd = j + 2;
				break;
			}
			/* attribute */
			const aStart = j;
			while (j < length && !/[\s=/>"'<]/.test(text[j])) j++;
			if (j === aStart) {
				error(j, j + 1, `unexpected '${text[j]}' in <${element.name}>`);
				j++;
				continue;
			}
			const attribute: XAttribute = {
				name: text.slice(aStart, j),
				nameStart: aStart,
				nameEnd: j,
				raw: null,
				value: null,
				valueStart: j,
				valueEnd: j,
				quoted: false,
			};
			element.attributes.push(attribute);
			let k = j;
			while (k < length && /\s/.test(text[k])) k++;
			if (text[k] !== "=") {
				error(aStart, j, `attribute ${attribute.name} has no value (${attribute.name}="...")`);
				continue;
			}
			k++;
			while (k < length && /\s/.test(text[k])) k++;
			const quote = text[k];
			if (quote === '"' || quote === "'") {
				const close = text.indexOf(quote, k + 1);
				const stop = close < 0 ? length : close;
				attribute.quoted = true;
				attribute.valueStart = k + 1;
				attribute.valueEnd = stop;
				attribute.raw = text.slice(k + 1, stop);
				if (close < 0) error(k, k + 1, `the value of ${attribute.name} is never closed (${quote})`);
				const lt = attribute.raw.indexOf("<");
				if (lt >= 0) error(attribute.valueStart + lt, attribute.valueStart + lt + 1, "'<' inside a value: write it &lt;");
				attribute.value = decodeEntities(attribute.raw, attribute.valueStart, doc.errors);
				j = close < 0 ? length : close + 1;
			} else {
				const vStart = k;
				while (k < length && !/[\s>]/.test(text[k]) && !(text[k] === "/" && text[k + 1] === ">")) k++;
				attribute.valueStart = vStart;
				attribute.valueEnd = k;
				attribute.raw = text.slice(vStart, k);
				attribute.value = attribute.raw;
				error(aStart, k, `the value of ${attribute.name} must be between quotes`);
				j = k;
			}
		}

		const seen = new Map<string, XAttribute>();
		for (const attribute of element.attributes) {
			const key = attribute.name.toLowerCase();
			if (seen.has(key)) error(attribute.nameStart, attribute.nameEnd, `${attribute.name} is given twice`);
			seen.set(key, attribute);
		}

		const parent = top();
		if (parent) parent.children.push(element);
		else doc.roots.push(element);
		if (element.startTagClosed && !element.selfClosing) stack.push(element);
		else element.end = element.startTagEnd;
		i = element.startTagEnd;
	}

	for (const open of stack) {
		error(open.nameStart, open.nameEnd, `<${open.name}> is not closed (</${open.name}>)`);
		open.end = length;
	}
	return doc;
}

export function attribute(element: XElement, name: string): XAttribute | undefined {
	return element.attributes.find((a) => sameName(a.name, name));
}

export function* walk(elements: XElement[]): Generator<XElement> {
	for (const element of elements) {
		yield element;
		yield* walk(element.children);
	}
}

export function isOpaque(doc: XDocument, offset: number): boolean {
	return doc.opaque.some((r) => offset > r.start && offset < r.end);
}

/** The innermost element whose content (between its tags) holds the offset. */
export function elementAround(doc: XDocument, offset: number): XElement | null {
	let found: XElement | null = null;
	let list = doc.roots;
	for (;;) {
		const next = list.find((e) => !e.selfClosing && e.startTagClosed && offset >= e.startTagEnd && (e.closeStart < 0 ? offset <= e.end : offset <= e.closeStart));
		if (!next) return found;
		found = next;
		list = next.children;
	}
}

/** The element whose start tag holds the offset. */
export function startTagAt(doc: XDocument, offset: number): XElement | null {
	for (const element of walk(doc.roots)) {
		const inner = element.startTagClosed ? element.startTagEnd - (element.selfClosing ? 2 : 1) : element.startTagEnd;
		if (offset > element.start && offset <= inner) return element;
	}
	return null;
}
