#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum {
  TK_RESERVED,  // 記号
  TK_IDENT,     // 識別子
  TK_RETURN,    // return
  TK_IF,        // if
  TK_ELSE,      // else
  TK_WHILE,     // while
  TK_NUM,       // 整数トークン
  TK_EOF,       // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind;  // トークンの型
  Token *next;     // 次の入力トークン
  int val;         // kindがTK_NUMの場合、その数値
  char *str;       // トークン文字列
  int len;         // トークンの長さ
};

// 抽象構文木のノードの種類
typedef enum {
  ND_EQ,      // ==
  ND_NE,      // !=
  ND_LE,      // <=
  ND_L,       // <
  ND_ADD,     // +
  ND_SUB,     // -
  ND_MUL,     // *
  ND_DIV,     // /
  ND_ASSIGN,  // =
  ND_LVAR,    // ローカル変数
  ND_RETURN,  // return
  ND_IF,      // elseのないif
  ND_IFELSE,  // if (...) ...; else ...;
  ND_WHILE,   // while
  ND_NUM,     // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
  NodeKind kind;  // ノードの型
  Node *lhs;      // 左辺
  Node *mhs;      // 中辺（と言っていいのか？）
  Node *rhs;      // 右辺
  int val;        // kindがND_NUMの場合のみ使う
  int offset;     // kindがND_LVARの場合のみ使う
};

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
  LVar *next;  // 次の変数がNULL
  char *name;  // 変数の名前
  int len;     // 名前の長さ
  int offset;  // RBPからのオフセット
};

// 現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

// パースした文
extern Node *code[];

// ローカル変数
extern LVar *locals;

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
bool at_eof();
Token *tokenize(char *p);
void program();
void codegen();