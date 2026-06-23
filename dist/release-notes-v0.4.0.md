# mruby-x68k v0.4.0

Sharp X68000 / Human68k 向け `mruby.x` の実験リリースです。

## 主な変更

- OS系APIとX68000ハードウェア系APIをgem単位で分離しました。
- 配布バイナリを `bin/` 以下へ整理し、`.rb` 実行環境、`.mrb` コンパイラ、`.mrb` 実行専用VMを同梱しました。
- Human68k 上で mruby をシェルスクリプト的に使うためのOS系APIを拡充しました。
- `load` / `require` を追加しました。
- 低メモリ環境でのメモリ確保失敗時の扱いを改善しました。小規模なスクリプトであれば2MB環境でも実行でき、事前に `.mrb` へコンパイルして `mrb.x` / `mrb-os.x` で実行することで実行時メモリを抑えられます。
- サンプルのディレクトリ構成を用途別に整理しました。
- 同梱している libzm2 ヘッダを更新し、libzm2 1.1.0 への更新を反映しました。

### Gem / バイナリ構成

- `mruby-x68k-stdio` を廃止し、OS系APIを `mruby-x68k-os`、X68000ハードウェア系APIを `mruby-x68k-hardware` に分離しました。
- 配布バイナリを `bin/` 以下へ整理しました。
  - `mruby.x`: フル版 `.rb` 実行環境
  - `mrbc.x`: X68000上で動く `.rb` to `.mrb` コンパイラ。実行時APIを含めない専用最小構成です。
  - `mrb.x`: フル版 `.mrb` 実行専用VM
  - `mrb-os.x`: OS-only版 `.mrb` 実行専用VM
- `mrb.x` / `mrb-os.x` は mruby/c ではなく、mruby本体の RiteBinary `.mrb` 実行専用VMであることを明記しました。

### OS / シェルスクリプト向けAPI

Human68k 上で mruby をシェルスクリプト的に使うためのOS系APIを拡充しました。

- `system` と `$?` による外部コマンド実行と終了ステータス取得を追加しました。
- Ruby のバッククォート構文で外部コマンドの標準出力を文字列として取得できるようにしました。
  - 現時点では暫定導入です。
  - コマンドの標準出力を一時ファイルへリダイレクトし、それを文字列として読み戻します。
  - 一時ファイルの生成先は `TMP` / `TEMP` 環境変数も参照します。
- `String#lines` / `String#each_line` を追加し、外部コマンド出力をRuby側で処理しやすくしました。
- ファイル・ディレクトリ操作APIを拡充しました。
  - `File.read`, `File.write`, `File.readlines`, `File.exist?`, `File.size`, `File.delete`, `File.rename`
  - `File.basename`, `File.dirname`, `File.expand_path`
  - `Dir.pwd`, `Dir.chdir`, `Dir.entries`, `Dir.exist?`, `Dir.mkdir`, `Dir.delete`, `Dir.glob`
  - `FileUtils.cp`, `mv`, `mkdir`, `rm`
- `ENV` 相当の環境変数APIを追加しました。
  - `ENV[name]`, `ENV[name]=`, `ENV.delete`, `ENV.keys`
  - Human68k の挙動に合わせ、参照時は環境変数名の大文字小文字を区別しないようにしています。
- 最小的な `Pathname` クラスを追加しました。
- コマンドラインから `-e` で Ruby コードを直接実行できることをドキュメントに追記しました。

### load / require

- `load` / `require` を追加しました。
  - `mruby.x` では `.rb` / `.mrb` の読み込みに対応しています。
  - `mrb.x` / `mrb-os.x` では `.mrb` の読み込みに対応しています。
  - 実装は [mattn/mruby-require](https://github.com/mattn/mruby-require) を参考にし、X68000向けに動的ライブラリ読み込みを除いた形で取り込んでいます。

### メモリ不足時の改善

- `_dos_malloc` 失敗時の Human68k 固有の戻り値を正しく判定するようにしました。
- `mrbc.x` は最大空きブロックが不足している場合、必要メモリ量と最大空きブロック量を表示して終了するようにしました。
- `mruby.x` / `mrb.x` でもメモリ確保失敗時に無言終了しにくいよう、Out of memory 表示を整理しました。
- 小規模なスクリプトであれば2MB環境でも実行できます。
- `.rb` を直接実行する `mruby.x` より、事前に `.mrb` へコンパイルして `mrb.x` / `mrb-os.x` で実行するほうが実行時メモリを抑えられます。
- `mrbc.x` はコンパイル時にまとまった空きメモリを必要とするため、低メモリ環境では別環境で `.mrb` を生成して持ち込む運用も想定しています。

### サンプル構成

- サンプルのディレクトリ構成を用途別に整理しました。
- OS API、Z-MUSIC、ジョイスティック、グラフィックなど、用途ごとに確認しやすい配置にしています。

### libzm2

- 同梱している libzm2 ヘッダを更新し、libzm2 1.1.0 への更新を反映しました。
- libzm2 の利用条件は `THIRD_PARTY_NOTICES.md` を確認してください。

### 0.3.1 から継続して含まれる主な機能

- SEGA 3B/6B パッド読み取りAPI
  - `X68k::Joy.sega3b`, `sega3b_raw`
  - `X68k::Joy.sega6b`, `sega6b_raw`, `sega6b_scan_raw`
- AJOY.X 経由の CyberStick / アナログジョイスティック入力API
  - `X68k::Ajoy.read_raw`, `read`, `trigger_mask`, `buttons`
  - スロットル反転、ボタンプリセット、A/A+/B/B+ の扱いを整理しています。
- CyberStick / AJOY.X 入力を使ったワイヤーフレーム風フライトサンプル `cyber_flight.rb`
- グラフィック描画ページを切り替える `X68k::Screen.apage`
- VSync / Raster / Timer-D / OPM の割り込みpoll APIと確認用サンプル

## 同梱ファイル

- `bin/mruby.x`
  - X68000 / Human68k 向けのフル版 mruby 実行ファイル
- `bin/mrbc.x`
  - X68000 / Human68k 上で `.rb` から `.mrb` を生成するコンパイラ。実行時APIを含めない専用最小構成です。
- `bin/mrb.x`
  - `.mrb` 実行専用のフル版VM。mruby/c ではなく、mruby本体のRiteBinary実行用VMです。
- `bin/mrb-os.x`
  - `.mrb` 実行専用のOS-only版VM。mruby/c ではなく、mruby本体のRiteBinary実行用VMです。
- `samples/`
  - Rubyサンプルと Z-MUSIC 用サンプル音源
- `README-release.txt`
- `release-notes-v0.4.0.md`
- `LICENSE`
- `THIRD_PARTY_NOTICES.md`
- `third_party/libzm2/COPYING`
- `third_party/libzm2/COPYING.RUNTIME`

## mruby/c との関係

`bin/mrb.x` / `bin/mrb-os.x` は mruby/c ではありません。
通常の mruby 本体からビルドした、RiteBinary `.mrb` 実行専用の軽量VMです。
`.rb` ソースを直接実行する機能や mruby コンパイラは含まないため、`.rb` から `.mrb` への変換は `bin/mruby.x` または `bin/mrbc.x` で行ってください。

また、`.mrb` は同じ mruby バージョンと互換性のあるgem/API構成で実行する前提です。
X68000ハードウェアAPIを使う `.mrb` は `bin/mrb.x`、OS系APIだけを使う `.mrb` は `bin/mrb-os.x` での実行を想定しています。

## バージョン表示

`mruby --version` または `mruby -v` で、`mruby-x68k` 側のバージョンも表示します。

```text
mruby-x68k 0.4.0
target: X68000 / Human68k
mruby 4.0.0 (2026-04-20)
```

## サンプル

基本サンプル:

```text
mruby graph_demo.rb
mruby joy_spr.rb
mruby maze_chase.rb
mruby maze_chase.rb wait=2
mruby maze_chase.rb noaudio
```

Z-MUSIC の確認:

```text
mruby zm_test.rb ZMD/simplebgm.ZMD
mruby zm_game_audio.rb
```

SEGA 3B/6B パッドの確認:

```text
mruby joy_sega3b_test.rb
mruby joy_sega6b_test.rb
mruby joy_sega6b_scan.rb
```

CyberStick / AJOY.X の確認:

```text
AJOY.X
mruby ajoy_chk.rb
mruby cyber_flight.rb
```

OS API / バッククォート / require の確認:

```text
mruby samples/os/backquote_chk.rb
mruby samples/os/bq_dir_sort.rb
mruby samples/os/tree_grep.rb .rb x68k
mruby samples/os/bq_grep_cmd.rb .rb x68k
mruby samples/os/require_test.rb
mrb samples/os/require_mrb_test.mrb
```

## 注意事項

- このリリースは実験版です。
- Z-MUSIC サンプルを鳴らすには Z-MUSIC.X が常駐している必要があります。
- libzm2 の利用条件は `THIRD_PARTY_NOTICES.md` を確認してください。
- AJOY.X は同梱していません。CyberStick / アナログジョイスティックサンプルを使う場合は別途常駐させてください。
- SEGA 3B/6B パッドは、TH/Select を X68000 ジョイスティック pin 8 へ配線した変換アダプタを前提にしています。
- バッククォート対応は暫定導入です。実装方式や制約は今後見直す可能性があります。
- `load` / `require` は X68000 向けの最小実装です。動的ライブラリ読み込みには対応していません。
- このリポジトリには mruby 本体、xdev68k、elf2x68k、GCC、newlib、エミュレータ本体、Z-MUSIC.X、AJOY.X は含めていません。
- `bin/` 以下の実行ファイルを再配布する場合は、mruby のライセンス表記もあわせて確認してください。
