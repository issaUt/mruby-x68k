# OS / Shell API メモ

このメモは、mruby-x68k でバッククォート対応から拡張している OS 周りの API をまとめたものです。

X68000 / Human68k 上で Ruby をシェルスクリプト的に使えるようにすることを目的にしています。
外部コマンドの実行結果を Ruby 側で加工したり、Ruby 側でディレクトリをたどってファイルを処理したりするための機能です。

## 方針

mruby-x68k の OS API は、現時点では CRuby と完全互換にすることよりも、Human68k 上で実用的に使える最小機能を優先しています。

たとえば、以下のような使い方を想定しています。

- 外部コマンドの標準出力を文字列として受け取り、Ruby で整形する。
- `system` で外部コマンドを実行し、終了ステータスを `$?` で確認する。
- `Dir.entries` / `Dir.glob` でディレクトリを走査し、Ruby 側で簡単な grep や集計を行う。
- `File.read` / `File.write` で設定ファイルやログを扱う。
- `FileUtils` 相当の最小 API で、コピー、移動、削除、ディレクトリ作成を行う。

## コマンドラインからのワンライナー実行

`mruby.x` は `-e` オプションで、コマンドラインから直接 Ruby コードを実行できます。
小さな確認や、OS API の動作確認に便利です。

```text
mruby -e "p ENV.keys"
mruby -e "p ENV['path']"
mruby -e "puts Dir.pwd"
mruby -e "p Dir.entries('.')"
mruby -e "puts File.readlines('README.md')[0]"
```

Human68k 上では、上記のように外側をダブルクォート、Ruby 文字列をシングルクォートにする形で確認できています。
シングルクォート自体も Ruby 側の文字列リテラルとして利用できます。

複数行の処理や長い処理は `.rb` ファイルにしたほうが扱いやすいですが、`ENV` / `Dir` / `File` などの簡単な確認には `-e` が向いています。


## load / require

`load` / `require` で別ファイルを読み込めます。
この実装は [mattn/mruby-require](https://github.com/mattn/mruby-require) を参考に、Human68k 向けに `.rb` / `.mrb` 読み込みへ絞って移植しています。

```ruby
load "lib/foo.rb"
require "lib/foo"
require "./lib/foo.rb"
```

`require` は読み込み済みのファイルを `$"` / `$LOADED_FEATURES` に記録し、同じファイルは2回実行しません。
探索パスは `$:` / `$LOAD_PATH` です。初期値は `.` で、環境変数 `MRBLIB` または `RUBYLIB` があれば `;` または `,` 区切りで追加します。

`mruby.x` のようにコンパイラを含むビルドでは `.rb` と `.mrb` を読み込めます。
`mrb.x` / `mrb-os.x` のようなVM専用ビルドでは `.mrb` のみ読み込めます。Rubyソース `.rb` の読み込みには対応しません。

## バッククォート

バッククォートで外部コマンドを実行し、その標準出力を文字列として取得できます。

```ruby
out = `dir`
puts out
```

バッククォートは暫定導入です。
内部的にはコマンドの標準出力を一時ファイルへリダイレクトし、その内容を Ruby 文字列として読み戻しています。
一時ファイルは `TMP`、次に `TEMP` 環境変数を見て、定義されていればそのディレクトリに作成します。
`TMP` / `TEMP` の検索は Human68k の環境変数と同様に大文字小文字を区別しません。`tmp` や `temp` として定義されている場合も利用します。
どちらも未定義の場合は、従来通りカレントディレクトリへ作成します。

バッククォート実行後、`$?` にはコマンドの終了ステータスが整数で入ります。
CRuby の `Process::Status` オブジェクトではありません。

```ruby
out = `grep main *.c`
puts $?
```

### 一時ファイルの生成と削除

バッククォートは、1回の実行ごとに `MRBBQ000.TMP` のような一時ファイル名を作ります。
番号は内部カウンタで進み、`MRBBQ999.TMP` の次は `MRBBQ000.TMP` に戻ります。

通常の流れは以下です。

```text
1. 一時ファイル名を決定する
2. 同名ファイルがあれば事前に削除する
3. command > temporary-file の形で system 実行する
4. 一時ファイルを読み込んで Ruby 文字列にする
5. 読み込み後に一時ファイルを削除する
```

同一スクリプト内で複数回バッククォートを実行した場合は、通常それぞれ別の一時ファイル名を使い、各実行の読み込み後に削除します。

ただし、コマンド実行中や一時ファイル読み込み前に mruby.x 自体が異常終了した場合は、削除処理まで到達できないため、一時ファイルが残ることがあります。
Rubyスクリプト側で例外が発生しても、バッククォート処理が正常に戻った後であれば、その一時ファイルはすでに削除済みです。

## system と終了ステータス

`system(command)` で外部コマンドを実行できます。

```ruby
ok = system("dir")
puts ok
puts $?
```

戻り値は、終了ステータスが `0` の場合 `true`、それ以外は `false` です。
`$?` には整数の終了ステータスが入ります。

## String の行分割

バッククォートや `File.read` で取得した文字列を扱いやすくするため、`String#lines` と `String#each_line` を追加しています。
CRLF / LF / CR を行区切りとして扱い、返す各行には改行文字を含めます。

```ruby
out = `grep X68k samples/*.rb`
out.each_line do |line|
  line = line.chomp
  next if line.length == 0
  puts line
end

lines = out.lines
```

`each_line` はブロックを渡した場合は各行を `yield` して `self` を返します。
ブロックを渡さない場合は、X68000 上で Enumerator 依存を増やさないため、`lines` と同じ配列を返します。

## File

現在の主な `File` クラスメソッドは以下です。

```ruby
File.exist?(path)
File.exists?(path)
File.file?(path)
File.directory?(path)
File.read(path)
File.readlines(path)
File.write(path, data)
File.size(path)
File.open(path, mode = "r")
File.delete(path)
File.unlink(path)
File.rename(old_path, new_path)
File.copy(src_path, dst_path)
```

`File.exist?` は、ファイルでもディレクトリでも、存在すれば `true` を返します。
通常ファイルかどうかを判定したい場合は `File.file?`、ディレクトリかどうかを判定したい場合は `File.directory?` を使います。

`File.basename` / `File.dirname` / `File.extname` は、Human68k の標準的な `\\` に加えて、Rubyコードで扱いやすい `/` もパス区切りとして扱います。
ドライブ指定を考慮するため、`:` も区切りとして扱います。

`File.expand_path` / `File.absolute_path` は、相対パスを `Dir.pwd` からの絶対パスへ展開し、`.` と `..` を畳みます。
`A:\\...` や `A:/...` はドライブ付き絶対パスとして扱い、`\\...` や `/...` はルートからのパスとして扱います。

```ruby
if File.exist?("README.md")
  puts File.size("README.md")
end

if File.directory?("samples")
  puts "samples directory exists"
end
```

`File.delete` / `File.unlink` は通常ファイルの削除用です。
ディレクトリ削除には `Dir.delete` または `FileUtils.rmdir` を使います。

## Dir

現在の主な `Dir` クラスメソッドは以下です。

```ruby
Dir.entries(path = ".")
Dir.glob(pattern)
Dir.pwd
Dir.getwd
Dir.chdir(path)
Dir.mkdir(path)
Dir.exist?(path)
Dir.exists?(path)
Dir.delete(path)
Dir.rmdir(path)
Dir.rename(old_path, new_path)
```

`Dir.entries` は指定ディレクトリ内の名前を配列で返します。
`Dir.glob` は `*` と `?` のワイルドカード展開に対応しています。`**` を使うとサブディレクトリを再帰的に検索できます。
戻り値には `.` や `..` が含まれる環境があるため、再帰処理では明示的に除外してください。

```ruby
Dir.entries(".").each do |name|
  next if name == "." || name == ".."
  puts name
end
```

`Dir.chdir` は現在、ブロック形式には対応していません。
必要な場合は、呼び出し前の `Dir.pwd` を保存して戻してください。

```ruby
old = Dir.pwd
Dir.chdir("samples")
# ...
Dir.chdir(old)
```

## FileUtils

CRuby の `FileUtils` のうち、最小限の操作だけを用意しています。

```ruby
FileUtils.cp(src, dst)
FileUtils.copy(src, dst)
FileUtils.mv(src, dst)
FileUtils.move(src, dst)
FileUtils.mkdir(path)
FileUtils.rm(path)
FileUtils.remove(path)
FileUtils.rmdir(path)
```

現時点では `mkdir_p` や `rm_rf` のような再帰的・破壊的な操作は実装していません。
必要になった場合も、Human68k 上での安全性を確認しながら追加する方針です。

## ENV

環境変数は `ENV` で扱えます。

```ruby
puts ENV["PATH"]
ENV["FOO"] = "BAR"
puts ENV["FOO"]
ENV.delete("FOO")
```

現在は以下の操作に対応しています。

```ruby
ENV[name]
ENV[name] = value
ENV.delete(name)
ENV.keys
```

Human68k では、環境変数の実表記は保持されますが、検索時は大文字小文字を区別しません。
mruby-x68k でも `ENV.keys` は実際に定義されている表記を返し、`ENV["PATH"]` / `ENV["path"]` の取得・更新・削除は同じ変数を対象にします。
新規に定義する場合は、指定した名前の表記をそのまま使います。

## 再帰検索の例

`Dir.entries` と `File` API を使うと、外部 `dir` コマンドに依存せず Ruby 側で再帰的にファイルをたどれます。

```ruby
def walk(path, &block)
  Dir.entries(path).each do |name|
    next if name == "." || name == ".."

    child = path == "." ? name : path + "/" + name
    if File.directory?(child)
      walk(child, &block)
    elsif File.file?(child)
      block.call(child)
    end
  end
end
```

`samples/os/tree_grep.rb` は、この方式で指定拡張子のファイルを探し、Ruby 側で文字列検索するサンプルです。

## 外部 grep を使う例

Human68k 上に `grep` や `find` がある場合は、Ruby から外部コマンドとして呼び出し、結果だけ Ruby 側でまとめることもできます。

`samples/os/bq_grep_cmd.rb` は、`Dir.glob("**/*.rb")` のように対象ファイルを集め、各ファイルに外部 `grep` を実行し、結果を Ruby 側の配列に持って最後に出力するサンプルです。

```ruby
out = `grep x68k samples/*.rb`
puts out
```

## サンプル

OS / Shell API 関連のサンプルは以下です。

```text
samples/os/backquote_chk.rb  バッククォートの最小確認
samples/os/bq_dir_sort.rb    dir 出力をRubyで整形・ソートする例
samples/os/bq_grep_cmd.rb    外部 grep の結果をRuby側でまとめる例
samples/os/tree_grep.rb      Ruby側で再帰探索とgrep相当処理を行う例
samples/os/shell_ops.rb      system, Dir, File, FileUtils, ENV の基本操作例
samples/os/pathname_chk.rb   File のパス分解と Pathname の基本確認
samples/os/os_api_test.rb    OS / Shell API のまとめテスト
```

## Pathname

`Pathname` は、OS API を Ruby 側で扱いやすくするための小さな補助クラスとして用意しています。
CRuby の `Pathname` 完全互換ではありません。

```ruby
p = Pathname.new("samples")
p.children.each do |child|
  puts child.basename
end
```

現在の主なメソッドは以下です。

```ruby
Pathname.new(path)
to_s
join(*parts)
+(other)
basename
dirname
extname
expand_path
exist?
file?
directory?
read
write(data)
size
children
```

`join` は `/` でパスを連結します。
Human68k 上の実ファイルアクセスでは `\\` が基本ですが、mruby-x68k のパス分解処理では `/` と `\\` の両方を区切りとして扱います。

## 注意点

- バッククォートは暫定導入で、一時ファイルを使います。`TMP` / `TEMP` が未定義の場合はカレントディレクトリに作成します。
- `$?` は整数の終了ステータスです。CRuby の `Process::Status` ではありません。
- 改行コードは Human68k のコマンド出力に依存します。行分割時は CR/LF の両方を考慮してください。
- `FileUtils.rm` は通常ファイル削除用です。再帰削除はありません。
- `Dir.chdir` のブロック形式は未対応です。
- `File.open` は簡易ラッパーです。CRuby の File と完全互換ではありません。
- パス分解では `/`、`\\`、`:` を区切りとして扱います。`Pathname#join` は `/` で連結します。

## 今後の候補

今後、必要に応じて以下を検討します。

- `Dir.chdir` のブロック形式
- `FileUtils.mkdir_p`
- 安全性を確認したうえでの限定的な再帰コピー/削除
- `ENV.each` 相当
- 終了ステータスの詳細表現
