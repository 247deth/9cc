#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }

  // エラー出力用に入力の先頭アドレスを保持する
  user_input = argv[1];

  // トークナイズする
  token = tokenize(argv[1]);

  // パースする
  Node *node = expr();

  // コード生成する
  codegen(node);

  return 0;
}
