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
			(AST) { .type = AST_LIST, .AST_LIST = astlist(p->arena) });

	while (p->cur_token.type != END) {
		tmp = parse_statement(p);
		if (tmp) 
			astpush(program->AST_LIST, tmp);
		next_token(p);
	}
	return program;
}

priv AST *parse_expression_stmt(Parser *p);
priv AST *parse_statement(Parser *p)
{
	switch (p->cur_token.type) {
		case TK_VAL:
		case TK_VAR:
		case TK_RETURN:
			return NULL;
		default:
			return parse_expression_stmt(p);
	}
}

priv AST *parse_expression(Parser *p, Precedence prec);
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

priv void next_token(Parser *p)
{
	p->cur_token = p->next_token;
	p->next_token = lexer_token(p->lexer);
}

priv i64 str_atol(String s);
priv bool is_num(char c);
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
	return ast_alloc(p->arena, (AST) { AST_LIST, .AST_LIST = parse_many(p, RBRACKET) });
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

priv AST *parse_infix_expression(Parser *p, AST *left)
{
	AST	*res = ast_alloc(p->arena, (AST) { AST_INFIX, .AST_INFIX = { .left = left, .op = p->cur_token.lit, .right = NULL }});
	Precedence prec = PRECEDENCE(p->cur_token.type);
	next_token(p);
	res->AST_INFIX.right = parse_expression(p, prec);
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
		case TRUE:
		case FALSE:
			return &parse_bool;
		case BANG:
		case MINUS:
			return &parse_prefix_expression;
		case LPAREN:
			return &parse_grouped_expression;
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
		/* case LPAREN: */
		/* 	return &parse_call_expression; */
		/* case LBRACKET: */
		/* 	return &parse_index_expression; */
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

// HELPERS
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

priv bool is_num(char c)
{
	return (('0' <= c) && (c <= '9'));
}

priv i64 str_atol(String s)
{
	i64 x = 0;
	u64 i = 0;
	while (is_num(s.buf[i]) && i < s.len) {
		x = x * 10 + (s.buf[i] - '0');
		i++;
	}
	return x;
}
