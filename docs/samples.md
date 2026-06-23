# サンプル一覧

`samples/` 配下のサンプルは、機能ごとにディレクトリを分けています。
X68000 / Human68k 上でも扱いやすいよう、各ファイル名は長くなりすぎない範囲にしています。

## ディレクトリ構成

```text
samples/os/        バッククォート、File/Dir/ENV、grep系、実用ツール
samples/graphics/  グラフィック、BG、スプライト
samples/input/     キーボード、ジョイスティック、CyberStick
samples/sound/     Z-MUSIC、SE、ADPCM
samples/games/     ゲーム、デモ
samples/system/    CRTC、割り込み、IOCS、スーパーバイザ
samples/ZMD/       Z-MUSIC用サンプル音源
```

## OS / Shell

- `samples/os/backquote_chk.rb`
  - バッククォートで外部コマンドの標準出力を文字列として取得する確認。
- `samples/os/bq_dir_sort.rb`
  - `dir` の出力をRuby側で整形、ソートする例。
- `samples/os/bq_grep_cmd.rb`
  - `Dir.glob` でファイルを集め、外部 `grep` を実行し、結果をRuby側でまとめる例。
- `samples/os/tree_grep.rb`
  - `Dir.entries` / `File.readlines` を使い、Rubyだけで再帰検索とgrep相当処理を行う例。
- `samples/os/require_test.rb`
  - `load` / `require` で別Rubyファイルを読み込む例。
- `samples/os/require_mrb_test.rb`
  - `mrb.x` / `mrb-os.x` で `.mrb` を `require` する例。
- `samples/os/shell_ops.rb`
  - `system`, `Dir`, `File`, `FileUtils`, `ENV` を使ったシェルスクリプト風サンプル。
- `samples/os/pathname_chk.rb`
  - `File.basename` / `File.dirname` / `File.extname` と `Pathname` の確認。
- `samples/os/os_api_test.rb`
  - OS / Shell API のまとめテスト。
- `samples/os/HexCheck.rb`
  - HEXファイル変換系の実用ツール例。

## Graphics

- `samples/graphics/graph_demo.rb`
  - グラフィック画面への線、矩形、塗りつぶし描画。
- `samples/graphics/bg_main.rb`, `samples/graphics/bg_map.rb`
  - BG表示とスクロール確認。
- `samples/graphics/map_chk.rb`
  - BGマップ表示確認。
- `samples/graphics/spr_move.rb`
  - スプライト移動。

## Input

- `samples/input/joy_spr.rb`
  - 入力とスプライト移動の組み合わせ。
- `samples/input/ajoy_chk.rb`
  - AJOY.X 経由の CyberStick / アナログジョイスティック確認。
- `samples/input/cyber_poll.rb`
  - CyberStick直接ポーリング調査用。通常利用はAJOY.X経由を推奨。
- `samples/input/joy_sega3b_test.rb`
  - SEGA 3ボタンパッド確認。
- `samples/input/joy_sega6b_test.rb`
  - SEGA 6ボタンパッド確認。
- `samples/input/joy_sega6b_scan.rb`
  - SEGA 6ボタンパッドのスキャン列確認。

## Sound

- `samples/sound/zm_test.rb`
  - Z-MUSIC常駐確認とZMD再生の最小テスト。
- `samples/sound/zm_se.rb`
  - Z-MUSICのSE再生テスト。
- `samples/sound/zm_adpcm.rb`
  - ADPCM再生テスト。
- `samples/sound/zm_game_audio.rb`
  - BGM再生中にキー入力でSE/ADPCMを鳴らすゲーム用途サンプル。

## Games / Demos

- `samples/games/maze_chase.rb`
  - BG、スプライト、入力、VSync、Z-MUSICを使ったパックマン風サンプル。
- `samples/games/cyber_flight.rb`
  - CyberStick / AJOY.X 入力を使ったワイヤーフレーム風フライトデモ。

## System

- `samples/system/crtc_chk.rb`
  - CRTC垂直表示/垂直帰線待ちの確認。
- `samples/system/sys_iocs_chk.rb`
  - スーパーバイザ切り替え、割り込み禁止、汎用IOCS呼び出しの確認。
- `samples/system/int_poll_chk.rb`
  - VSync割り込みをpoll方式で確認。
- `samples/system/int_raster_chk.rb`
  - Raster割り込みをpoll方式で確認。
- `samples/system/int_timer_chk.rb`
  - Timer-D割り込みをpoll方式で確認。

## Z-MUSIC Assets

`samples/ZMD/` には、Z-MUSICサンプル用の音源データを置いています。

```text
samples/ZMD/simplebgm.ZMD   通常BGM
samples/ZMD/attackbgm.ZMD   パワー餌中BGM
samples/ZMD/eat.ZMD         ドット取得SE
samples/ZMD/charge.ZMD      パワー餌、敵撃破SE
samples/ZMD/miss.ZMD        ミス時ジングル
samples/ZMD/laser.ZMD       Z-MUSIC SEテスト用
samples/ZMD/snare.pcm       ADPCMテスト用
```
