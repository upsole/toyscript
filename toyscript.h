#ifndef TOYSCRIPT_H
# define TOYSCRIPT_H

// ~LEXER
typedef enum TokenType { 
	END, ILLEGAL, SEMICOLON, COMMA, BANG, STAR, 
	SLASH, MOD, GT, LT, LPAREN, RPAREN, LBRACE, 
	RBRACE, LBRACKET, RBRACKET, PLUS, MINUS, ASSIGN, 
	EQ, NOT_EQ, TK_INT, TK_STRING, TK_IDENT, TK_VAL, 
	TK_VAR, TK_FN, TK_RETURN,  	IF, ELSE, TRUE, FALSE
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

// ~AST
typedef struct AST AST;
typedef struct ASTNode ASTNode;
typedef enum ASTType { AST_VAL, AST_VAR, AST_RETURN, // STATEMENTS
	AST_IDENT, AST_INT, AST_BOOL, AST_STR, AST_LIST, AST_FN, // VALUES
	AST_PREFIX, AST_INFIX, AST_COND, AST_CALL, AST_INDEX // EXPRESSIONS
} ASTType;

struct ASTNode {
	AST		*ast;
	ASTNode *next;
};

typedef struct ASTList {
	Arena	*arena;
	ASTNode	*head;
	ASTNode	*tail;
	int	len;
}	ASTList;

struct AST {
	ASTType type;
	union {
		struct AST_INT { long value; } AST_INT;
		struct AST_BOOL { bool value; } AST_BOOL;
		String AST_STR;
		struct AST_RETURN { AST *value; } *AST_RETURN;
		struct AST_VAL { AST *value; } *AST_VAL;
		struct AST_VAR { AST *value; } *AST_VAR;
		ASTList *AST_LIST;
		struct AST_FN { ASTList *params; ASTList *body; } AST_FN;
		struct AST_PREFIX { String op; AST *right; } AST_PREFIX;
		struct AST_INFIX { AST *left; String op; AST *right; } AST_INFIX;
		struct AST_COND { AST *condition; ASTList *consequence; ASTList *alternative; } AST_COND;
		struct AST_CALL { AST *function; ASTList *args; } AST_CALL;
		struct AST_INDEX { AST *left; AST *right; } AST_INDEX;
	};
};

typedef struct Parser {
	Arena	*arena;
	Lexer	*lexer;
	StrList	*errors;
	Token cur_token;
	Token next_token;
} Parser;

// API
Lexer 	*lexer(Arena *a, String input);
Token 	lexer_token(Lexer *l);
String	token_str(TokenType type);

Parser	*parser(Arena *a, Lexer *l);
AST		*parse_program(Parser *p);
void	parser_print_errors(Parser *p);

#endif 
