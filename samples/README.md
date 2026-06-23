# samples

mruby-x68k のサンプルは、機能ごとにディレクトリを分けています。
詳しい一覧は `../docs/samples.md` を参照してください。

```text
os/        バッククォート、File/Dir/ENV、grep系、実用ツール
graphics/  グラフィック、BG、スプライト
input/     キーボード、ジョイスティック、CyberStick
sound/     Z-MUSIC、SE、ADPCM
games/     ゲーム、デモ
system/    CRTC、割り込み、IOCS、スーパーバイザ
ZMD/       Z-MUSIC用サンプル音源
```

- `os/require_test.rb`: `load` / `require` で別Rubyファイルを読み込む例。
- `os/require_mrb_test.rb`: VM用に `.mrb` を `require` する例。
