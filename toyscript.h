#ifndef TOYSCRIPT_H
# define TOYSCRIPT_H

// ~LEXER
typedef enum TokenType { 
	END, ILLEGAL, SEMICOLON, COMMA, BANG, STAR, 
	SLASH, MOD, GT, LT, LPAREN, RPAREN, LBRACE, 
	RBRACE, LBRACKET, RBRACKET, PLUS, MINUS, ASSIGN, 
	EQ, NOT_EQ, TK_INT, TK_STRING, TK_IDENT, TK_VAL, 
	TK_VAR, TK_FN, TK_RETURN, IF, ELSE, TK_WHILE,
	TRUE, FALSE, TK_NULL
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
typedef enum ASTType { AST_VAL, AST_VAR, AST_RETURN, AST_ASSIGN, // STATEMENTS
	AST_IDENT, AST_INT, AST_BOOL, AST_STR, AST_LIST, AST_FN, // VALUES
	AST_PREFIX, AST_INFIX, AST_COND, AST_CALL, AST_INDEX, // EXPRESSIONS
	AST_WHILE, AST_NULL, AST_PROGRAM
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
		struct AST_NULL {} AST_NULL;
		struct AST_INT { long value; } AST_INT;
		struct AST_BOOL { bool value; } AST_BOOL;
		String AST_STR;
		struct AST_RETURN { AST *value; } AST_RETURN;
		struct AST_VAL { String name; AST *value; } AST_VAL;
		struct AST_VAR { String name; AST *value; } AST_VAR;
		ASTList *AST_LIST;
		struct AST_FN { ASTList *params; ASTList *body; } AST_FN;
		struct AST_PREFIX { String op; AST *right; } AST_PREFIX;
		struct AST_INFIX { AST *left; String op; AST *right; } AST_INFIX;
		struct AST_COND { AST *condition; ASTList *consequence; ASTList *alternative; } AST_COND;
		struct AST_CALL { AST *function; ASTList *args; } AST_CALL;
		struct AST_INDEX { AST *left; AST *index; } AST_INDEX;
		struct AST_ASSIGN { AST *left; AST *right; } AST_ASSIGN;
		struct AST_WHILE { AST *condition; ASTList *body; } AST_WHILE;
	};
};

typedef struct Parser {
	Arena	*arena;
	Lexer	*lexer;
	StrList	*errors;
	Token cur_token;
	Token next_token;
} Parser;

// ~EVAL
typedef struct Element Element;
typedef struct Namespace Namespace;
typedef struct ElemList	ElemList;
typedef struct ElemArray ElemArray;

typedef enum ElementType { ELE_NULL, ERR, INT, BOOL, STR, LIST, ARRAY, RETURN, FUNCTION, BUILTIN, TYPE } ElementType;
typedef Element (*BuiltinFunction)(Arena *a, Namespace *ns, ElemArray *args);
struct Element {
	ElementType type;
	union {
		struct		ELE_NULL {} ELE_NULL;
		String 		ERR;
		long 		INT;
		bool 		BOOL;
		String		STR;
		ElemList	*LIST;
		ElemArray	*ARRAY;
		struct RETURN { Element *value; } RETURN; 
		struct FUNCTION { ASTList *params; ASTList *body; Namespace *namespace; } FUNCTION;
		BuiltinFunction BUILTIN;
		ElementType	TYPE;
	};
};

// ~ LISTS
typedef struct ElemNode {
	Element	element;
	struct ElemNode *next;
} ElemNode;
struct ElemList {
	Arena	*arena;
	ElemNode *head;
	ElemNode *tail;
	int len;
};

struct ElemArray {
	Element *items;
	int	len;
};
// ~NAMESPACE
typedef struct Bind Bind;
struct Bind {
	String key;
	Element element;
	bool	mutable;
	Bind *next;
};

struct Namespace {
	Arena *arena;
	u64 len;
	u64 cap;
	Bind **values;
	Namespace *parent;
};

// API
Lexer 	*lexer(Arena *a, String input);
Token 	lexer_token(Lexer *l);
String	token_str(TokenType type);

void 	ast_aprint(Arena *a, AST *node);
String	asttype_str(ASTType type);
String 	ast_str(Arena *a, AST *node);

Parser	*parser(Arena *a, Lexer *l);
AST		*parse_program(Parser *p);
void	parser_print_errors(Parser *p);

Element	eval(Arena *a, Namespace *ns, AST *node);
String	to_string(Arena *a, Element e);
String	type_str(ElementType type);

Namespace *ns_create(Arena *a, u64 cap);

#endif 
