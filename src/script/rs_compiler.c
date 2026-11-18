/**
 * Raven Script - script/rs_compiler.c
 * Checked syntax tree -> bytecode, and the validation of the bytecode
 * before it runs.
 *
 * The validation follows every path of a chunk and computes the depth of
 * the stack before each instruction: operands in range, jumps on
 * instructions, no instruction that pops what is not there, the same depth
 * whatever the path, no path that runs past the end. The virtual machine
 * then only runs code that cannot touch memory it does not own.
 */

#include "rs_internal.h"

/* ---------------------------------------------------------------------------
 *  Sources and chunks
 * ------------------------------------------------------------------------- */

rs_source* rs_source_create(const rs_unit* unit) {
	rs_source* source = (rs_source*)rs_calloc(1, sizeof(rs_source));
	source->refs = 1;
	rs_unit_init(&source->unit, unit->file, unit->text, unit->length, unit->base_line, unit->base_column);
	return source;
}

void rs_source_retain(rs_source* source) {
	if (source) source->refs++;
}

void rs_source_release(rs_source* source) {
	if (!source || --source->refs > 0) return;
	rs_unit_free(&source->unit);
	rs_free(source);
}

rs_chunk* rs_chunk_create(const char* name) {
	rs_chunk* chunk = (rs_chunk*)rs_calloc(1, sizeof(rs_chunk));
	chunk->refs = 1;
	chunk->name = rs_strdup(name);
	chunk->sig.ret = rs_type_make(RS_T_VOID, 0);
	return chunk;
}

void rs_chunk_retain(rs_chunk* chunk) {
	if (chunk) chunk->refs++;
}

void rs_chunk_release(rs_chunk* chunk) {
	uint32_t i;
	int p;
	if (!chunk || --chunk->refs > 0) return;
	for (i = 0; i < chunk->constant_count; i++) rs_value_release(chunk->constants[i]);
	for (p = 0; p < RS_MAX_PARAMS; p++) rs_free(chunk->param_names[p]);
	rs_free(chunk->constants);
	rs_free(chunk->code);
	rs_free(chunk->name);
	rs_source_release(chunk->source);
	rs_free(chunk);
}

/* ---------------------------------------------------------------------------
 *  Emitting
 * ------------------------------------------------------------------------- */

typedef struct rs_jump_list {
	uint32_t* at;
	uint32_t count;
	uint32_t capacity;
} rs_jump_list;

typedef struct rs_loop {
	rs_jump_list breaks;
	rs_jump_list continues;
} rs_loop;

typedef struct rs_compiler {
	rs_batch* batch;
	rs_chunk* chunk;
	rs_loop* loops;
	uint32_t loop_count;
	uint32_t loop_capacity;
} rs_compiler;

static uint32_t rs_emit(rs_compiler* compiler, int op, int32_t a, int32_t b, const rs_node* at) {
	rs_chunk* chunk = compiler->chunk;
	rs_instr* instr;
	RS_GROW(chunk->code, chunk->count, chunk->capacity);
	instr = &chunk->code[chunk->count];
	instr->op = (uint8_t)op;
	instr->a = a;
	instr->b = b;
	instr->c = 0;
	instr->start = at ? at->start : 0;
	instr->end = at ? at->end : 0;
	return chunk->count++;
}

static void rs_patch(rs_compiler* compiler, uint32_t at, uint32_t target) {
	compiler->chunk->code[at].a = (int32_t)target;
}

static int32_t rs_constant(rs_compiler* compiler, rs_value value) {
	rs_chunk* chunk = compiler->chunk;
	uint32_t i;
	for (i = 0; i < chunk->constant_count; i++) {
		rs_value existing = chunk->constants[i];
		if (existing.type != value.type) continue;
		if (value.type == RS_V_NUMBER && memcmp(&existing.as.number, &value.as.number, sizeof(double)) == 0) return (int32_t)i;
		if (value.type == RS_V_STRING && existing.as.string->length == value.as.string->length &&
			memcmp(existing.as.string->text, value.as.string->text, value.as.string->length) == 0) {
			rs_value_release(value);
			return (int32_t)i;
		}
	}
	RS_GROW(chunk->constants, chunk->constant_count, chunk->constant_capacity);
	chunk->constants[chunk->constant_count] = value;
	return (int32_t)chunk->constant_count++;
}

static int32_t rs_string_constant(rs_compiler* compiler, const char* text, size_t length) {
	rs_value value;
	value.type = RS_V_STRING;
	/* constants are not counted against the memory limit of the scripts */
	value.as.string = rs_string_new(NULL, text, length);
	return rs_constant(compiler, value);
}

static int32_t rs_encode_type(rs_type type) {
	return (int32_t)(type.base | (type.depth << 8));
}

static void rs_jump_add(rs_jump_list* list, uint32_t at) {
	RS_GROW(list->at, list->count, list->capacity);
	list->at[list->count++] = at;
}

static void rs_compile_expression(rs_compiler* compiler, const rs_node* node);
static void rs_compile_statement(rs_compiler* compiler, const rs_node* node);

/** Pushes the value a variable of that type has before it is given one. */
static void rs_compile_default(rs_compiler* compiler, rs_type type, const rs_node* at) {
	if (type.depth > 0) {
		rs_emit(compiler, OP_ARRAY, 0, rs_encode_type(rs_type_element(type)), at);
		return;
	}
	switch (type.base) {
		case RS_T_NUMBER: {
			rs_value zero = rs_value_number(0.0);
			rs_emit(compiler, OP_CONST, rs_constant(compiler, zero), 0, at);
			break;
		}
		case RS_T_STRING: rs_emit(compiler, OP_CONST, rs_string_constant(compiler, "", 0), 0, at); break;
		case RS_T_BOOL: rs_emit(compiler, OP_FALSE, 0, 0, at); break;
		default: rs_emit(compiler, OP_NULL, 0, 0, at); break;
	}
}

static int rs_binary_op(int token) {
	switch (token) {
		case TK_PLUS: return OP_ADD;
		case TK_MINUS: return OP_SUB;
		case TK_STAR: return OP_MUL;
		case TK_SLASH: return OP_DIV;
		case TK_PERCENT: return OP_MOD;
		case TK_EQ: return OP_EQ;
		case TK_NE: return OP_NE;
		case TK_LT: return OP_LT;
		case TK_LE: return OP_LE;
		case TK_GT: return OP_GT;
		case TK_GE: return OP_GE;
		case TK_PLUS_ASSIGN: return OP_ADD;
		case TK_MINUS_ASSIGN: return OP_SUB;
		case TK_STAR_ASSIGN: return OP_MUL;
		case TK_SLASH_ASSIGN: return OP_DIV;
		case TK_PERCENT_ASSIGN: return OP_MOD;
		default: return OP_NOP;
	}
}

static void rs_compile_call(rs_compiler* compiler, const rs_node* node) {
	uint32_t i;
	switch (node->resolved) {
		case RS_R_METHOD:
			rs_compile_expression(compiler, node->a->a);
			for (i = 0; i < node->count; i++) rs_compile_expression(compiler, node->list[i]);
			rs_emit(compiler, OP_METHOD, node->slot, (int32_t)node->count, node);
			break;
		case RS_R_INTRINSIC:
			for (i = 0; i < node->count; i++) rs_compile_expression(compiler, node->list[i]);
			rs_emit(compiler, OP_INTRINSIC, node->slot, (int32_t)node->count, node);
			break;
		case RS_R_FUNCTION:
		case RS_R_EXTERNAL:
			for (i = 0; i < node->count; i++) rs_compile_expression(compiler, node->list[i]);
			{
				uint32_t at = rs_emit(compiler, node->resolved == RS_R_FUNCTION ? OP_CALL : OP_CALL_EXTERNAL, rs_string_constant(compiler, node->path, strlen(node->path)), (int32_t)node->count, node);
				compiler->chunk->code[at].c = rs_encode_type(node->type);
			}
			break;
		default:
			/* (never compiled: the batch had errors) */
			rs_emit(compiler, OP_NULL, 0, 0, node);
			break;
	}
}

static void rs_compile_expression(rs_compiler* compiler, const rs_node* node) {
	switch (node->kind) {
		case RS_N_NUMBER:
			rs_emit(compiler, OP_CONST, rs_constant(compiler, rs_value_number(node->number)), 0, node);
			break;
		case RS_N_STRING:
			rs_emit(compiler, OP_CONST, rs_string_constant(compiler, node->text, node->text_length), 0, node);
			break;
		case RS_N_BOOL:
			rs_emit(compiler, node->number != 0.0 ? OP_TRUE : OP_FALSE, 0, 0, node);
			break;
		case RS_N_NULL:
			rs_emit(compiler, OP_NULL, 0, 0, node);
			break;
		case RS_N_NAME:
			if (node->resolved == RS_R_LOCAL) rs_emit(compiler, OP_LOAD_LOCAL, node->slot, 0, node);
			else rs_emit(compiler, OP_LOAD_GLOBAL, node->slot, node->batch_global, node);
			break;
		case RS_N_MEMBER:
			rs_compile_expression(compiler, node->a);
			rs_emit(compiler, OP_METHOD, node->slot, 0, node);
			break;
		case RS_N_CALL:
			rs_compile_call(compiler, node);
			break;
		case RS_N_INDEX:
			rs_compile_expression(compiler, node->a);
			rs_compile_expression(compiler, node->b);
			rs_emit(compiler, OP_INDEX, 0, 0, node);
			break;
		case RS_N_UNARY:
			rs_compile_expression(compiler, node->a);
			rs_emit(compiler, node->op == TK_BANG ? OP_NOT : OP_NEG, 0, 0, node);
			break;
		case RS_N_BINARY:
			if (node->op == TK_AND || node->op == TK_OR) {
				uint32_t jump;
				rs_compile_expression(compiler, node->a);
				jump = rs_emit(compiler, node->op == TK_AND ? OP_AND : OP_OR, 0, 0, node);
				rs_compile_expression(compiler, node->b);
				rs_patch(compiler, jump, compiler->chunk->count);
			} else {
				rs_compile_expression(compiler, node->a);
				rs_compile_expression(compiler, node->b);
				rs_emit(compiler, rs_binary_op(node->op), 0, 0, node);
			}
			break;
		case RS_N_ARRAY: {
			uint32_t i;
			for (i = 0; i < node->count; i++) rs_compile_expression(compiler, node->list[i]);
			rs_emit(compiler, OP_ARRAY, (int32_t)node->count, rs_encode_type(rs_type_element(node->type)), node);
			break;
		}
		default:
			rs_emit(compiler, OP_NULL, 0, 0, node);
			break;
	}
}

static void rs_compile_store(rs_compiler* compiler, const rs_node* target) {
	if (target->resolved == RS_R_LOCAL) rs_emit(compiler, OP_STORE_LOCAL, target->slot, 0, target);
	else rs_emit(compiler, OP_STORE_GLOBAL, target->slot, target->batch_global, target);
}

static void rs_compile_assign(rs_compiler* compiler, const rs_node* node) {
	const rs_node* target = node->a;
	if (target->kind == RS_N_NAME) {
		if (node->op != TK_ASSIGN) {
			rs_compile_expression(compiler, target);
			rs_compile_expression(compiler, node->b);
			rs_emit(compiler, rs_binary_op(node->op), 0, 0, node);
		} else {
			rs_compile_expression(compiler, node->b);
		}
		rs_compile_store(compiler, target);
		return;
	}
	/* array[index] (op)= value */
	rs_compile_expression(compiler, target->a);
	rs_compile_expression(compiler, target->b);
	if (node->op != TK_ASSIGN) {
		rs_emit(compiler, OP_DUP2, 0, 0, node);
		rs_emit(compiler, OP_INDEX, 0, 0, target);
		rs_compile_expression(compiler, node->b);
		rs_emit(compiler, rs_binary_op(node->op), 0, 0, node);
	} else {
		rs_compile_expression(compiler, node->b);
	}
	rs_emit(compiler, OP_STORE_INDEX, 0, 0, target);
}

static void rs_loop_begin(rs_compiler* compiler) {
	rs_loop* loop;
	RS_GROW(compiler->loops, compiler->loop_count, compiler->loop_capacity);
	loop = &compiler->loops[compiler->loop_count++];
	memset(loop, 0, sizeof(*loop));
}

static void rs_loop_end(rs_compiler* compiler, uint32_t continue_target, uint32_t break_target) {
	rs_loop* loop = &compiler->loops[--compiler->loop_count];
	uint32_t i;
	for (i = 0; i < loop->breaks.count; i++) rs_patch(compiler, loop->breaks.at[i], break_target);
	for (i = 0; i < loop->continues.count; i++) rs_patch(compiler, loop->continues.at[i], continue_target);
	rs_free(loop->breaks.at);
	rs_free(loop->continues.at);
}

static void rs_compile_block(rs_compiler* compiler, const rs_node* block) {
	uint32_t i;
	if (!block) return;
	for (i = 0; i < block->count; i++) rs_compile_statement(compiler, block->list[i]);
}

static void rs_compile_statement(rs_compiler* compiler, const rs_node* node) {
	switch (node->kind) {
		case RS_N_LET:
			if (node->a) rs_compile_expression(compiler, node->a);
			else rs_compile_default(compiler, node->type, node);
			rs_emit(compiler, OP_STORE_LOCAL, node->slot, 0, node);
			break;
		case RS_N_ASSIGN:
			rs_compile_assign(compiler, node);
			break;
		case RS_N_EXPR:
			rs_compile_expression(compiler, node->a);
			rs_emit(compiler, OP_POP, 0, 0, node);
			break;
		case RS_N_IF: {
			uint32_t skip, end;
			rs_compile_expression(compiler, node->a);
			skip = rs_emit(compiler, OP_JUMP_IF_FALSE, 0, 0, node->a);
			rs_compile_block(compiler, node->b);
			if (node->c) {
				end = rs_emit(compiler, OP_JUMP, 0, 0, node);
				rs_patch(compiler, skip, compiler->chunk->count);
				if (node->c->kind == RS_N_IF) rs_compile_statement(compiler, node->c);
				else rs_compile_block(compiler, node->c);
				rs_patch(compiler, end, compiler->chunk->count);
			} else {
				rs_patch(compiler, skip, compiler->chunk->count);
			}
			break;
		}
		case RS_N_WHILE: {
			uint32_t start = compiler->chunk->count, exit;
			rs_loop_begin(compiler);
			rs_compile_expression(compiler, node->a);
			exit = rs_emit(compiler, OP_JUMP_IF_FALSE, 0, 0, node->a);
			rs_compile_block(compiler, node->b);
			rs_emit(compiler, OP_JUMP, (int32_t)start, 0, node);
			rs_patch(compiler, exit, compiler->chunk->count);
			rs_loop_end(compiler, start, compiler->chunk->count);
			break;
		}
		case RS_N_FOR: {
			uint32_t start, exit = (uint32_t)-1, update;
			if (node->a) rs_compile_statement(compiler, node->a);
			start = compiler->chunk->count;
			rs_loop_begin(compiler);
			if (node->b) {
				rs_compile_expression(compiler, node->b);
				exit = rs_emit(compiler, OP_JUMP_IF_FALSE, 0, 0, node->b);
			}
			rs_compile_block(compiler, node->d);
			update = compiler->chunk->count;
			if (node->c) rs_compile_statement(compiler, node->c);
			rs_emit(compiler, OP_JUMP, (int32_t)start, 0, node);
			if (exit != (uint32_t)-1) rs_patch(compiler, exit, compiler->chunk->count);
			rs_loop_end(compiler, update, compiler->chunk->count);
			break;
		}
		case RS_N_BREAK:
			rs_jump_add(&compiler->loops[compiler->loop_count - 1].breaks, rs_emit(compiler, OP_JUMP, 0, 0, node));
			break;
		case RS_N_CONTINUE:
			rs_jump_add(&compiler->loops[compiler->loop_count - 1].continues, rs_emit(compiler, OP_JUMP, 0, 0, node));
			break;
		case RS_N_RETURN:
			if (node->a && !(node->a->type.depth == 0 && node->a->type.base == RS_T_VOID)) {
				rs_compile_expression(compiler, node->a);
				rs_emit(compiler, OP_RETURN, 0, 0, node);
			} else {
				if (node->a) {
					/* return of a void call: run it */
					rs_compile_expression(compiler, node->a);
					rs_emit(compiler, OP_POP, 0, 0, node);
				}
				rs_emit(compiler, OP_RETURN_VOID, 0, 0, node);
			}
			break;
		case RS_N_BLOCK:
			rs_compile_block(compiler, node);
			break;
		default:
			break;
	}
}

static rs_source* rs_batch_source(rs_batch* batch, int unit) {
	if (!batch->sources[unit]) batch->sources[unit] = rs_source_create(&batch->units[unit]);
	return batch->sources[unit];
}

static void rs_compile_function(rs_compiler* compiler, rs_fn_info* info) {
	rs_node* node = info->node;
	rs_chunk* chunk;
	uint32_t p;
	if (node->is_declare || !node->a) return;
	chunk = rs_chunk_create(info->name);
	chunk->sig = info->sig;
	for (p = 0; p < node->param_count && p < RS_MAX_PARAMS; p++) {
		chunk->param_names[p] = rs_strdup(node->params[p].name);
		chunk->sig.names[p] = chunk->param_names[p];
	}
	chunk->param_count = (int)node->param_count;
	chunk->local_count = node->locals > chunk->param_count ? node->locals : chunk->param_count;
	chunk->source = rs_batch_source(compiler->batch, node->unit);
	rs_source_retain(chunk->source);
	compiler->chunk = chunk;
	rs_compile_block(compiler, node->a);
	if (node->type.depth == 0 && node->type.base == RS_T_VOID) rs_emit(compiler, OP_RETURN_VOID, 0, 0, NULL);
	else {
		rs_node end;
		memset(&end, 0, sizeof(end));
		end.start = node->a->end > 0 ? node->a->end - 1 : 0;
		end.end = node->a->end;
		rs_emit(compiler, OP_MISSING_RETURN, 0, 0, &end);
	}
	info->chunk = chunk;
	compiler->chunk = NULL;
}

void rs_compile(rs_batch* batch) {
	rs_compiler compiler;
	uint32_t i;
	memset(&compiler, 0, sizeof(compiler));
	compiler.batch = batch;
	batch->sources = (rs_source**)rs_calloc(batch->unit_count ? batch->unit_count : 1, sizeof(rs_source*));
	for (i = 0; i < batch->function_count; i++) rs_compile_function(&compiler, &batch->functions[i]);
	/* the initializers of the globals, in order: one chunk per unit (positions are in their unit) */
	batch->inits = (rs_chunk**)rs_calloc(batch->unit_count ? batch->unit_count : 1, sizeof(rs_chunk*));
	for (i = 0; i < batch->global_count; i++) {
		rs_node* node = batch->globals[i].node;
		rs_chunk* chunk = batch->inits[node->unit];
		if (!chunk) {
			chunk = rs_chunk_create("<initializers>");
			chunk->source = rs_batch_source(batch, node->unit);
			rs_source_retain(chunk->source);
			batch->inits[node->unit] = chunk;
		}
		compiler.chunk = chunk;
		if (node->a) rs_compile_expression(&compiler, node->a);
		else rs_compile_default(&compiler, node->type, node);
		rs_emit(&compiler, OP_STORE_GLOBAL, (int32_t)i, 1, node);
	}
	for (i = 0; i < batch->unit_count; i++) {
		if (!batch->inits[i]) continue;
		compiler.chunk = batch->inits[i];
		rs_emit(&compiler, OP_RETURN_VOID, 0, 0, NULL);
	}
	compiler.chunk = NULL;
	rs_free(compiler.loops);
}

/* ---------------------------------------------------------------------------
 *  Validation
 * ------------------------------------------------------------------------- */

static int rs_validate_fail(char* error, size_t size, const rs_chunk* chunk, uint32_t at, const char* message) {
	if (error && size) snprintf(error, size, "invalid bytecode in '%s' at %u: %s", chunk->name ? chunk->name : "?", at, message);
	return 0;
}

/** The deepest a chunk may use the stack (beyond its locals). */
#define RS_MAX_CHUNK_STACK 4096

int rs_chunk_validate(const rs_chunk* chunk, uint32_t global_count, char* error, size_t error_size) {
	int32_t* depth;
	uint32_t* work;
	uint32_t work_count = 0, i;
	int max_depth = 0, ok = 1;
	if (chunk->count == 0) return rs_validate_fail(error, error_size, chunk, 0, "empty");
	if (chunk->param_count < 0 || chunk->param_count > RS_MAX_PARAMS || chunk->local_count < chunk->param_count || chunk->local_count > 65535) {
		return rs_validate_fail(error, error_size, chunk, 0, "bad locals");
	}
	/* operands */
	for (i = 0; i < chunk->count; i++) {
		const rs_instr* instr = &chunk->code[i];
		switch (instr->op) {
			case OP_CONST:
				if (instr->a < 0 || (uint32_t)instr->a >= chunk->constant_count) return rs_validate_fail(error, error_size, chunk, i, "constant out of range");
				break;
			case OP_LOAD_LOCAL:
			case OP_STORE_LOCAL:
				if (instr->a < 0 || instr->a >= chunk->local_count) return rs_validate_fail(error, error_size, chunk, i, "local out of range");
				break;
			case OP_LOAD_GLOBAL:
			case OP_STORE_GLOBAL:
				if (instr->b != 0) return rs_validate_fail(error, error_size, chunk, i, "global not relocated");
				if (instr->a < 0 || (uint32_t)instr->a >= global_count) return rs_validate_fail(error, error_size, chunk, i, "global out of range");
				break;
			case OP_JUMP:
			case OP_JUMP_IF_FALSE:
			case OP_AND:
			case OP_OR:
				if (instr->a < 0 || (uint32_t)instr->a >= chunk->count) return rs_validate_fail(error, error_size, chunk, i, "jump out of range");
				break;
			case OP_ARRAY:
				if (instr->a < 0 || instr->a > 65535) return rs_validate_fail(error, error_size, chunk, i, "bad array size");
				if ((instr->b & 0xFF) > RS_T_STRING || (instr->b >> 8) >= RS_MAX_ARRAY_DEPTH || (instr->b & 0xFF) < RS_T_BOOL) return rs_validate_fail(error, error_size, chunk, i, "bad array type");
				break;
			case OP_CALL:
			case OP_CALL_EXTERNAL:
				if (instr->a < 0 || (uint32_t)instr->a >= chunk->constant_count || chunk->constants[instr->a].type != RS_V_STRING) return rs_validate_fail(error, error_size, chunk, i, "bad function name");
				if (instr->b < 0 || instr->b > RS_MAX_PARAMS) return rs_validate_fail(error, error_size, chunk, i, "bad argument count");
				if ((instr->c & 0xFF) > RS_T_STRING || (instr->c >> 8) > RS_MAX_ARRAY_DEPTH || instr->c < 0) return rs_validate_fail(error, error_size, chunk, i, "bad return type");
				break;
			case OP_METHOD:
				if (instr->a < 0 || instr->a >= RS_M_COUNT_) return rs_validate_fail(error, error_size, chunk, i, "bad method");
				if (instr->b < 0 || instr->b > 2) return rs_validate_fail(error, error_size, chunk, i, "bad argument count");
				break;
			case OP_INTRINSIC:
				if (instr->a < 0 || instr->a >= RS_I_COUNT_) return rs_validate_fail(error, error_size, chunk, i, "bad intrinsic");
				if (instr->b < 0 || instr->b > RS_MAX_PARAMS) return rs_validate_fail(error, error_size, chunk, i, "bad argument count");
				break;
			default:
				if (instr->op >= OP_COUNT_) return rs_validate_fail(error, error_size, chunk, i, "unknown instruction");
				break;
		}
	}
	/* stack depth along every path */
	depth = (int32_t*)rs_malloc(chunk->count * sizeof(int32_t));
	work = (uint32_t*)rs_malloc(chunk->count * sizeof(uint32_t));
	for (i = 0; i < chunk->count; i++) depth[i] = -1;
	depth[0] = 0;
	work[work_count++] = 0;
	while (ok && work_count > 0) {
		uint32_t at = work[--work_count];
		for (;;) {
			const rs_instr* instr = &chunk->code[at];
			int32_t d = depth[at], need = 0, after = d;
			int falls = 1, jumps = 0;
			int32_t jump_depth = 0;
			switch (instr->op) {
				case OP_NOP: break;
				case OP_CONST: case OP_NULL: case OP_TRUE: case OP_FALSE: case OP_LOAD_LOCAL: case OP_LOAD_GLOBAL: after = d + 1; break;
				case OP_POP: case OP_STORE_LOCAL: case OP_STORE_GLOBAL: need = 1; after = d - 1; break;
				case OP_DUP2: need = 2; after = d + 2; break;
				case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
				case OP_EQ: case OP_NE: case OP_LT: case OP_LE: case OP_GT: case OP_GE: case OP_INDEX:
					need = 2; after = d - 1; break;
				case OP_NEG: case OP_NOT: need = 1; break;
				case OP_JUMP: falls = 0; jumps = 1; jump_depth = d; break;
				case OP_JUMP_IF_FALSE: need = 1; after = d - 1; jumps = 1; jump_depth = d - 1; break;
				case OP_AND: case OP_OR: need = 1; after = d - 1; jumps = 1; jump_depth = d; break;
				case OP_ARRAY: need = instr->a; after = d - instr->a + 1; break;
				case OP_STORE_INDEX: need = 3; after = d - 3; break;
				case OP_CALL: case OP_CALL_EXTERNAL: case OP_INTRINSIC: need = instr->b; after = d - instr->b + 1; break;
				case OP_METHOD: need = instr->b + 1; after = d - instr->b; break;
				case OP_RETURN: need = 1; falls = 0; break;
				case OP_RETURN_VOID: case OP_MISSING_RETURN: falls = 0; break;
				default: ok = rs_validate_fail(error, error_size, chunk, at, "unknown instruction"); break;
			}
			if (!ok) break;
			if (d < need) {
				ok = rs_validate_fail(error, error_size, chunk, at, "stack underflow");
				break;
			}
			if (after > max_depth) max_depth = after;
			if (max_depth > RS_MAX_CHUNK_STACK) {
				ok = rs_validate_fail(error, error_size, chunk, at, "stack too deep");
				break;
			}
			if (jumps) {
				uint32_t target = (uint32_t)instr->a;
				if (depth[target] == -1) {
					depth[target] = jump_depth;
					work[work_count++] = target;
				} else if (depth[target] != jump_depth) {
					ok = rs_validate_fail(error, error_size, chunk, at, "inconsistent stack at a jump");
					break;
				}
			}
			if (!falls) break;
			if (at + 1 >= chunk->count) {
				ok = rs_validate_fail(error, error_size, chunk, at, "runs past the end");
				break;
			}
			if (depth[at + 1] == -1) {
				depth[at + 1] = after;
				at++;
				continue;
			}
			if (depth[at + 1] != after) ok = rs_validate_fail(error, error_size, chunk, at, "inconsistent stack");
			break;
		}
	}
	rs_free(depth);
	rs_free(work);
	if (ok) ((rs_chunk*)chunk)->max_stack = max_depth;
	return ok;
}
