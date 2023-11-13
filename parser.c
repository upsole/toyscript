#include "toyscript.h"

typedef Expression (*PrefixParser)(Parser *p);
typedef Expression (*InfixParser)(Parser *p, Expression ex);

Expression parse_identifier(Parser *p);
Expression parse_int(Parser *p);
Expression parse_string(Parser *p);
Expression parse_list(Parser *p);
Expression parse_function(Parser *p);
Expression parse_boolean(Parser *p);
Expression parse_grouped_expression(Parser *p);
Expression parse_cond_expression(Parser *p);
Expression parse_prefix_expression(Parser *p);
Expression parse_index_expression(Parser *p, Expression ex);
Expression parse_infix_expression(Parser *p, Expression ex);
Expression parse_call_expression(Parser *p, Expression ex);

Expression *alloc_expression(Parser *p, Expression ex);

PrefixParser prefix_parser(TokenType t)
{
	switch (t) {
		case IDENT:
			return &parse_identifier;
		case INT:
			return &parse_int;
		case STRING:
			return &parse_string;
		case TRUE:
		case FALSE:
			return &parse_boolean;
		case LBRACKET:
			return &parse_list;
		case BANG:
		case MINUS:
			return &parse_prefix_expression;
		case LPAREN:
			return &parse_grouped_expression;
		case FN:
			return &parse_function;
		case IF:
			return &parse_cond_expression;
		default:
			return NULL;
	}
}

InfixParser infix_parser(TokenType t)
{
	switch (t) {
		case PLUS:
		case MINUS:
		case STAR:
		case SLASH:
		case EQ:
		case NOT_EQ:
		case LT:
		case GT:
			return &parse_infix_expression;
		case LPAREN:
			return &parse_call_expression;
		case LBRACKET:
			return &parse_index_expression;
		default:
			return NULL;
	}
}

Precedence precedence(TokenType t)
{
	switch (t) {
		case EQ:
			return EQUALS;
		case NOT_EQ:
			return EQUALS;
		case LT:
			return LESSGREATER;
		case GT:
			return LESSGREATER;
		case PLUS:
			return SUM;
		case MINUS:
			return SUM;
		case STAR:
		case SLASH:
			return PRODUCT;
		case LPAREN:
			return CALL;
		case LBRACKET:
			return INDEX;
		default:
			return LOWEST;
	}
}

Parser *parser_init(Arena *a, Lexer *l)
{
	Parser *p = arena_alloc(a, sizeof(Parser));
	p->arena = a;
	p->peek_token = (Token){0};
	p->l = l;
	p->errors = NULL;

	// Init
	parse_next_tok(p);
	parse_next_tok(p);
	return p;
}

void parse_next_tok(Parser *p)
{
	p->cur_token = p->peek_token;
	p->peek_token = lex_tok(p->l);
}

Program parse_program(Parser *p)
{
	Statement s = {0};
	Program prog = {0};
	prog.statements = stmtlist(p->arena);

	while (p->cur_token.type != END) {
		s = parse_statement(p);
		if (s.type != STMT_ERR) {
			stmt_push(prog.statements, s);
		}
		parse_next_tok(p);
	}
	return prog;
}

Statement parse_program_stmt(Parser *p)
{
	return (Statement){.type = PROGRAM, ._program = parse_program(p)};
}

Statement parse_statement(Parser *p)
{
	switch (p->cur_token.type) {
		case VAL:
			return parse_val_stmt(p);
		case RETURN:
			return parse_return_stmt(p);
		default:
			return parse_expression_stmt(p);
	}
}

Statement parse_val_stmt(Parser *p) // TODO refactor returns
{
	ValStatement v = {0};
	Statement s = {0};

	v.tok = p->cur_token;
	if (!expect_peek(p, IDENT)) return (s.type = STMT_ERR, s);
	v.name = (Identifier){.tok = p->cur_token, .value = p->cur_token.lit};

	if (!expect_peek(p, ASSIGN)) return (s.type = STMT_ERR, s);
	parse_next_tok(p);
	v.value = parse_expression(p, LOWEST);
	if (v.value.type == EXP_ERROR) return (Statement){.type = STMT_ERR};
	s._val = v;
	s.type = STMT_VAL;
	if (p->peek_token.type == SEMICOLON) parse_next_tok(p);
	return s;
}

Statement parse_return_stmt(Parser *p) // TODO refactor return
{
	ReturnStatement r = {0};
	Statement s = {0};

	r.tok = p->cur_token;
	parse_next_tok(p);

	r.value = parse_expression(p, LOWEST);
	if (r.value.type == EXP_ERROR) return (Statement){.type = STMT_ERR};
	s._return = r;
	s.type = STMT_RET;
	if (p->peek_token.type == SEMICOLON) parse_next_tok(p);
	return s;
}

Statement parse_expression_stmt(Parser *p)
{
	Statement s = {0};

	s._exp = parse_expression(p, LOWEST);
	s.type = STMT_EXP;
	if (p->peek_token.type == SEMICOLON) parse_next_tok(p);
	return s;
}

Expression parse_expression(Parser *p, Precedence prec)
{
	Expression ex = {0};
	PrefixParser pfix_parser = prefix_parser(p->cur_token.type);
	if (!pfix_parser) {
		parser_add_error(
		    p, str_concatv(p->arena, 3,
				   str("Parser error: Prefix not recognized. "
				       "(No function for this prefix "),
				   tok_string(p->cur_token.type), str(")")));
		return (ex.type = EXP_ERROR, ex);
	}
	ex = pfix_parser(p);

	while ((p->peek_token.type != SEMICOLON) &&
	       prec < precedence(p->peek_token.type)) {
		InfixParser ifix_parser = infix_parser(p->peek_token.type);
		if (!ifix_parser) return ex;
		parse_next_tok(p);
		ex = ifix_parser(p, ex);
	}
	return ex;
}

Expression parse_identifier(Parser *p)
{
	return (Expression){
	    .type = EXP_IDENT,
	    ._ident = {.tok = p->cur_token, .value = p->cur_token.lit}};
}

Expression parse_int(Parser *p)
{
	return (Expression){
	    .type = EXP_INT,
	    ._int = {.tok = p->cur_token, .value = str_atol(p->cur_token.lit)}};
}

Expression parse_string(Parser *p)
{
	return (Expression){
	    .type = EXP_STR,
	    ._string = {.tok = p->cur_token, .value = p->cur_token.lit}};
}

Expression parse_boolean(Parser *p)
{
	return (Expression){.type = EXP_BOOL,
			    ._bool = {.tok = p->cur_token,
				      .value = (p->cur_token.type == TRUE)}};
}

ExpList *parse_explist(Parser *p, TokenType end_type);
Expression parse_list(Parser *p)
{
	Expression res = {0};
	res.type = EXP_LIST;
	res._list.tok = p->cur_token;
	res._list.items = parse_explist(p, RBRACKET);
	return res;
}

ExpList *parse_explist(Parser *p, TokenType end_type)
{ // XXX in these kind of list build that can fail: maybe we could track
  // starting point of arena and pop back to that point if it fails
	ExpList *res = explist(p->arena);
	if (p->peek_token.type == end_type) {
		parse_next_tok(p);
		return res;
	}
	parse_next_tok(p);
	exp_push(res, parse_expression(p, LOWEST));
	while (p->peek_token.type == COMMA) {
		parse_next_tok(p);
		parse_next_tok(p);
		exp_push(res, parse_expression(p, LOWEST));
	}
	if (!expect_peek(p, end_type)) return NULL;
	return res;
}

BlockStatement *parse_block_stmt(Parser *p)
{
	BlockStatement *block = arena_alloc(p->arena, sizeof(BlockStatement));
	block->tok = p->cur_token;
	block->statements = stmtlist(p->arena);
	parse_next_tok(p);
	Statement s = {0};
	while ((p->cur_token.type) != RBRACE) {
		s = parse_statement(p);
		if (s.type != STMT_ERR) {
			stmt_push(block->statements, s);
		}
		parse_next_tok(p);
	}
	return block;
}

IdentList *parse_function_params(Parser *p);
Expression parse_function(Parser *p)
{
	FunctionLiteral func = {0};
	if (!expect_peek(p, LPAREN)) return (Expression){.type = EXP_ERROR};
	func.params = parse_function_params(p);
	if (!expect_peek(p, LBRACE)) return (Expression){.type = EXP_ERROR};
	func.body = parse_block_stmt(p);
	return (Expression){.type = EXP_FN, ._fn = func};
}

IdentList *parse_function_params(Parser *p)
{
	IdentList *params = identlist(p->arena);
	if (p->peek_token.type == RPAREN) {
		parse_next_tok(p);
		return params;
	}
	parse_next_tok(p);
	Identifier ident = {.tok = p->cur_token, .value = p->cur_token.lit};
	ident_push(params, ident);
	while (p->peek_token.type == COMMA) {
		parse_next_tok(p);
		parse_next_tok(p);
		ident = (Identifier){.tok = p->cur_token,
				     .value = p->cur_token.lit};
		ident_push(params, ident);
	}
	if (!expect_peek(p, RPAREN)) {
		return NULL;
	}
	return params;
}

Expression parse_prefix_expression(Parser *p)
{
	PrefixExp pfix = {0};
	pfix.tok = p->cur_token;
	pfix.operator= p->cur_token.lit;

	parse_next_tok(p);

	Expression right = parse_expression(p, PREFIX);
	pfix.right = alloc_expression(p, right);
	return (Expression){.type = EXP_PREFIX, ._prefix = pfix};
}

Expression parse_grouped_expression(Parser *p)
{
	Expression ex = {0};
	parse_next_tok(p);

	ex = parse_expression(p, LOWEST);
	if (!expect_peek(p, RPAREN)) {
		return (ex.type = EXP_ERROR, ex);
	}
	return ex;
}

Expression parse_cond_expression(Parser *p)
{
	ConditionalExp ex = {0};
	ex.tok = p->cur_token;
	if (!expect_peek(p, LPAREN)) {
		return (Expression){.type = EXP_ERROR};
	}
	parse_next_tok(p);
	Expression condition = parse_expression(p, LOWEST);
	ex.condition = alloc_expression(p, condition);
	if (!expect_peek(p, RPAREN)) {
		return (Expression){.type = EXP_ERROR};
	}
	if (!expect_peek(p, LBRACE)) {
		return (Expression){.type = EXP_ERROR};
	}
	ex.consequence = parse_block_stmt(p);
	if (p->peek_token.type == ELSE) {
		parse_next_tok(p);
		if (!expect_peek(p, LBRACE))
			return (Expression){.type = EXP_ERROR};
		ex.alternative = parse_block_stmt(p);
	}
	return (Expression){.type = EXP_COND, ._conditional = ex};
}

Expression parse_call_expression(Parser *p, Expression ex)
{
	CallExp call = {0};
	call.tok = p->cur_token;
	call.function = alloc_expression(p, ex);
	call.arguments = parse_explist(p, RPAREN);
	return (Expression){.type = EXP_CALL, ._call = call};
}

Expression parse_infix_expression(Parser *p, Expression ex)
{
	InfixExp infix = {0};
	infix.tok = p->cur_token;
	infix.operator= p->cur_token.lit;
	infix.left = alloc_expression(p, ex);
	Precedence prec = precedence(p->cur_token.type);
	parse_next_tok(p);
	Expression right = parse_expression(p, prec);
	Expression *r = alloc_expression(p, right);
	infix.right = r;
	return (Expression){.type = EXP_INFIX, ._infix = infix};
}

Expression parse_index_expression(Parser *p, Expression ex)
{
	Expression res = {0};
	res.type = EXP_INDEX;
	res._index.left = alloc_expression(p, ex);

	parse_next_tok(p);
	res._index.index = alloc_expression(p, parse_expression(p, LOWEST));
	if (!expect_peek(p, RBRACKET)) return (Expression){.type = EXP_ERROR};
	return res;
}

bool expect_peek(Parser *p, TokenType t)
{
	if (p->peek_token.type == t) {
		parse_next_tok(p);
		return true;
	} else {
		parser_add_error(
		    p, str_concatv(p->arena, 6,
				   str("Parser error: Wrong token type after "),
				   tok_string(p->cur_token.type), str(" got "),
				   tok_string(p->peek_token.type),
				   str(" expected "), tok_string(t)));
		return false;
	}
}

Expression *alloc_expression(Parser *p, Expression ex)
{
	Expression *ptr = arena_alloc(p->arena, sizeof(ex));

	ptr->type = ex.type;
	switch (ex.type) {
		case EXP_INFIX:
			ptr->_infix = ex._infix;
			break;
		case EXP_PREFIX:
			ptr->_prefix = ex._prefix;
			break;
		case EXP_INT:
			ptr->_int = ex._int;
			break;
		case EXP_LIST:
			ptr->_list = ex._list;
			break;
		case EXP_INDEX:
			ptr->_index = ex._index;
			break;
		case EXP_STR:
			ptr->_string = ex._string;
			break;
		case EXP_IDENT:
			ptr->_ident = ex._ident;
			break;
		case EXP_BOOL:
			ptr->_bool = ex._bool;
			break;
		case EXP_CALL:
			ptr->_call = ex._call;
			break;
		case EXP_FN:
			ptr->_fn = ex._fn;
			break;
		default:
			ptr = NULL;
			break;
	}
	return ptr;
}

// Error Handling
StrList *strlist(Arena *a)
{
	StrList *list = arena_alloc(a, sizeof(StrList));
	list->arena = a;
	list->head = NULL;
	list->tail = NULL;
	list->len = 0;
	return list;
}

void str_push(StrList *l, String string)
{
	StrNode *node = arena_alloc(l->arena, sizeof(StrNode));
	node->s = string;
	node->next = NULL;
	if (!l->head) {
		l->head = node;
		l->tail = node;
		l->len++;
		return;
	}
	l->tail->next = node;
	l->tail = node;
	l->len++;
}

void parser_add_error(Parser *p, String msg)
{
	if (!p->errors) {
		p->errors = strlist(p->arena);
	}
	str_push(p->errors, msg);
}

void strlist_print(StrList *list)
{
	if (!list) return;
	for (StrNode *cursor = list->head; cursor; cursor = cursor->next) {
		str_print(cursor->s);
		printf("\n");
	}
}
