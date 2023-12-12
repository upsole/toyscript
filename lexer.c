#include "base.h"
#include "toyscript.h"

priv void lexer_read(Lexer *l);
priv char lexer_peek(Lexer *l);
priv String lexer_read_ident(Lexer *l);
priv String lexer_read_int(Lexer *l);
priv String lexer_read_string(Lexer *l);
priv TokenType keywords(String s);

priv bool is_alpha(char c);
priv bool is_num(char c);
priv bool is_blank(char c);

Lexer *lexer(Arena *a, String input)
{
	Lexer	*l = arena_alloc(a, sizeof(Lexer));
	l->input = input;
	l->pos = 0;
	l->next = 1;
	l->ch = l->input.buf[0];
	return l;
}

priv void lexer_read(Lexer *l)
{
	if (l->next >= l->input.len) 
		l->ch = 0;
	else
		l->ch = l->input.buf[l->next];
	l->pos = l->next;
	l->next += 1;
}

priv char lexer_peek(Lexer *l)
{
	if (l->next >= l->input.len)
		return 0;
	else
		return l->input.buf[l->next];
}

Token lexer_token(Lexer *l)
{
	Token t = {0};
	while (is_blank(l->ch))
		lexer_read(l);
	switch (l->ch) {
		case '#': {
			while (l->ch != '\n')
				lexer_read(l);
			return lexer_token(l);
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
			if (lexer_peek(l) == '=') {
				t.type = EQ;
				t.lit = str("==");
				lexer_read(l);
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
			if (lexer_peek(l) == '=') {
				t.type = NOT_EQ;
				t.lit = str("!=");
				lexer_read(l);
			} else {
				t.type = BANG;
				t.lit = str("!");
			}
			break;
		case '"':
			t.type = TK_STRING;
			t.lit = lexer_read_string(l);
			break;
		case 0:
			return (Token) { END, str("") };
		default:
			if (is_alpha(l->ch)) {
				t.lit = lexer_read_ident(l);
				t.type = keywords(t.lit);
				return t;
			} else if (is_num(l->ch)) {
				t.lit = lexer_read_int(l);
				t.type = TK_INT;
				return t;
			} else {
				t.type = ILLEGAL;
				t.lit = (String){&l->ch, 1};
			}
	}
	lexer_read(l);
	return t;
}

priv String lexer_read_ident(Lexer *l)
{
	u32 begin = l->pos;
	while (is_alpha(l->ch) || is_num(l->ch) || l->ch == '?')
		lexer_read(l);
	return str_slice(l->input, begin, l->pos);
}

priv String lexer_read_int(Lexer *l)
{
	u32 begin = l->pos;
	while (is_num(l->ch))
		lexer_read(l);
	return str_slice(l->input, begin, l->pos);
}

priv String lexer_read_string(Lexer *l)
{
	lexer_read(l); // skip first "
	u32 begin = l->pos;
	while (l->ch != '"')
		lexer_read(l);
	return str_slice(l->input, begin, l->pos);
}

priv TokenType keywords(String s)
{
	if (str_eq(s, str("val"))) return TK_VAL;
	if (str_eq(s, str("var"))) return TK_VAR;
	if (str_eq(s, str("fn"))) return TK_FN;
	if (str_eq(s, str("return"))) return TK_RETURN;
	if (str_eq(s, str("if"))) return IF;
	if (str_eq(s, str("else"))) return ELSE;
	if (str_eq(s, str("while"))) return TK_WHILE;
	if (str_eq(s, str("true"))) return TRUE;
	if (str_eq(s, str("false"))) return FALSE;
	if (str_eq(s, str("NULL"))) return TK_NULL;
	return TK_IDENT;
}

// HELPERS
priv bool is_alpha(char c)
{
	return ((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (c == '_'));
}

priv bool is_num(char c)
{
	return (('0' <= c) && (c <= '9'));
}

priv bool is_blank(char c)
{
	return (c == ' ' || c == '\n' || c == '\t' || c == '\r');
}

String token_str(TokenType type)
{
	String names[] = {
	    str("EOF"),	   str("ILLEGAL"), str("SEMICOLON"), str("COMMA"),
	    str("BANG"),   str("STAR"),	   str("SLASH"),     str("MOD"),
	    str("GT"),	   str("LT"),	   str("LPAREN"),    str("RPAREN"),
	    str("LBRACE"), str("RBRACE"),  str("LBRACKET"),  str("RBRACKET"),
	    str("PLUS"),   str("MINUS"),   str("ASSIGN"),    str("EQ"),
	    str("NOT_EQ"), str("INT"),	   str("STRING"), 	 str("IDENT"),     str("VAL"),    
		str("VAR"),    str("FN"),	   str("RETURN"),    str("IF"),	   
		str("ELSE"),   str("WHILE"),   str("TRUE"),    	 str("FALSE"),	
		str("NULL")
	};
	if (NEVER(type < 0 || type >= arrlen(names)))
		return str("Unknown type");
	return names[type];
}

