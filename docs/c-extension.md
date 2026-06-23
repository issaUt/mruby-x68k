# C拡張の追加方法

このリポジトリでは、X68000 / Human68k 向けの機能を用途別の mrbgem として追加しています。

- `mruby-x68k-os`: Human68k 上のファイル、ディレクトリ、環境変数、コマンド実行など、シェルスクリプト的に使う機能
- `mruby-x68k-hardware`: 画面、グラフィック、入力、割り込み、Z-MUSIC など、X68000 ハードウェア寄りの機能

Ruby側からは `Dir.entries`、`` `dir` ``、`X68k::Graph.line`、`X68k::Joy.sega6b` のように呼び出し、実体はCでHuman68k、IOCS、ハードウェア寄りの処理を行います。

## 基本構成

```text
mrbgems/mruby-x68k-os/
  mrbgem.rake
  src/x68k_os.c
  src/x68k_os.inc
  mrblib/os_shell.rb
  mrblib/pathname.rb

mrbgems/mruby-x68k-hardware/
  mrbgem.rake
  src/x68k_hardware.c
  src/x68k_*.inc
  mrblib/x68k_*.rb
```

`mrbgem.rake` はmrubyにgemを認識させるための定義です。

```ruby
MRuby::Gem::Specification.new('mruby-x68k-hardware') do |spec|
  spec.license = 'MIT'
  spec.author  = 'X68000 mruby port'
  spec.summary = 'X68000 hardware helpers for mruby'
end
```

C側の入口は、OS系が `src/x68k_os.c`、ハードウェア系が `src/x68k_hardware.c` です。
大きな機能単位は `src/x68k_*.inc` に分け、入口ファイルから include しています。

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
mrb_mruby_x68k_hardware_gem_init(mrb_state *mrb)
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

OS系の追加機能は、Ruby標準に近い名前へ寄せます。

```text
File
Dir
ENV
FileUtils
Pathname
Kernel#system
Kernel#`
```

X68000固有機能は `X68k` モジュール配下にまとめます。

```text
X68k::Screen
X68k::Graph
X68k::Text
X68k::Sprite
X68k::Bg
X68k::Key
X68k::Joy
X68k::Ajoy
X68k::Crtc
X68k::Interrupt
X68k::ZMusic
X68k::Sys
X68k::Iocs
```

追加するときも、既存の分類に合わせます。
たとえば画面モードなら `X68k::Screen`、線や矩形描画なら `X68k::Graph`、スプライトなら `X68k::Sprite` に置きます。

## ビルドへの反映

mruby本体側のX68000向け `build_config` で、必要なgemを有効化してからビルドします。

```ruby
conf.gem File.expand_path('../../mruby-x68k/mrbgems/mruby-x68k-os', __dir__)
conf.gem File.expand_path('../../mruby-x68k/mrbgems/mruby-x68k-hardware', __dir__)
```

OS系だけを使う軽量構成では `mruby-x68k-os` だけを有効にできます。

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

1. OS系なら `mruby-x68k-os/src/x68k_os.inc`、ハードウェア系なら対応する `mruby-x68k-hardware/src/x68k_*.inc` にC関数を追加する。
2. `mrb_mruby_x68k_os_gem_init` または `mrb_mruby_x68k_hardware_gem_init` でRuby名へ登録する。
3. `samples/` に短いRubyサンプルを追加する。
4. WSL側で `MRUBY_CONFIG=x68k rake -j2` を実行する。
5. 生成された `mruby.x` をXM6 TypeGの実行フォルダへコピーして確認する。
6. 動作が固まったらREADMEやdocsへAPIの説明を追記する。
