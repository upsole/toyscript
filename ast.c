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

	while (p->cur_token.type != END) {
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
	if (p->cur_token.type == SEMICOLON) {
		right = ast_alloc(p->arena, (AST) { AST_NULL });
	} else
		right = parse_expression(p, LOWEST);
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

priv AST *parse_null(Parser *p)
{
	return ast_alloc(p->arena, (AST) { AST_NULL });
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
	astpush(params, tmp);
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

priv AST *parse_while_statement(Parser *p)
{
	u64 previous_offset = p->arena->used;
	AST *res; 
	if (!expect_peek(p, LPAREN)) return NULL;
	next_token(p);
	res = ast_alloc(p->arena, (AST) { AST_WHILE, .AST_WHILE = {0} });
	AST *condition = parse_expression(p, LOWEST);
	if (!condition) {
		parser_error(p, str("Empty condition"));
		return NULL;
	} 
	res->AST_WHILE.condition = condition;
	if (!expect_peek(p, RPAREN)) return (arena_pop_to(p->arena, previous_offset), NULL);
	if (!expect_peek(p, LBRACE)) return (arena_pop_to(p->arena, previous_offset), NULL);

	ASTList *body = parse_block_statement(p);
	if (!body) return (arena_pop_to(p->arena, previous_offset), NULL);
	res->AST_WHILE.body = body;
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
		case TK_NULL:
			return &parse_null;
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
		case ASSIGN:
			return &parse_assignment_expression;
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
		case ASSIGN:
			return ASSIGNMENT;
		default:
			return LOWEST;
	}
}

// ~AST
AST *ast_alloc(Arena *a, AST node)
{
	AST	*ptr = arena_alloc(a, sizeof(AST));
	if (node.type == AST_STR || node.type == AST_IDENT)
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

// STDOUT
void parser_print_errors(Parser *p)
{
	str_print(str("Parser has errors.\n"));
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
			return CONCAT(a, 
					str("|"), node->AST_VAL.name, 
					str("="), 
					ast_str(a, node->AST_VAL.value), str("|")
					);
		case AST_VAR:
			return CONCAT(a, 
					str("|"), node->AST_VAR.name, 
					str("="), 
					ast_str(a, node->AST_VAR.value), str("|")
					);
		case AST_RETURN:
			return CONCAT(a, str("|return "), 
					ast_str(a, node->AST_RETURN.value),
					str("|"));
		case AST_ASSIGN:
			return CONCAT(a, str("|"), ast_str(a, node->AST_ASSIGN.left),
					str(" = "), ast_str(a, node->AST_ASSIGN.right), str("|"));
		case AST_PROGRAM:
		case AST_LIST:
			return astlist_str(a, node->AST_LIST);
		case AST_FN:
			return str_concat(a, 
					astlist_str(a, node->AST_FN.params), 
					astlist_str(a, node->AST_FN.body));
		case AST_PREFIX:
			return CONCAT(a, str("("), 
					node->AST_PREFIX.op, 
					ast_str(a, node->AST_PREFIX.right), str(")"));
		case AST_INFIX:
			return CONCAT(a, str("("),
					ast_str(a, node->AST_INFIX.left), 
					node->AST_INFIX.op, 
					ast_str(a, node->AST_INFIX.right), str(")"));
		case AST_COND:
			return CONCAT(a, str("|if"),
					ast_str(a, node->AST_COND.condition), str("{"),
					astlist_str(a, node->AST_COND.consequence), str("}"),
					(node->AST_COND.alternative) ? (CONCAT(a, str(" else{"), astlist_str(a, node->AST_COND.alternative), str("}"))) : str(""),
					str("|")
					);
		case AST_WHILE:
			return CONCAT(a, str("|while"),
					ast_str(a, node->AST_WHILE.condition), str("{"),
					astlist_str(a, node->AST_WHILE.body), str("}"),
					str("|")
					);
		case AST_CALL:
			return CONCAT(a, str("("), 
					ast_str(a, node->AST_CALL.function),
					str("<"),
					astlist_str(a, node->AST_CALL.args),
					str(">"), str(")")
					);
		case AST_INDEX:
			return CONCAT(a, str("("), 
					ast_str(a, node->AST_INDEX.left), str("["),
					ast_str(a, node->AST_INDEX.index), str("])"));
	}
	return (NEVER(1), str(""));
}

String astlist_str(Arena *a, ASTList *lst)
{
	if (!lst) return str("");
	char *buf = arena_alloc(a, 1);
	buf[0] = '[';
	u64 len = 1;
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
	return (String){ buf, len };
}

void ast_aprint(Arena *a, AST *node)
{
	str_print(ast_str(a, node));
}

String	asttype_str(ASTType type)
{
	String typenames[] = {
		str("AST_VAL"), str("AST_VAR"), str("AST_ASSIGN"),
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
