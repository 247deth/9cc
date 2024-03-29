#include "9cc.h"

// ラベルのシーケンシャルID
unsigned int id;

// 左辺値（ローカル変数）のアドレスを計算してスタックにpush
void gen_lval(Node *node) {
  if (node->kind != ND_LVAR) error("代入の左辺値が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

// コード生成
void gen(Node *node) {
  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_LVAR:
      gen_lval(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
    case ND_ASSIGN:
      gen_lval(node->edge[0]);
      gen(node->edge[1]);

      printf("  pop rdi\n");
      printf("  pop rax\n");
      printf("  mov [rax], rdi\n");
      printf("  push rdi\n");
      return;
    case ND_RETURN:
      gen(node->edge[0]);
      printf("  pop rax\n");
      printf("  mov rsp, rbp\n");
      printf("  pop rbp\n");
      printf("  ret\n");
      return;
    case ND_IF: {
      unsigned int my_id = id++;
      gen(node->edge[0]);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%u\n", my_id);
      gen(node->edge[1]);
      printf(".Lend%u:\n", my_id);
      return;
    }
    case ND_IFELSE: {
      unsigned int my_id = id++;
      gen(node->edge[0]);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lelse%u\n", my_id);
      gen(node->edge[1]);
      printf("  jmp .Lend%u\n", my_id);
      printf(".Lelse%u:\n", my_id);
      gen(node->edge[2]);
      printf(".Lend%u:\n", my_id);
      return;
    }
    case ND_WHILE: {
      unsigned int my_id = id++;
      printf(".Lbegin%u:\n", my_id);
      gen(node->edge[0]);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%u\n", my_id);
      gen(node->edge[1]);
      printf("  jmp .Lbegin%u\n", my_id);
      printf(".Lend%u:\n", my_id);
      return;
    }
    case ND_FOR: {
      unsigned int my_id = id++;
      if (node->edge[0]) gen(node->edge[0]);
      printf(".Lbegin%u:\n", my_id);
      if (node->edge[1]) {
        gen(node->edge[1]);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je  .Lend%u\n", my_id);
      }
      gen(node->edge[3]);
      if (node->edge[2]) gen(node->edge[2]);
      printf("  jmp .Lbegin%u\n", my_id);
      if (node->edge[1]) printf(".Lend%u:\n", my_id);
      return;
    }
  }

  gen(node->edge[0]);
  gen(node->edge[1]);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_EQ:  // ==
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NE:  // !=
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:  // <=
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_L:  // <
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
  }

  printf("  push rax\n");
}

// コード生成部
void codegen() {
  unsigned int stacksize = 0;
  if (locals) stacksize = locals->offset;
  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // プロローグ
  // 変数の領域を確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %u\n", stacksize);

  // 先頭の式から順にコード生成
  for (int i = 0; code[i]; i++) {
    gen(code[i]);

    // 式の評価結果としてスタックに一つの値が残っている
    // はずなので、スタックが溢れないようにポップしておく
    printf("  pop rax\n");
  }

  // エピローグ
  // 最後の式の結果がRAXに残っているのでそれが返り値になる
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}
