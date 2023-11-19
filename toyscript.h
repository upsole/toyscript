#ifndef TOYSCRIPT
#define TOYSCRIPT
#include "base.h"
// TOKENS
typedef enum TokenType {
	END,
	ILLEGAL,
	// Operators
	SEMICOLON,
	COMMA,
	BANG,
	STAR,
	SLASH,
	MOD,
	GT,
	LT,
	LPAREN,
	RPAREN,
	LBRACE,
	RBRACE,
	LBRACKET,
	RBRACKET,
	PLUS,
	MINUS,
	ASSIGN,
	EQ,
	NOT_EQ,
	// Identifiers and Keywords
	INT,
	STRING,
	IDENT,
	VAL,
	FN,
	RETURN,
	IF,
	ELSE,
	TRUE,
	FALSE
} TokenType;
typedef struct Token {
	TokenType type;
	String lit;
} Token;
Token tok(TokenType type, String lit);
TokenType keywords(String s);
void tok_print(Token t);
String tok_string(TokenType type);
void toktype_print(TokenType t);

// LEXER
typedef struct Lexer {
	String input;
	u64 pos;
	u64 next;
	char ch;
} Lexer;

Lexer *lex_init(Arena *a, String input);
void lex_read(Lexer *l);
Token lex_tok(Lexer *l);
String lex_read_ident(Lexer *l);
String lex_read_int(Lexer *l);
String lex_read_string(Lexer *l);

// AST TODO cleanup
typedef struct Identifier {
	Token tok;
	String value;
} Identifier;

typedef struct IntLiteral {
	Token tok;
	i64 value;
} IntLiteral;

typedef struct BoolLiteral {
	Token tok;
	bool value;
} BoolLiteral;
typedef struct BlockStatement BlockStatement;
typedef struct IdentList IdentList;
typedef struct FunctionLiteral {
	Token tok;
	IdentList *params;
	BlockStatement *body;
} FunctionLiteral;

typedef struct ExpList ExpList;

typedef struct InfixExp InfixExp;
typedef struct PrefixExp PrefixExp;
typedef struct ConditionalExp ConditionalExp;
typedef struct CallExp CallExp;
typedef enum ExpressionType {
	EXP_ERROR,
	EXP_IDENT,
	EXP_INT,
	EXP_STR,
	EXP_LIST,
	EXP_INDEX,
	EXP_FN,
	EXP_CALL,
	EXP_BOOL,
	EXP_PREFIX,
	EXP_INFIX,
	EXP_COND,
} ExpressionType;
typedef struct Expression Expression;

struct Expression {
	ExpressionType type;
	union {
		Identifier _ident;
		IntLiteral _int;
		BoolLiteral _bool;
		FunctionLiteral _fn;
		struct ListLiteral {
			Token tok;
			ExpList *items;
		} _list;
		struct StringLiteral {
			Token tok;
			String value;
		} _string;
		struct IndexExpression {
			Token tok;
			Expression *left;
			Expression *index;
		} _index;
		struct CallExp {
			Token tok;
			Expression *function;
			ExpList *arguments;
		} _call;
		struct InfixExp {
			Token tok;
			Expression *left;
			String operator;
			Expression *right;
		} _infix;
		struct PrefixExp {
			Token tok;
			String operator;
			Expression *right;
		} _prefix;
		struct ConditionalExp {
			Token tok;
			Expression *condition;
			BlockStatement *consequence;
			BlockStatement *alternative;
		} _conditional;
	};
};

typedef struct ValStatement {
	Token tok;
	Identifier name;
	Expression value;
} ValStatement;

typedef struct ReturnStatement {
	Token tok;
	Expression value;
} ReturnStatement;

typedef struct ExpressionStatement {
	Token tok;
	Expression expression;
} ExpressionStatement;

typedef struct StmtList StmtList;
typedef struct Program Program;
typedef enum StmtType {
	STMT_ERR,
	STMT_VAL,
	STMT_RET,
	STMT_EXP,
	PROGRAM
} StmtType;
typedef struct Statement {
	StmtType type;
	union {
		ValStatement _val;
		ReturnStatement _return;
		Expression _exp;
		struct Program {
			StmtList *statements;
		} _program;
	};
} Statement;
void stmt_print(Statement s);
void val_stmt_print(Statement s);
void ret_stmt_print(Statement s);
void exp_stmt_print(Statement s);
String stmt_string(Arena *a, Statement s);
String stmttype_string(StmtType type);
String exp_string(Arena *a, Expression ex);
String exptype_string(ExpressionType type);

typedef struct StmtNode StmtNode;
struct StmtNode {
	Statement s;
	StmtNode *next;
};

struct StmtList {
	Arena *arena;
	StmtNode *head;
	StmtNode *tail;
	u64 len;
};
StmtList *stmtlist(Arena *a);
void stmt_push(StmtList *l, Statement st);

typedef struct IdentNode IdentNode;
struct IdentNode {
	Identifier id;
	IdentNode *next;
};

typedef struct IdentList {
	Arena *arena;
	IdentNode *head;
	IdentNode *tail;
	u64 len;
} IdentList;
IdentList *identlist(Arena *a);
void ident_push(IdentList *l, Identifier st);
typedef struct ExpNode ExpNode;
struct ExpNode {
	Expression ex;
	ExpNode *next;
};
typedef struct ExpList {
	Arena *arena;
	ExpNode *head;
	ExpNode *tail;
	u64 len;
} ExpList;
ExpList *explist(Arena *a);
void exp_push(ExpList *l, Expression ex);

struct BlockStatement {
	Token tok;
	StmtList *statements;
};

// Parser
typedef struct StrNode StrNode;
struct StrNode {
	String s;
	StrNode *next;
};
typedef struct StrList {
	Arena *arena;
	StrNode *head;
	StrNode *tail;
	u64 len;
} StrList;
StrList *strlist(Arena *a);
void str_push(StrList *l, String string);
void strlist_print(StrList *list);
typedef struct Parser {
	Arena *arena;
	Lexer *l;
	StrList *errors;
	Token cur_token;
	Token peek_token;
} Parser;

typedef enum Precedence {
	LOWEST,
	EQUALS,
	LESSGREATER,
	SUM,
	PRODUCT,
	PREFIX,
	CALL,
	INDEX
} Precedence;

Parser *parser_init(Arena *a, Lexer *l);
void parse_next_tok(Parser *p);
Program parse_program(Parser *p);
Statement parse_program_stmt(Parser *p);
void prog_print(Program p);
Statement parse_statement(Parser *p);
Statement parse_return_stmt(Parser *p);
Statement parse_val_stmt(Parser *p);
Statement parse_expression_stmt(Parser *p);

void parser_add_error(Parser *p, String msg);

Expression parse_expression(Parser *p, Precedence prec);
bool expect_peek(Parser *p, TokenType t);
#endif
