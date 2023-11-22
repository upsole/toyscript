#include "toyscript.h"
#include <unistd.h>
// Struct to String
String tok_string(TokenType type)
{
	String names[] = {
	    str("EOF"),	   str("ILLEGAL"), str("SEMICOLON"), str("COMMA"),
	    str("BANG"),   str("STAR"),	   str("SLASH"),     str("MOD"),
	    str("GT"),	   str("LT"),	   str("LPAREN"),    str("RPAREN"),
	    str("LBRACE"), str("RBRACE"),  str("LBRACKET"),  str("RBRACKET"),
	    str("PLUS"),   str("MINUS"),   str("ASSIGN"),    str("EQ"),
	    str("NOT_EQ"), str("INT"),	   str("IDENT"),     str("VAL"),
	    str("FN"),	   str("RETURN"),  str("IF"),	     str("ELSE"),
	    str("TRUE"),   str("FALSE")};
	return names[type];
}

String exptype_string(ExpressionType type)
{
	String names[] = {str("ERROR"),	 str("IDENT"), str("INT"),
			  str("STRING"), str("LIST"),  str("INDEX"),
			  str("FN"),	 str("CALL"),  str("BOOL"),
			  str("PREFIX"), str("INFIX"), str("COND")};
	return names[type];
}

String stmttype_string(StmtType type)
{
	String names[] = {str("ERROR"), str("VAL"), str("RET"), str("EXP")};
	return names[type];
}

String stmt_string(Arena *a, Statement s)
{
	switch (s.type) {
		case STMT_VAL:
			return str_concatv(
			    a, 6, s._val.tok.lit, str(" "), s._val.name.value,
			    str("="), exp_string(a, s._val.value), str(";"));
		case STMT_RET:
			return str_concatv(a, 3, s._return.tok.lit, str(" "),
					   exp_string(a, s._return.value));
		case STMT_EXP:
			return exp_string(a, s._exp);
		default:
			return str("Statement N/A");
	}
}

String stmtlist_string(Arena *a, StmtList *list)
{
	if (!list) return (String){0};
	char *buf;
	u64 len = 0;
	for (StmtNode *cursor = list->head; cursor; cursor = cursor->next) {
		String tmp = stmt_string(a, cursor->s);
		if (!len)
			buf = arena_alloc(a, tmp.len);
		else
			arena_alloc(a, tmp.len);
		memcpy(buf + len, tmp.buf, tmp.len);
		len += tmp.len;
	}
	return (String){.buf = buf, .len = len};
}

String explist_string(Arena *a, ExpList *list)
{
	if (!list) return (String){0};
	char *buf;
	u64 len = 0;
	for (ExpNode *cursor = list->head; cursor; cursor = cursor->next) {
		String tmp = exp_string(a, cursor->ex);
		if (!len)
			buf = arena_alloc(a, tmp.len);
		else
			arena_alloc(a, tmp.len);
		memcpy(buf + len, tmp.buf, tmp.len);
		len += tmp.len;
		if (cursor->next) {
			tmp = str(" ");
			arena_alloc(a, tmp.len);
			memcpy(buf + len, tmp.buf, tmp.len);
			len += tmp.len;
		}
	}
	return (String){.buf = buf, .len = len};
}

String identlist_string(Arena *a, IdentList *list)
{
	if (!list) return (String){0};
	char *buf;
	u64 len = 0;
	for (IdentNode *cursor = list->head; cursor; cursor = cursor->next) {
		String tmp = cursor->id.value;
		if (!len)
			buf = arena_alloc(a, tmp.len);
		else
			arena_alloc(a, tmp.len);
		memcpy(buf + len, tmp.buf, tmp.len);
		len += tmp.len;
		if (cursor->next) {
			tmp = str(" ");
			arena_alloc(a, tmp.len);
			memcpy(buf + len, tmp.buf, tmp.len);
			len += tmp.len;
		}
	}
	return (String){.buf = buf, .len = len};
}

String exp_string(Arena *a, Expression ex)
{
	switch (ex.type) {
		case EXP_INT:
			return ex._int.tok.lit;
		case EXP_IDENT:
			return ex._ident.value;
		case EXP_BOOL:
			return ex._bool.tok.lit;
		case EXP_LIST:
			return str_concatv(a, 5, str("("), str("["),
					   explist_string(a, ex._list.items),
					   str("]"), str(")"));
		case EXP_INDEX:
			return str_concatv(
			    a, 6, str("("), exp_string(a, *ex._index.left),
			    str("["), exp_string(a, *ex._index.index), str("]"),
			    str(")"));
		case EXP_PREFIX:
			return str_concatv(a, 4, str("("), ex._prefix.operator,
					   exp_string(a, *ex._prefix.right),
					   str(")"));
		case EXP_INFIX:
			return str_concatv(
			    a, 5, str("("), exp_string(a, *ex._infix.left),
			    ex._infix.operator, exp_string(a, *ex._infix.right),
			    str(")"));
		case EXP_FN:
			return str_concatv(
			    a, 7, str("("), ex._fn.tok.lit, str("["),
			    identlist_string(a, ex._fn.params), str("]->"),
			    stmtlist_string(a, ex._fn.body->statements),
			    str(")"));
		case EXP_CALL:
			return str_concatv(
			    a, 6, str("("), exp_string(a, *ex._call.function),
			    str("|"), explist_string(a, ex._call.arguments),
			    str("|"), str(")"));
		case EXP_COND:
			if (!ex._conditional.alternative)
				return str_concatv(
				    a, 4, str("("),
				    exp_string(a, *ex._conditional.condition),
				    stmtlist_string(a,
						    ex._conditional.consequence
							->statements),
				    str(")"));
			else
				return str_concatv(
				    a, 6, str("("),
				    exp_string(a, *ex._conditional.condition),
				    stmtlist_string(a,
						    ex._conditional.consequence
							->statements),
				    str(" else"),
				    stmtlist_string(a,
						    ex._conditional.alternative
							->statements),
				    str(")"));
		default:
			return str("N/A");
	}
}

// PRINT NO BUFFER
void prog_print(Program p)
{
	StmtNode *cursor = p.statements->head;

	while (cursor) {
		write(1, ">", 1);
		stmt_print(cursor->s);
		write(1, "\n", 1);
		cursor = cursor->next;
	}
}

void stmt_print(Statement s)
{
	switch (s.type) {
		case PROGRAM:
			prog_print(s._program);
			break;
		case STMT_VAL:
			val_stmt_print(s);
			break;
		case STMT_RET:
			ret_stmt_print(s);
			break;
		case STMT_EXP:
			exp_stmt_print(s);
			break;
		default:
			str_print(str("N/A Statement "));
			str_print(stmttype_string(s.type));
	}
}

void stmtlist_print(StmtList *list);
void args_print(IdentList *list);
void exp_print(Expression ex)
{
	switch (ex.type) {
		case EXP_IDENT:
			str_print(ex._ident.value);
			break;
		case EXP_INT:
			str_print(ex._int.tok.lit);
			break;
		case EXP_BOOL:
			str_print(ex._bool.tok.lit);
			break;
		case EXP_PREFIX:
			write(1, "(", 1);
			str_print(ex._prefix.operator);
			exp_print(*ex._prefix.right);
			write(1, ")", 1);
			break;
		case EXP_INFIX:
			write(1, "(", 1);
			exp_print(*ex._infix.left);
			str_print(ex._infix.operator);
			exp_print(*ex._infix.right);
			write(1, ")", 1);
			break;
		case EXP_FN:
			write(1, "(", 1);
			str_print(ex._fn.tok.lit);
			args_print(ex._fn.params);
			write(1, "->", 2);
			stmtlist_print(ex._fn.body->statements);
			write(1, ")", 1);
			break;
		case EXP_CALL:
			write(1, "(", 1);
			exp_print(*ex._call.function);
			write(1, "|", 1);
			for (ExpNode *cursor = ex._call.arguments->head; cursor;
			     cursor = cursor->next) {
				exp_print(cursor->ex);
				if (cursor->next) write(1, " ", 1);
			}
			write(1, "|", 1);
			write(1, ")", 1);
			break;
		case EXP_COND:
			write(1, "(", 1);
			str_print(ex._conditional.tok.lit);
			exp_print(*ex._conditional.condition);
			stmtlist_print(ex._conditional.consequence->statements);
			if (ex._conditional.alternative) {
				write(1, " else", 5);
				stmtlist_print(
				    ex._conditional.alternative->statements);
			}
			write(1, ")", 1);
			break;
		default:
			str_print(str("N/A Expression "));
			str_print(exptype_string(ex.type));
	}
}

void stmtlist_print(StmtList *list)
{
	if (!list) return;
	for (StmtNode *cursor = list->head; cursor; cursor = cursor->next) {
		stmt_print(cursor->s);
	}
}

void args_print(IdentList *list)
{
	write(1, "[", 1);
	for (IdentNode *cursor = list->head; cursor; cursor = cursor->next) {
		str_print(cursor->id.value);
		if (cursor->next) write(1, " ", 1);
	}
	write(1, "]", 1);
}

void val_stmt_print(Statement s)
{
	str_print(s._val.tok.lit);
	write(1, " ", 1);
	str_print(s._val.name.value);
	write(1, " = ", 3);
	exp_print(s._val.value);
	write(1, ";", 1);
}

void ret_stmt_print(Statement s)
{
	str_print(s._return.tok.lit);
	write(1, " ", 1);
	exp_print(s._return.value);
	write(1, ";", 1);
}

void exp_stmt_print(Statement s)
{
	exp_print(s._exp);
}
