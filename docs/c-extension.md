# C拡張の追加方法

このリポジトリでは、X68000 / Human68k 向けの機能を `mrbgems/mruby-x68k-stdio` という mrbgem として追加しています。
Ruby側からは `X68k::Graph.line` や `X68k::Sprite.put` のように呼び出し、実体はCでIOCSやハードウェア寄りの処理を行います。

## 基本構成

```text
mrbgems/mruby-x68k-stdio/
  mrbgem.rake
  src/x68k_stdio.c
```

`mrbgem.rake` はmrubyにこのgemを認識させるための定義です。

```ruby
MRuby::Gem::Specification.new('mruby-x68k-stdio') do |spec|
  spec.license = 'MIT'
  spec.author  = 'X68000 mruby port'
  spec.summary = 'small stdio helpers for X68000 mruby'
end
```

C側の実装は `src/x68k_stdio.c` にまとめています。
現時点では stdio / File / Screen / Graph / Text / Sprite / Key / Joy / Bg をここで定義しています。

## C関数の形

mrubyから呼び出すC関数は、基本的に次の形です。

```c
static mrb_value
x68k_graph_line(mrb_state *mrb, mrb_value self)
{
  mrb_int x1, y1, x2, y2, color;

  mrb_get_args(mrb, "iiiii", &x1, &y1, &x2, &y2, &color);
  /* X68000側の描画処理 */

  return mrb_nil_value();
}
```

引数は `mrb_get_args` で取り出します。
よく使う指定は以下です。

```text
i  Integer
z  NUL終端文字列
S  String
*  可変長引数
|  省略可能引数の開始
```

戻り値は `mrb_value` です。
`nil` を返す場合は `mrb_nil_value()`、整数を返す場合は `mrb_fixnum_value(value)` を使います。

## Rubyへ公開する

初期化関数で、モジュールやクラスへC関数を登録します。

```c
void
mrb_mruby_x68k_stdio_gem_init(mrb_state *mrb)
{
  struct RClass *x68k;
  struct RClass *graph;

  x68k = mrb_define_module(mrb, "X68k");
  graph = mrb_define_module_under(mrb, x68k, "Graph");

  mrb_define_class_method(mrb, graph, "line", x68k_graph_line, MRB_ARGS_REQ(5));
}
```

これでRuby側から次のように呼び出せます。

```ruby
X68k::Graph.line(0, 0, 255, 255, 0xffff)
```

インスタンスを持たないX68000制御系APIは、基本的に `X68k::Graph.line` のようなクラスメソッド/モジュール関数として公開しています。

## 名前空間の方針

X68000固有機能は `X68k` モジュール配下にまとめます。

```text
X68k::Screen
X68k::Graph
X68k::Text
X68k::Sprite
X68k::Bg
X68k::Key
X68k::Joy
```

追加するときも、既存の分類に合わせます。
たとえば画面モードなら `X68k::Screen`、線や矩形描画なら `X68k::Graph`、スプライトなら `X68k::Sprite` に置きます。

## ビルドへの反映

このgemをmruby本体側へ配置し、X68000向けの `build_config.rb` で有効化してからビルドします。

```text
mruby-host/mrbgems/mruby-x68k-stdio
```

ビルド例:

```sh
cd /home/utsu/work/elf2x68k/mruby-host
MRUBY_CONFIG=x68k rake -j2
```

生成される実行ファイル:

```text
build/x68k/bin/mruby.x
```

## 実装時の注意

### 1. Rubyから細かく呼びすぎない

MC68000上では、RubyからC関数を大量に呼ぶだけでも負荷になります。
毎フレーム大量に呼ぶ処理は、できるだけC側でまとめて処理するAPIにした方が有利です。

### 2. 引数チェックをC側で行う

座標や色番号など、範囲外の値が来る可能性があります。
最初は小さく実装しても、安定してきたらC側で範囲チェックを入れる方が安全です。

### 3. バッファ寿命に注意する

`RSTRING_PTR` で得たポインタはRubyオブジェクトの内部バッファです。
C側で長期間保持せず、その場で使い切るようにします。

### 4. エラーはmrubyの例外へ寄せる

ファイルI/Oなどで失敗した場合は、`mrb_sys_fail` や `mrb_raise` を使ってRuby側の例外として返します。

```c
if (fp == NULL) {
  mrb_sys_fail(mrb, path);
}
```

### 5. Human68k / IOCSの制約を隠しすぎない

RubyらしいAPIに寄せるのは有効ですが、X68000固有の画面モード、PCG番号、BGコード、パレット制約は完全には隠せません。
薄いラッパーから始め、必要なところだけRuby向けに整える方が保守しやすいです。

## 追加作業の流れ

1. `src/x68k_stdio.c` にC関数を追加する。
2. `mrb_mruby_x68k_stdio_gem_init` でRuby名へ登録する。
3. `samples/` に短いRubyサンプルを追加する。
4. WSL側で `MRUBY_CONFIG=x68k rake -j2` を実行する。
5. 生成された `mruby.x` をXM6 TypeGの実行フォルダへコピーして確認する。
6. 動作が固まったらREADMEやdocsへAPIの説明を追記する。

## 小さな例

Ruby側:

```ruby
X68k::Graph.line(0, 0, 255, 255, 0xffff)
```

C側では、引数を取り出してIOCSまたは直接アクセスの処理へ渡します。

```c
static mrb_value
x68k_graph_line(mrb_state *mrb, mrb_value self)
{
  mrb_int x1, y1, x2, y2, color;

  mrb_get_args(mrb, "iiiii", &x1, &y1, &x2, &y2, &color);
  /* draw line */

  return mrb_nil_value();
}
```

このように、Ruby側には短いAPIを見せ、重い処理やX68000固有の呼び出しはC側に寄せます。
