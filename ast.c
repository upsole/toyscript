#include "base.h"
#include "toyscript.h"

// Types & LUTS 
typedef enum Precedence { LOWEST, EQUALS, LESSGREATER, SUM, PRODUCT, PREFIX, CALL, INDEX } Precedence;
typedef AST* (*PrefixParser)(Parser *p);
typedef AST* (*InfixParser) (Parser *p, AST *left);
priv PrefixParser PREFIX_PARSERS(TokenType type);
priv InfixParser INFIX_PARSERS(TokenType type);
priv Precedence PRECEDENCE(TokenType type);

// HEADERS
priv ASTList *astlist(Arena *a);
priv void astpush(ASTList *l, AST *ast);
priv AST *ast_alloc(Arena *a, AST node); 
priv void next_token(Parser *p);
priv bool expect_peek(Parser *p, TokenType t);
priv void parser_error(Parser *p, String msg);

// ~PARSER
Parser	*parser(Arena *a, Lexer *l)
{
	Parser *p = arena_alloc(a, sizeof(Parser));
	p->arena = a;
	p->lexer = l;
	p->errors = NULL;

	p->next_token = (Token) {0};

	next_token(p);
	next_token(p);
	return p;
}

priv AST *parse_statement(Parser *p);
AST	*parse_program(Parser *p)
{
	AST	*tmp;
	AST	*program = ast_alloc(p->arena,  
			(AST) { .type = AST_PROGRAM, .AST_LIST = astlist(p->arena) });

	while (p->cur_token.type != END) {
		tmp = parse_statement(p);
		if (tmp) 
			astpush(program->AST_LIST, tmp);
		next_token(p);
	}
	return program;
}

ASTList *parse_block_statement(Parser *p)
{
	AST	*tmp;
	ASTList	*block = astlist(p->arena);
	next_token(p);
	while (p->cur_token.type != RBRACE) {
		tmp = parse_statement(p);
		if (tmp) 
			astpush(block, tmp);
		next_token(p);
	}
	return block;
}

priv AST *parse_return(Parser *p);
priv AST *parse_binding(Parser *p, TokenType type);
priv AST *parse_expression_stmt(Parser *p);
priv AST *parse_statement(Parser *p)
{
	switch (p->cur_token.type) {
		case TK_RETURN:
			return parse_return(p);
		case TK_VAL:
		case TK_VAR:
			return parse_binding(p, p->cur_token.type);
		default:
			return parse_expression_stmt(p);
	}
}

priv AST *parse_expression(Parser *p, Precedence prec);
priv AST *parse_return(Parser *p)
{
	u64	previous_offset = p->arena->used;
	next_token(p);
	AST *res = ast_alloc(p->arena, (AST) { AST_RETURN, .AST_RETURN = { NULL }});
	AST *right = parse_expression(p, LOWEST);
	if (!right) {
		arena_pop_to(p->arena, previous_offset);
		return NULL;
	}
	res->AST_RETURN.value = right;
	if (p->next_token.type == SEMICOLON) next_token(p);
	return res;
}

priv AST *parse_binding(Parser *p, TokenType type)
{
	u64	previous_offset = p->arena->used;
	AST	*res; 
	if (!expect_peek(p, TK_IDENT)) return NULL;

	switch (type) {
		case TK_VAL:
			res = ast_alloc(p->arena, (AST) { AST_VAL, .AST_VAL = { p->cur_token.lit, NULL}});
			break;
		case TK_VAR:
			res = ast_alloc(p->arena, (AST) { AST_VAR, .AST_VAR = { p->cur_token.lit, NULL}});
			break;
		default:
			return NULL;
	}
	if (!expect_peek(p, ASSIGN)) {
		arena_pop_to(p->arena, previous_offset);
		return NULL;
	}
	next_token(p);
	AST *right = parse_expression(p, LOWEST);
	if (!right) {
		arena_pop_to(p->arena, previous_offset);
		return NULL;
	} 
	switch (type) {
		case TK_VAL:
			res->AST_VAL.value = right;
			break;
		case TK_VAR:
			res->AST_VAR.value = right;
			break;
		default:
			return NULL;
	}
	if (p->next_token.type == SEMICOLON) next_token(p);
	return res;
}

priv AST *parse_expression_stmt(Parser *p)
{
	AST	*res = parse_expression(p, LOWEST);
	if (p->next_token.type == SEMICOLON) next_token(p);
	return res;
}

priv AST *parse_expression(Parser *p, Precedence prec)
{
	AST	*res = NULL;
	PrefixParser pfix_parser = PREFIX_PARSERS(p->cur_token.type);
	if (!pfix_parser) {
		parser_error(p, CONCAT(p->arena, str("Parsing error - prefix not recognized: "), 
					token_str(p->cur_token.type)));
		return NULL;
	}
	res = pfix_parser(p);
	while ((p->next_token.type != SEMICOLON) && 
			prec < PRECEDENCE(p->next_token.type)) {
		InfixParser ifix_parser = INFIX_PARSERS(p->next_token.type);
		if (!ifix_parser) return res;
		next_token(p);
		res = ifix_parser(p, res);
	}
	return res;
}

priv AST *parse_int(Parser *p)
{
	return ast_alloc(p->arena, (AST) { AST_INT, .AST_INT = {str_atol(p->cur_token.lit)} });
}

priv AST *parse_bool(Parser *p)
{
	return ast_alloc(p->arena, (AST) { AST_BOOL, .AST_BOOL = {p->cur_token.type == TRUE}});
}

priv AST *parse_string(Parser *p)
{
	return ast_alloc(p->arena, (AST) { AST_STR, .AST_STR =  p->cur_token.lit });
}

priv AST *parse_ident(Parser *p)
{
	return ast_alloc(p->arena, (AST) { AST_IDENT, .AST_STR =  p->cur_token.lit });
}

priv ASTList *parse_many(Parser *p, TokenType end_type);
priv AST *parse_list(Parser *p)
{
	ASTList	*lst = parse_many(p, RBRACKET);
	if (!lst)
		return NULL;
	return ast_alloc(p->arena, (AST) { AST_LIST, .AST_LIST = lst });
}

priv ASTList *parse_many(Parser *p, TokenType end_type)
{
	u64	previous_offset = p->arena->used;
	ASTList *lst = astlist(p->arena);
	if (p->next_token.type == end_type) // empty list
		{ next_token(p); return lst; }
	next_token(p);
	astpush(lst, parse_expression(p, LOWEST));
	while (p->next_token.type == COMMA) {
		next_token(p), next_token(p);
		astpush(lst, parse_expression(p, LOWEST));
	}
	if (!expect_peek(p, end_type)) {
		arena_pop_to(p->arena, previous_offset); // Failed to build list, resetting arena to previous state XXX test this
		return NULL;
	}
	return lst;
}

priv AST *parse_prefix_expression(Parser *p)
{
	AST	*res = ast_alloc(p->arena, (AST) { AST_PREFIX, .AST_PREFIX = { p->cur_token.lit , NULL } });

	next_token(p);

	res->AST_PREFIX.right = parse_expression(p, PREFIX);
	return res;
}

priv AST *parse_grouped_expression(Parser *p)
{
	AST	*res = NULL;
	next_token(p);
	res = parse_expression(p, LOWEST);
	if (!expect_peek(p, RPAREN)) return NULL;
	return res;
}

priv ASTList *parse_function_params(Parser *p)
{
	u64 previous_offset = p->arena->used;
	ASTList	*params = astlist(p->arena);
	AST	*tmp = NULL;
	if (p->next_token.type == RPAREN)
		return (next_token(p), params);
	next_token(p);
	if (p->cur_token.type == TK_IDENT)
		tmp = parse_ident(p);
	else
		return (arena_pop_to(p->arena, previous_offset), NULL);
	while (p->next_token.type == COMMA) {
		next_token(p), next_token(p);
		if (p->cur_token.type == TK_IDENT) {
			tmp = parse_ident(p);
			astpush(params, tmp);
		}
		else
			return (arena_pop_to(p->arena, previous_offset), NULL);
	}
	if (!expect_peek(p, RPAREN))
		return (arena_pop_to(p->arena, previous_offset), NULL);
	return params;
}

priv AST *parse_function(Parser *p)
{
	u64	previous_offset = p->arena->used;
	AST	*res = ast_alloc(p->arena, (AST) { AST_FN, .AST_FN ={0}});
	if (!expect_peek(p, LPAREN))
		return (arena_pop_to(p->arena, previous_offset), NULL);
	ASTList	*params = parse_function_params(p);
	if (!params) return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_FN.params = params;
	if (!expect_peek(p, LBRACE))
		return (arena_pop_to(p->arena, previous_offset), NULL);
	ASTList	*body = parse_block_statement(p);
	if (!body) return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_FN.body = body;
	return res;
}

priv AST *parse_if_expression(Parser *p)
{
	u64 previous_offset = p->arena->used;
	AST	*res;
	if (!expect_peek(p, LPAREN)) return NULL;
	next_token(p);
	res = ast_alloc(p->arena, (AST) { AST_COND, .AST_COND = {0} });

	AST	*condition = parse_expression(p, LOWEST);
	if (!condition) 
		return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_COND.condition = condition;

	if (!expect_peek(p, RPAREN))
		return (arena_pop_to(p->arena, previous_offset), NULL);
	if (!expect_peek(p, LBRACE))
		return (arena_pop_to(p->arena, previous_offset), NULL);

	ASTList	*consequence = parse_block_statement(p);
	if (!consequence)
		return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_COND.consequence = consequence;

	if (p->next_token.type == ELSE) {
		next_token(p);
		if (!expect_peek(p, LBRACE))
			return (arena_pop_to(p->arena, previous_offset), NULL);
		ASTList *alternative = parse_block_statement(p);
		if (!alternative)
			return (arena_pop_to(p->arena, previous_offset), NULL);
		res->AST_COND.alternative = alternative;
	}
	return res;
}

priv AST *parse_infix_expression(Parser *p, AST *left)
{
	AST	*res = ast_alloc(p->arena, (AST) { AST_INFIX, .AST_INFIX = { .left = left, .op = p->cur_token.lit, .right = NULL }});
	Precedence prec = PRECEDENCE(p->cur_token.type);
	next_token(p);
	res->AST_INFIX.right = parse_expression(p, prec);
	return res;
}

priv AST *parse_call_expression(Parser *p, AST *function)
{
	u64	previous_offset = p->arena->used;
	AST	*res = ast_alloc(p->arena, (AST) { AST_CALL, .AST_CALL = { function, NULL } });
	ASTList	*args = parse_many(p, RPAREN);
	if (!args) 
		return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_CALL.args = args;
	return res;
}

priv AST *parse_index_expression(Parser *p, AST *lst)
{
	u64	previous_offset = p->arena->used;
	AST	*res = ast_alloc(p->arena, (AST) { AST_INDEX, .AST_INDEX = { lst, NULL } });
	next_token(p);
	AST *index = parse_expression(p, LOWEST);
	if (!index || !expect_peek(p, RBRACKET)) 
		return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_INDEX.index = index;
	return res;
}

// Tables
priv PrefixParser PREFIX_PARSERS(TokenType type)
{
	switch (type) {
		case TK_INT:
			return &parse_int;
		case TK_STRING:
			return &parse_string;
		case TK_IDENT:
			return &parse_ident;
		case LBRACKET:
			return &parse_list;
		case TK_FN:
			return &parse_function;
		case TRUE:
		case FALSE:
			return &parse_bool;
		case BANG:
		case MINUS:
			return &parse_prefix_expression;
		case LPAREN:
			return &parse_grouped_expression;
		case IF:
			return &parse_if_expression;
		default:
			return NULL;
	}
}

priv InfixParser INFIX_PARSERS(TokenType type)
{
	switch (type) {
		case PLUS:
		case MINUS:
		case STAR:
		case SLASH:
		case MOD:
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

priv Precedence PRECEDENCE(TokenType type)
{
	switch (type) {
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
		case MOD:
			return PRODUCT;
		case LPAREN:
			return CALL;
		case LBRACKET:
			return INDEX;
		default:
			return LOWEST;
	}
}

// ~AST
AST *ast_alloc(Arena *a, AST node)
{
	AST	*ptr = arena_alloc(a, sizeof(AST));
	*ptr = node;
	return ptr;
}

priv ASTList *astlist(Arena *a)
{
	ASTList *l = arena_alloc(a, sizeof(ASTList));
	l->arena = a;
	l->head = NULL;
	l->len = 0;
	return l;
}

priv void astpush(ASTList *l, AST *ast)
{
	ASTNode	*node = arena_alloc(l->arena, sizeof(ASTNode));
	node->ast = ast;
	node->next = NULL;

	if (NEVER(!l)); // XXX extend

	if (!l->head) {
		l->head = node;
		l->tail = node;
		l->len++;
		return ;
	}
	l->tail->next = node;
	l->tail = node;
	l->len++;
}

// PARSER INTERNALS
priv void next_token(Parser *p)
{
	p->cur_token = p->next_token;
	p->next_token = lexer_token(p->lexer);
}

priv bool expect_peek(Parser *p, TokenType type)
{
	if (p->next_token.type == type) {
		next_token(p);
		return true;
	} else {
		parser_error(p, CONCAT(p->arena,
					str("Parsing error - Wrong token after "),
					token_str(p->cur_token.type), 
					str(" got "), token_str(p->next_token.type),
					str(" expected "), token_str(type)));
		return false;
	}
}

priv void parser_error(Parser *p, String msg)
{
	if (!p->errors) p->errors = strlist(p->arena);
	strpush(p->errors, msg);
}

void parser_print_errors(Parser *p)
{
	str_print(str("Parser has errors.\n"));
	for (StrNode *tmp = p->errors->head; tmp; tmp = tmp->next)
		str_print(tmp->string), str_print(str("\n"));
}

priv void astlist_print(Arena *a, ASTList *lst)
{
	str_print(str("["));
	for (ASTNode *tmp = lst->head; tmp; tmp = tmp->next) {
		ast_aprint(a, tmp->ast); 
		if (tmp->next) str_print(str(" "));
	}
	str_print(str("]"));
}


void ast_aprint(Arena *a, AST *node)
{
	switch (node->type) {
		case AST_VAL:
			str_print(str("|"));
			str_print(node->AST_VAL.name);
			str_print(str("="));
			ast_aprint(a, node->AST_VAL.value);
			str_print(str("|"));
			break;
		case AST_VAR:
			str_print(str("|"));
			str_print(node->AST_VAR.name);
			str_print(str("="));
			ast_aprint(a, node->AST_VAR.value);
			str_print(str("|"));
			break;
		case AST_RETURN:
			str_print(str("|"));
			str_print(str("return "));
			ast_aprint(a, node->AST_RETURN.value);
			str_print(str("|"));
			break;
		case AST_INT:
			str_print(str_fmt(a, "%ld", node->AST_INT.value));
			break;
		case AST_BOOL:
			if (node->AST_BOOL.value) str_print(str("TRUE"));
			else str_print(str("FALSE"));
			break;
		case AST_STR:
		case AST_IDENT:
			str_print(node->AST_STR);
			break;
		case AST_PROGRAM:
		case AST_LIST:
			astlist_print(a, node->AST_LIST);
			break;
		case AST_PREFIX:
			str_print(str("("));
			str_print(node->AST_PREFIX.op);
			ast_aprint(a, node->AST_PREFIX.right);
			str_print(str(")"));
			break;
		case AST_INFIX:
			str_print(str("("));
			ast_aprint(a, node->AST_INFIX.left);
			str_print(node->AST_INFIX.op);
			ast_aprint(a, node->AST_INFIX.right);
			str_print(str(")"));
			break;
		case AST_FN:
			str_print(str("|"));
			str_print(str("fn"));
			astlist_print(a, node->AST_FN.params);
			str_print(str("->"));
			astlist_print(a, node->AST_FN.body);
			str_print(str("|"));
			break;
		case AST_CALL:
			str_print(str("("));
			ast_aprint(a, node->AST_CALL.function);
			str_print(str("<"));
			astlist_print(a, node->AST_CALL.args);
			str_print(str(">"));
			str_print(str(")"));
			break;
		case AST_COND:
			str_print(str("|"));
			str_print(str("if"));
			ast_aprint(a, node->AST_COND.condition);
			str_print(str("{"));
			astlist_print(a, node->AST_COND.consequence);
			str_print(str("}"));
			if (node->AST_COND.alternative) {
				str_print(str(" else"));
				str_print(str("{"));
				astlist_print(a, node->AST_COND.alternative);
				str_print(str("}"));
			}
			str_print(str("|"));
			break;
		case AST_INDEX:
			str_print(str("("));
			ast_aprint(a, node->AST_INDEX.left);
			str_print(str("["));
			ast_aprint(a, node->AST_INDEX.index);
			str_print(str("]"));
			str_print(str(")"));
			break;
		default:
			str_print(str("Unknown"));
	}
}
