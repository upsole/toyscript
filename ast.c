#include "base.h"
#include "toyscript.h"

// Types & LUTS 
typedef enum Precedence { LOWEST, ASSIGNMENT, EQUALS, LESSGREATER, SUM, PRODUCT, PREFIX, CALL, INDEX } Precedence;
typedef AST* (*PrefixParser)(Parser *p);
typedef AST* (*InfixParser) (Parser *p, AST *left);
priv PrefixParser PREFIX_PARSERS(TokenType type);
priv InfixParser INFIX_PARSERS(TokenType type);
priv Precedence PRECEDENCE(TokenType type);

// HEADERS
ASTList *astlist(Arena *a);
void astpush(ASTList *l, AST *ast);
AST *ast_alloc(Arena *a, AST node); 
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

	while (p->cur_token.type != TK_END) {
		tmp = parse_statement(p);
		if (p->errors) return NULL;
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
	while (p->cur_token.type != TK_RBRACE) {
		tmp = parse_statement(p);
		if (tmp) 
			astpush(block, tmp);
		next_token(p);
	}
	if (p->next_token.type == TK_SEMICOLON) next_token(p);
	return block;
}

priv AST *parse_return(Parser *p);
priv AST *parse_binding(Parser *p, TokenType type);
priv AST *parse_expression_stmt(Parser *p);
priv AST *parse_while_statement(Parser *p);
priv AST *parse_statement(Parser *p)
{
	switch (p->cur_token.type) {
		case TK_RETURN:
			return parse_return(p);
		case TK_VAL:
		case TK_VAR:
			return parse_binding(p, p->cur_token.type);
		case TK_WHILE:
			return parse_while_statement(p);
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
	AST *right;
	if (p->cur_token.type == TK_SEMICOLON) {
		right = ast_alloc(p->arena, (AST) { AST_NULL });
	} else
		right = parse_expression(p, LOWEST);
	if (!right) {
		arena_pop_to(p->arena, previous_offset);
		return NULL;
	}
	res->AST_RETURN.value = right;
	if (p->next_token.type == TK_SEMICOLON) next_token(p);
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
	if (!expect_peek(p, TK_ASSIGN)) {
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
	if (p->next_token.type == TK_SEMICOLON) next_token(p);
	return res;
}

priv AST *parse_expression_stmt(Parser *p)
{
	AST	*res = parse_expression(p, LOWEST);
	if (p->next_token.type == TK_SEMICOLON) next_token(p);
	return res;
}

priv AST *parse_expression(Parser *p, Precedence prec)
{
	AST	*res = NULL;
	PrefixParser pfix_parser = PREFIX_PARSERS(p->cur_token.type);
	if (!pfix_parser) {
		parser_error(p, str_fmt(p->arena, "Parsing error - not a valid prefix: %.*s", 
					fmt(token_str(p->cur_token.type))));
		return NULL;
	}
	res = pfix_parser(p);
	while ((p->next_token.type != TK_SEMICOLON) && 
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
	return ast_alloc(p->arena, (AST) { AST_BOOL, .AST_BOOL = {p->cur_token.type == TK_TRUE}});
}
priv String read_escapes(Parser *p, String raw);
priv AST *parse_string(Parser *p)
{
	String raw = p->cur_token.lit;
	String res = {0};
	if (str_chr(raw, '\\'))
		res = read_escapes(p, raw); // XXX diff signature
	else
		res = str_dup(p->arena, raw);
	return ast_alloc(p->arena, (AST) { AST_STR, .AST_STR = res });
}

priv String read_escapes(Parser *p, String raw)
{
	char *buf = arena_alloc(p->arena, 0);
	u32 len = 0;
	for (int i = 0; i < raw.len; i++) {
		arena_alloc(p->arena, 1);
		if (raw.buf[i] == '\\') {
			i++;
			if (raw.buf[i] == 'n')
				memcpy(buf + len, "\n", 1);
			else
				memcpy(buf + len, raw.buf + i, 1);
		} else {
			memcpy(buf + len, raw.buf + i, 1);
		}
		len++;
	}
	return (String) { buf, len };
}

priv AST *parse_ident(Parser *p)
{
	return ast_alloc(p->arena, (AST) { AST_IDENT, .AST_STR =  p->cur_token.lit });
}

priv AST *parse_null(Parser *p)
{
	return ast_alloc(p->arena, (AST) { AST_NULL });
}

priv ASTList *parse_many(Parser *p, TokenType end_type);
priv AST *parse_list(Parser *p)
{
	ASTList	*lst = parse_many(p, TK_RBRACKET);
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
	while (p->next_token.type == TK_COMMA) {
		next_token(p), next_token(p);
		astpush(lst, parse_expression(p, LOWEST));
	}
	if (!expect_peek(p, end_type)) {
		arena_pop_to(p->arena, previous_offset); // Failed to build list, resetting arena to previous state
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
	if (!expect_peek(p, TK_RPAREN)) return NULL;
	return res;
}

priv ASTList *parse_function_params(Parser *p)
{
	u64 previous_offset = p->arena->used;
	ASTList	*params = astlist(p->arena);
	AST	*tmp = NULL;
	if (p->next_token.type == TK_RPAREN)
		return (next_token(p), params);
	next_token(p);
	if (p->cur_token.type == TK_IDENT)
		tmp = parse_ident(p);
	else
		return (arena_pop_to(p->arena, previous_offset), NULL);
	astpush(params, tmp);
	while (p->next_token.type == TK_COMMA) {
		next_token(p), next_token(p);
		if (p->cur_token.type == TK_IDENT) {
			tmp = parse_ident(p);
			astpush(params, tmp);
		}
		else
			return (arena_pop_to(p->arena, previous_offset), NULL);
	}
	if (!expect_peek(p, TK_RPAREN))
		return (arena_pop_to(p->arena, previous_offset), NULL);
	return params;
}

priv AST *parse_function(Parser *p)
{
	u64	previous_offset = p->arena->used;
	AST	*res = ast_alloc(p->arena, (AST) { AST_FN, .AST_FN ={0}});
	if (!expect_peek(p, TK_LPAREN))
		return (arena_pop_to(p->arena, previous_offset), NULL);
	ASTList	*params = parse_function_params(p);
	if (!params) return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_FN.params = params;
	if (!expect_peek(p, TK_LBRACE))
		return (arena_pop_to(p->arena, previous_offset), NULL);
	ASTList	*body = parse_block_statement(p);
	if (!body) return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_FN.body = body;
	return res;
}

priv AST *parse_while_statement(Parser *p)
{
	u64 previous_offset = p->arena->used;
	AST *res; 
	if (!expect_peek(p, TK_LPAREN)) return NULL;
	next_token(p);
	res = ast_alloc(p->arena, (AST) { AST_WHILE, .AST_WHILE = {0} });
	AST *condition = parse_expression(p, LOWEST);
	if (!condition) {
		parser_error(p, str("Empty condition"));
		return NULL;
	} 
	res->AST_WHILE.condition = condition;
	if (!expect_peek(p, TK_RPAREN)) return (arena_pop_to(p->arena, previous_offset), NULL);
	if (!expect_peek(p, TK_LBRACE)) return (arena_pop_to(p->arena, previous_offset), NULL);

	ASTList *body = parse_block_statement(p);
	if (!body) return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_WHILE.body = body;

	return res;
}

priv AST *parse_if_expression(Parser *p)
{
	u64 previous_offset = p->arena->used;
	AST	*res;
	if (!expect_peek(p, TK_LPAREN)) return NULL;
	next_token(p);
	res = ast_alloc(p->arena, (AST) { AST_COND, .AST_COND = {0} });

	AST	*condition = parse_expression(p, LOWEST);
	if (!condition) 
		return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_COND.condition = condition;

	if (!expect_peek(p, TK_RPAREN))
		return (arena_pop_to(p->arena, previous_offset), NULL);
	if (!expect_peek(p, TK_LBRACE))
		return (arena_pop_to(p->arena, previous_offset), NULL);

	ASTList	*consequence = parse_block_statement(p);
	if (!consequence)
		return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_COND.consequence = consequence;

	if (p->next_token.type == TK_ELSE) {
		next_token(p);
		if (!expect_peek(p, TK_LBRACE))
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

priv AST *parse_assignment_expression(Parser *p, AST *left)
{
	AST *res = ast_alloc(p->arena, (AST) { AST_ASSIGN, .AST_ASSIGN =  {  left,  NULL  }});
	Precedence prec = PRECEDENCE(p->cur_token.type);
	next_token(p);
	res->AST_ASSIGN.right = parse_expression(p, prec);
	return res;
}

priv AST *parse_call_expression(Parser *p, AST *function)
{
	u64	previous_offset = p->arena->used;
	AST	*res = ast_alloc(p->arena, (AST) { AST_CALL, .AST_CALL = { function, NULL } });
	ASTList	*args = parse_many(p, TK_RPAREN);
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
	if (!index || !expect_peek(p, TK_RBRACKET)) 
		return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_INDEX.index = index;
	return res;
}

// Tables
priv PrefixParser PREFIX_PARSERS(TokenType type)
{
	switch (type) {
		case TK_INT: return &parse_int;
		case TK_NIL: return &parse_null;
		case TK_STRING: return &parse_string;
		case TK_IDENT: return &parse_ident;
		case TK_LBRACKET: return &parse_list;
		case TK_FN: return &parse_function;
		case TK_TRUE: case TK_FALSE: return &parse_bool;
		case TK_BANG: case TK_MINUS: return &parse_prefix_expression;
		case TK_LPAREN: return &parse_grouped_expression;
		case TK_IF: return &parse_if_expression;
		default: return NULL;
	}
}

priv InfixParser INFIX_PARSERS(TokenType type)
{
	switch (type) {
		case TK_PLUS: case TK_MINUS: case TK_STAR:
		case TK_SLASH: case TK_MOD: case TK_EQ:
		case TK_NOT_EQ: case TK_LT: case TK_GT:
			return &parse_infix_expression;
		case TK_ASSIGN: return &parse_assignment_expression;
		case TK_LPAREN: return &parse_call_expression;
		case TK_LBRACKET: return &parse_index_expression;
		default: return NULL;
	}
}

priv Precedence PRECEDENCE(TokenType type)
{
	switch (type) {
		case TK_EQ:
			return EQUALS;
		case TK_NOT_EQ:
			return EQUALS;
		case TK_LT:
			return LESSGREATER;
		case TK_GT:
			return LESSGREATER;
		case TK_PLUS:
			return SUM;
		case TK_MINUS:
			return SUM;
		case TK_STAR:
		case TK_SLASH:
		case TK_MOD:
			return PRODUCT;
		case TK_LPAREN:
			return CALL;
		case TK_LBRACKET:
			return INDEX;
		case TK_ASSIGN:
			return ASSIGNMENT;
		default:
			return LOWEST;
	}
}

// ~AST
AST *ast_alloc(Arena *a, AST node)
{
	AST	*ptr = arena_alloc(a, sizeof(AST));
	if (node.type == AST_IDENT) // XXX where is it better to alloc (parse_string now allocs)
		node.AST_STR = str_dup(a, node.AST_STR);
	if (node.type == AST_VAL)
		node.AST_VAL.name = str_dup(a, node.AST_VAL.name);
	if (node.type == AST_VAR)
		node.AST_VAR.name = str_dup(a, node.AST_VAR.name);
	*ptr = node;
	return ptr;
}

ASTList *astlist(Arena *a)
{
	ASTList *l = arena_alloc(a, sizeof(ASTList));
	l->arena = a;
	l->head = NULL;
	l->len = 0;
	return l;
}

void astpush(ASTList *l, AST *ast)
{
	ASTNode	*node = arena_alloc(l->arena, sizeof(ASTNode));
	node->ast = ast;
	node->next = NULL;

	if (NEVER(!l));

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

ASTList *astlist_copy(Arena *a, ASTList *lst)
{
	ASTList *new = astlist(a);
	for (ASTNode *tmp = lst->head; tmp; tmp = tmp->next)
		astpush(new, tmp->ast);
	return new;
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
		parser_error(p, str_fmt(p->arena, 
					"Parsing error - Wrong token after %.*s. Got %.*s, expected %.*s",
					fmt(token_str(p->cur_token.type)), fmt(token_str(p->next_token.type)),
					fmt(token_str(type))));
		return false;
	}
}

priv void parser_error(Parser *p, String msg)
{
	if (!p->errors) p->errors = strlist(p->arena);
	strpush(p->errors, msg);
}

// STDOUT
void parser_print_errors(Parser *p)
{
	str_print(str("\nParser has errors.\n"));
	for (StrNode *tmp = p->errors->head; tmp; tmp = tmp->next)
		str_print(tmp->string), str_print(str("\n"));
}

String astlist_str(Arena *a, ASTList *lst);
String ast_str(Arena *a, AST *node)
{
	switch (node->type) {
		case AST_INT:
			return str_fmt(a, "%ld", node->AST_INT.value);
		case AST_BOOL:
			return node->AST_BOOL.value ? str("true") : str("false");
		case AST_IDENT:
		case AST_STR:
			return node->AST_STR;
		case AST_NULL:
			return str("NULL");
		case AST_VAL:
			return str_fmt(a, "|%.*s=%.*s|", fmt(node->AST_VAL.name), fmt(ast_str(a, node->AST_VAL.value)));
		case AST_VAR:
			return str_fmt(a, "|%.*s=%.*s|", fmt(node->AST_VAR.name), fmt(ast_str(a, node->AST_VAR.value)));
		case AST_RETURN:
			return str_fmt(a, "|return %.*s|", ast_str(a, node->AST_RETURN.value));
		case AST_ASSIGN:
			return str_fmt(a, "|%.*s = %.*s|", fmt(ast_str(a, node->AST_ASSIGN.left)), fmt(ast_str(a, node->AST_ASSIGN.right)));
		case AST_PROGRAM:
		case AST_LIST:
			return astlist_str(a, node->AST_LIST);
		case AST_FN:
			return str_concat(a, astlist_str(a, node->AST_FN.params), astlist_str(a, node->AST_FN.body));
		case AST_PREFIX:
			return str_fmt(a, "(%.*s%.*s)", fmt(node->AST_PREFIX.op), fmt(ast_str(a, node->AST_PREFIX.right)));
		case AST_INFIX:
			return str_fmt(a, "(%.*s%.*s%.*s)", fmt(ast_str(a, node->AST_INFIX.left)), 
					fmt(node->AST_INFIX.op), fmt(ast_str(a, node->AST_INFIX.right)));
		case AST_COND: {
			String alternative = (node->AST_COND.alternative) ? astlist_str(a, node->AST_COND.alternative) : str("");
			return str_fmt(a, "|if%.*s{%.*s}%.*s|",
					fmt(ast_str(a,node->AST_COND.condition)),
					fmt(astlist_str(a, node->AST_COND.consequence)),
					fmt(alternative));
			}
		case AST_WHILE:
			return str_fmt(a, "|while %.*s{%.*s}|", fmt(ast_str(a, node->AST_WHILE.condition)),
					fmt(astlist_str(a, node->AST_WHILE.body)));
		case AST_CALL:
			return str_fmt(a, "(%.*s<%.*s>)", fmt(ast_str(a, node->AST_CALL.function)),
					fmt(astlist_str(a, node->AST_CALL.args)));
		case AST_INDEX:
			return str_fmt(a, "(%.*s[%.*s])", fmt(ast_str(a, node->AST_INDEX.left)), fmt(ast_str(a, node->AST_INDEX.index)));
	}
	return (NEVER(1), str(""));
}

String astlist_str(Arena *a, ASTList *lst)
{
	if (!lst) return str("");
	char *buf = arena_alloc(a, 1);
	buf[0] = '[';
	u32 len = 1;
	for (ASTNode *cursor = lst->head; cursor; cursor = cursor->next) {
		String tmp = ast_str(a, cursor->ast);
		arena_alloc(a, tmp.len);
		memcpy(buf + len, tmp.buf, tmp.len);
		len += tmp.len;
		if (cursor->next) {
			tmp = str(", ");
			arena_alloc(a, tmp.len);
			memcpy(buf + len, tmp.buf, tmp.len);
			len += tmp.len;
		}
	}
	String tmp = str("]");
	arena_alloc(a, tmp.len);
	memcpy(buf + len, tmp.buf, tmp.len);
	len += tmp.len;
	return (String) { buf, len };
}

void ast_aprint(Arena *a, AST *node)
{
	str_print(ast_str(a, node));
}

String	asttype_str(ASTType type)
{
	String typenames[] = {
		str("AST_VAL"), str("AST_VAR"), str("AST_ASSIGN"), str("AST_WHILE"),
		str("AST_RETURN"), str("AST_IDENT"), str("AST_INT"), 
		str("AST_BOOL"), str("AST_BOOL"), str("AST_STR"), 
		str("AST_LIST"), str("AST_FN"), str("AST_PREFIX"), 
		str("AST_INFIX"), str("AST_COND"), str("AST_CALL"), 
		str("AST_INDEX"), str("AST_PROGRAM")
	};
	if (NEVER(type < 0 || type > arrlen(typenames)))
		return str("Unkown ast type");
	return typenames[type];
}
