#include "9cc.h"

Node *code[100];

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
  LVar *next;  // 次の変数がNULL
  char *name;  // 変数の名前
  int len;     // 名前の長さ
  int offset;  // RBPからのオフセット
};

//ローカル変数
LVar *locals;

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

// ローカル変数を追加して、追加したLVar構造体を返す
LVar *new_lvar(Token *tok) {
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->next = locals;
  if (!locals)
    lvar->offset = 8;
  else
    lvar->offset = locals->offset + 8;
  lvar->name = tok->str;
  lvar->len = tok->len;
  locals = lvar;
  return locals;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待しているkindの時には、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume_kind(TokenKind kind) {
  if (token->kind != kind) return false;
  token = token->next;
  return true;
}

// 次のトークンがTK_IDENTの時には、トークンを1つ読み進めて
// TK_IDENTなトークンを返す。それ以外の場合にはNULLを返す。
Token *consume_ident() {
  if (token->kind == TK_IDENT) {
    Token *ret = token;
    token = token->next;
    return ret;
  }
  return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM) error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

// 新しい2項演算子ノードを作成する
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 新しい数値ノードを作成する
Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

/* パーサの文法
program    = stmt*
stmt       = expr ";" | "return" expr ";"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | ident | "(" expr ")"
*/

Node *expr();

// 中括弧のパーサ
Node *primary() {
  // 次のトークンが "(" なら、"(" expr ")" のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  // 次のトークンがローカル変数
  Token *tok = consume_ident();
  if (tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    LVar *lvar = find_lvar(tok);
    if (!lvar) lvar = new_lvar(tok);
    node->offset = lvar->offset;
    return node;
  }

  // 上のいずれでもないなら数値のはず
  return new_node_num(expect_number());
}

// 単項演算子のパーサ
Node *unary() {
  if (consume("+")) return primary();
  if (consume("-")) return new_node(ND_SUB, new_node_num(0), primary());
  return primary();
}

// 積・商のパーサ
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// 二項演算子+, -のパーサ
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// <, <=, >, >= のパーサ
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<="))
      node = new_node(ND_LE, node, add());
    else if (consume(">="))
      node = new_node(ND_LE, add(), node);
    else if (consume("<"))
      node = new_node(ND_L, node, add());
    else if (consume(">"))
      node = new_node(ND_L, add(), node);
    else
      return node;
  }
}

// ==, != のパーサ
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

// 代入文のパーサ
Node *assign() {
  Node *node = equality();
  if (consume("=")) return new_node(ND_ASSIGN, node, assign());
  return node;
}

// 式のパーサ
Node *expr() { return assign(); }

// 文のパーサ
Node *stmt() {
  Node *node;
  if (consume_kind(TK_RETURN)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
  } else {
    node = expr();
  }
  expect(";");
  return node;
}

// プログラムのパーサ
void program() {
  int i = 0;
  while (!at_eof()) code[i++] = stmt();
  code[i] = NULL;
}