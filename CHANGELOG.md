# Changelog

## 0.3.1 - Unreleased

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
