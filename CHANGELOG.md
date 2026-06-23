# Changelog

## 0.4.0 - Unreleased

- `mruby-x68k-stdio` を廃止し、OS系APIを `mruby-x68k-os`、X68000ハードウェア系APIを `mruby-x68k-hardware` に分離しました。
- 配布バイナリを `bin/` 以下に整理しました。
  - `mruby.x`: フル版 `.rb` 実行環境
  - `mrbc.x`: X68000上で動く `.rb` to `.mrb` コンパイラ
  - `mrb.x`: フル版 `.mrb` 実行専用VM
  - `mrb-os.x`: OS-only版 `.mrb` 実行専用VM
- `mrb.x` / `mrb-os.x` は mruby/c ではなく、mruby本体の RiteBinary `.mrb` 実行専用VMであることをドキュメントに明記しました。
- WSL上のビルド設定を、フル版、OS-only版、`.mrb` VM版をまとめて生成できる構成に整理しました。
- 同梱している libzm2 ヘッダを更新し、libzm2 1.1.0 への更新を反映しました。

- Human68k 上で mruby をシェルスクリプト的に使うためのOS系APIを拡充しました。
  - `system` と `$?` による外部コマンド実行と終了ステータス取得を追加しました。
  - バッククォート構文で外部コマンドの標準出力を文字列として取得できるようにしました。
  - バッククォートの一時ファイルは `TMP` / `TEMP` 環境変数を参照して生成先を選ぶようにしました。
  - `String#lines` / `String#each_line` を追加し、外部コマンド出力をRuby側で処理しやすくしました。
- ファイル・ディレクトリ操作APIを拡充しました。
  - `File.read`, `File.write`, `File.readlines`, `File.exist?`, `File.size`, `File.delete`, `File.rename`
  - `File.basename`, `File.dirname`, `File.expand_path`
  - `Dir.pwd`, `Dir.chdir`, `Dir.entries`, `Dir.exist?`, `Dir.mkdir`, `Dir.delete`, `Dir.glob`
  - `FileUtils.cp`, `mv`, `mkdir`, `rm`
- `ENV` 相当の環境変数APIを追加しました。
  - `ENV[name]`, `ENV[name]=`, `ENV.delete`, `ENV.keys`
  - Human68k の挙動に合わせ、参照時は環境変数名の大文字小文字を区別しないようにしました。
- 最小的な `Pathname` クラスを追加しました。
  - パスの結合、展開、存在確認、ディレクトリ判定など、OSスクリプト用途で使う範囲を実装しています。
- `load` / `require` を追加しました。
  - `mruby.x` では `.rb` / `.mrb` の読み込みに対応しています。
  - `mrb.x` / `mrb-os.x` では `.mrb` の読み込みに対応しています。
  - 実装は [mattn/mruby-require](https://github.com/mattn/mruby-require) を参考にし、X68000向けに動的ライブラリ読み込みを除いた形で取り込んでいます。
- コマンドラインから `-e` で Ruby コードを直接実行できることをドキュメントに追記しました。
- 低メモリ環境での動作を改善しました。
  - `_dos_malloc` 失敗時の Human68k 固有の戻り値を正しく判定するようにしました。
  - `mrbc.x` は最大空きブロックが不足している場合、必要メモリ量と最大空きブロック量を表示して終了するようにしました。
  - `mruby.x` / `mrb.x` でもメモリ確保失敗時に無言終了しにくいよう、Out of memory 表示を整理しました。
- サンプルのディレクトリ構成を用途別に整理しました。
- OS API のサンプルを整理・追加しました。
  - バッククォートで `dir` 結果を取得してRuby側で整形するサンプル
  - Rubyのみで再帰検索する `tree_grep.rb`
  - 外部 `grep` コマンドを利用して結果をRuby側で保持・整形するサンプル
  - `load` / `require` の `.rb` / `.mrb` 読み込みサンプル

## 0.3.1

- SEGA 3B/6B パッド読み取りAPIを追加しました。
  - `X68k::Joy.sega3b`, `sega3b_raw`
  - `X68k::Joy.sega6b`, `sega6b_raw`, `sega6b_scan_raw`
  - 通常利用向けの5フェーズ読み取りと、診断向けのRAWスキャンを分けています。
- AJOY.X 経由の CyberStick / アナログジョイスティック入力APIを追加しました。
  - `X68k::Ajoy.available?`, `read_raw`, `read`, `trigger_mask`, `buttons`
  - スロットル反転、ボタンプリセット、A/A+/B/B+ の扱いを整理しました。
- CyberStick / AJOY.X を使ったワイヤーフレーム風フライトサンプル `cyber_flight.rb` を追加しました。
- グラフィック描画ページを切り替える `X68k::Screen.apage` を追加しました。
  - `X68k::Screen.vpage` と組み合わせたページ切り替え描画をサンプルとドキュメントに反映しました。
  - mode 10 のグラフィック色指定について、実機確認に基づくメモを追加しました。
- VSync / Raster / Timer-D / OPM の割り込みpoll APIと確認用サンプルを整理しました。
  - Ruby VM を割り込みハンドラから直接触らず、C側カウンタをRuby側からpollする方針です。
  - Z-MUSIC と衝突しやすい割り込みを壊さないよう、enable/disable と調査用サンプルを分けています。
- Ruby のバッククォート構文で外部コマンドの標準出力を文字列として取得できるようにしました。
  - 現時点では暫定導入で、実装方式や制約は今後見直す可能性があります。

## 0.3.0

- Z-MUSIC 呼び出しを libzm2 ベースに変更しました。
  - libzm2 ヘッダを `third_party/libzm2/` に同梱しています。
  - `play_bgm`, `play_se`, `play_adpcm`, `fadeout`, `stop`, `status` などの Ruby API は維持しています。
  - ZMD の共通コマンド解析は mruby-x68k 独自実装ではなく libzm2 の `zm2_get_zmd_common_size()` を利用します。
- libzm2 の `COPYING` / `COPYING.RUNTIME` を同梱し、`THIRD_PARTY_NOTICES.md` に利用条件と作者確認済みの配布許可を記載しました。

## 0.2.0

> Note: v0.2.0 の公開バイナリリリースは、Z-MUSIC 効果音再生まわりの実装に
> 権利面の懸念があったため取り下げました。v0.3.0 では libzm2 作者さんの
> 許可を得たうえで、該当処理を libzm2 ベースへ変更しています。

- Z-MUSIC v2 ファンクションコールの Ruby API を追加しました。
  - BGM ZMD再生
  - ZMD効果音再生
  - ADPCM再生
  - `fadeout`, `stop`, `status`, `pcm8?`
- CRTC の VBlank チェックと `wait_vblank` を追加しました。
- VSync / Raster 割り込みを C ハンドラで計数し、Ruby側から poll する API とサンプルを追加しました。
- スーパーバイザ切り替え、割り込み禁止、汎用IOCS呼び出しを追加しました。
- `maze_chase.rb` を BGM/SE/VBlank/ワープ通路対応のゲームサンプルとして更新しました。
- 高速な実機やアクセラレータ環境向けに `maze_chase.rb` の `wait=N` / `speed=N` 指定を追加しました。
- `samples/ZMD/` に Z-MUSIC 用サンプル音源を同梱しました。

## 0.1.0

- Sharp X68000 / Human68k 向け `mruby.x` の初回実験リリースです。
- Ruby スクリプトの直接実行を確認しました。
- `puts`, `print`, `p`, `printf` を確認しました。
- `File.exist?`, `File.read`, `File.write`, `File.size`, `File.open` を追加しました。
- キーボード入力、ジョイスティック入力を追加しました。
- IOCS ベースのグラフィック描画を追加しました。
- スプライト表示、PCG 定義、パレット設定を追加しました。
- BG によるタイルマップ表示を追加しました。
- `maze_chase.rb` によるゲーム的なサンプル動作を確認しました。
