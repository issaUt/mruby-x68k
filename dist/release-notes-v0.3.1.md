# mruby-x68k v0.3.1

Sharp X68000 / Human68k 向け `mruby.x` の実験リリースです。

## 主な変更

- SEGA 3B/6B パッド読み取りAPIを追加しました。
  - `X68k::Joy.sega3b`, `sega3b_raw`
  - `X68k::Joy.sega6b`, `sega6b_raw`, `sega6b_scan_raw`
- AJOY.X 経由の CyberStick / アナログジョイスティック入力APIを追加しました。
  - `X68k::Ajoy.read_raw`, `read`, `trigger_mask`, `buttons`
  - スロットル反転、ボタンプリセット、A/A+/B/B+ の扱いを整理しています。
- CyberStick / AJOY.X 入力を使ったワイヤーフレーム風フライトサンプル `cyber_flight.rb` を追加しました。
- グラフィック描画ページを切り替える `X68k::Screen.apage` を追加しました。
  - `X68k::Screen.vpage` と組み合わせることで、簡易的なページ切り替え描画ができます。
  - mode 10 の色指定について、実機確認に基づくメモを追加しています。
- VSync / Raster / Timer-D / OPM の割り込みpoll APIと確認用サンプルを整理しました。
  - Ruby VM を割り込みハンドラから直接触らず、C側カウンタをRuby側からpollする方針です。
- Ruby のバッククォート構文で外部コマンドの標準出力を文字列として取得できるようにしました。
  - 現時点では暫定導入です。
  - コマンドの標準出力を一時ファイルへリダイレクトし、それを文字列として読み戻します。

## 同梱ファイル

- `mruby.x`
  - X68000 / Human68k 向け mruby 実行ファイル
- `samples/`
  - Rubyサンプルと Z-MUSIC 用サンプル音源
- `README-release.txt`
- `release-notes-v0.3.1.md`
- `LICENSE`
- `THIRD_PARTY_NOTICES.md`
- `third_party/libzm2/COPYING`
- `third_party/libzm2/COPYING.RUNTIME`

## バージョン表示

`mruby --version` または `mruby -v` で、`mruby-x68k` 側のバージョンも表示します。

```text
mruby-x68k 0.3.1
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

バッククォートの確認:

```text
mruby backquote_chk.rb
mruby backquote_chk.rb "dir *.rb"
```

## 注意事項

- このリリースは実験版です。
- Z-MUSIC サンプルを鳴らすには Z-MUSIC.X が常駐している必要があります。
- libzm2 の利用条件は `THIRD_PARTY_NOTICES.md` を確認してください。
- AJOY.X は同梱していません。CyberStick / アナログジョイスティックサンプルを使う場合は別途常駐させてください。
- SEGA 3B/6B パッドは、TH/Select を X68000 ジョイスティック pin 8 へ配線した変換アダプタを前提にしています。
- バッククォート対応は暫定導入です。実装方式や制約は今後見直す可能性があります。
- このリポジトリには mruby 本体、xdev68k、elf2x68k、GCC、newlib、エミュレータ本体、Z-MUSIC.X、AJOY.X は含めていません。
- `mruby.x` を再配布する場合は、mruby のライセンス表記もあわせて確認してください。
