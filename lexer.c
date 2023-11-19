#include "toyscript.h"

Token tok(TokenType type, String lit)
{
	return (Token){.type = type, .lit = lit};
}

TokenType keywords(String s)
{
	if (str_eq(s, str("val"))) return VAL;
	if (str_eq(s, str("fn"))) return FN;
	if (str_eq(s, str("return"))) return RETURN;
	if (str_eq(s, str("if"))) return IF;
	if (str_eq(s, str("else"))) return ELSE;
	if (str_eq(s, str("true"))) return TRUE;
	if (str_eq(s, str("false"))) return FALSE;
	return IDENT;
}

void toktype_print(TokenType t)
{
	if (t == END)
		printf("EOF");
	else if (t == ILLEGAL)
		printf("ILLEGAL");
	else if (t == SEMICOLON)
		printf("SEMICOLON");
	else if (t == COMMA)
		printf("COMMA");
	else if (t == BANG)
		printf("BANG");
	else if (t == STAR)
		printf("STAR");
	else if (t == SLASH)
		printf("SLASH");
	else if (t == MOD)
		printf("MOD");
	else if (t == GT)
		printf("GREATER_THAN");
	else if (t == LT)
		printf("LESS_THAN");
	else if (t == LPAREN)
		printf("LPAREN");
	else if (t == RPAREN)
		printf("RPAREN");
	else if (t == LBRACE)
		printf("LBRACE");
	else if (t == RBRACE)
		printf("RBRACE");
	else if (t == PLUS)
		printf("PLUS");
	else if (t == MINUS)
		printf("MINUS");
	else if (t == ASSIGN)
		printf("ASSIGN");
	else if (t == EQ)
		printf("EQ");
	else if (t == NOT_EQ)
		printf("NOT_EQ");
	else if (t == INT)
		printf("INT");
	else if (t == IDENT)
		printf("IDENT");
	else if (t == VAL)
		printf("VAL");
	else if (t == FN)
		printf("FUNCTION");
	else if (t == RETURN)
		printf("RETURN");
	else if (t == IF)
		printf("IF");
	else if (t == ELSE)
		printf("ELSE");
	else if (t == TRUE)
		printf("TRUE");
	else if (t == FALSE)
		printf("FALSE");
	else
		printf("N/A");
}

void tok_print(Token t)
{
	printf("|Type: ");
	toktype_print(t.type);
	printf(", Literal: ");
	str_print(t.lit);
	printf(" |\n");
}

Lexer *lex_init(Arena *a, String input)
{
	Lexer *l = arena_alloc(a, sizeof(Lexer));
	l->input = input;
	l->pos = 0;
	l->next = 1;
	l->ch = l->input.buf[0];
	return (l);
}

void lex_read(Lexer *l)
{
	if (l->next >= l->input.len)
		l->ch = 0;
	else
		l->ch = l->input.buf[l->next];
	l->pos = l->next;
	l->next += 1;
}

char lex_peek(Lexer *l)
{
	if (l->next >= l->input.len)
		return 0;
	else
		return l->input.buf[l->next];
}
void lex_skip_comment(Lexer *l)
{
	while (l->ch != '\n')
		lex_read(l);
}
Token lex_tok(Lexer *l)
{
	Token t = {0};
	while (is_blank(l->ch))
		lex_read(l);
	switch (l->ch) {
		case '#': {
			while (l->ch != '\n')
				lex_read(l);
			return lex_tok(l);
		}
		case '+':
			t.type = PLUS;
			t.lit = str("+");
			break;
		case '-':
			t.type = MINUS;
			t.lit = str("-");
			break;
		case ';':
			t.type = SEMICOLON;
			t.lit = str(";");
			break;
		case ',':
			t.type = COMMA;
			t.lit = str(",");
			break;
		case '(':
			t.type = LPAREN;
			t.lit = str("(");
			break;
		case ')':
			t.type = RPAREN;
			t.lit = str(")");
			break;
		case '{':
			t.type = LBRACE;
			t.lit = str("{");
			break;
		case '}':
			t.type = RBRACE;
			t.lit = str("}");
			break;
		case '[':
			t.type = LBRACKET;
			t.lit = str("[");
			break;
		case ']':
			t.type = RBRACKET;
			t.lit = str("]");
			break;
		case '=':
			if (lex_peek(l) == '=') {
				t.type = EQ;
				t.lit = str("==");
				lex_read(l);
			} else {
				t.type = ASSIGN;
				t.lit = str("=");
			}
			break;
		case '<':
			t.type = LT;
			t.lit = str("<");
			break;
		case '>':
			t.type = GT;
			t.lit = str(">");
			break;
		case '*':
			t.type = STAR;
			t.lit = str("*");
			break;
		case '/':
			t.type = SLASH;
			t.lit = str("/");
			break;
		case '%':
			t.type = MOD;
			t.lit = str("%");
			break;
		case '!':
			if (lex_peek(l) == '=') {
				t.type = NOT_EQ;
				t.lit = str("!=");
				lex_read(l);
			} else {
				t.type = BANG;
				t.lit = str("!");
			}
			break;
		case '"':
			t.type = STRING;
			t.lit = lex_read_string(l);
			break;
		case 0:
			return tok(END, str(""));
		default:
			if (is_alpha(l->ch)) {
				t.lit = lex_read_ident(l);
				t.type = keywords(t.lit);
				return t;
			} else if (is_num(l->ch)) {
				t.lit = lex_read_int(l);
				t.type = INT;
				return t;
			} else {
				t.type = ILLEGAL;
				t.lit = (String){&l->ch, 1};
			}
	}
	lex_read(l);
	return t;
}

String lex_read_ident(Lexer *l)
{
	u64 begin = l->pos;
	while (is_alpha(l->ch) || is_num(l->ch) || l->ch == '?')
		lex_read(l);
	return str_slice(l->input, begin, l->pos);
}

String lex_read_int(Lexer *l)
{
	u64 begin = l->pos;
	while (is_num(l->ch))
		lex_read(l);
	return str_slice(l->input, begin, l->pos);
}

String lex_read_string(Lexer *l)
{
	lex_read(l); // skip first "
	u64 begin = l->pos;
	while (l->ch != '"')
		lex_read(l);
	return str_slice(l->input, begin, l->pos);
}
