#ifndef TOYSCRIPT_H
# define TOYSCRIPT_H

// ~LEXER
//
typedef enum TokenType { 
	END, ILLEGAL, SEMICOLON, COMMA, BANG, STAR, 
	SLASH, MOD, GT, LT, LPAREN, RPAREN, LBRACE, 
	RBRACE, LBRACKET, RBRACKET, PLUS, MINUS, ASSIGN, 
	EQ, NOT_EQ, INT, STRING, IDENT, VAL, VAR, FN, RETURN, 
	IF, ELSE, TRUE, FALSE
} TokenType;

typedef struct Token {
	TokenType type;
	String lit;
} Token;

typedef struct Lexer {
	String	input;
	u64		pos;
	u64		next;
	char	ch;
}	Lexer;

// API
Lexer *lexer(Arena *a, String input);
Token lexer_token(Lexer *l);
String token_str(TokenType type);
#endif 
