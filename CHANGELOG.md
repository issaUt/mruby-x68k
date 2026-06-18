# Changelog

## Unreleased

- Z-MUSIC 呼び出しを libzm2 ベースに変更しました。
  - libzm2 ヘッダを `third_party/libzm2/` に同梱しています。
  - `play_bgm`, `play_se`, `play_adpcm`, `fadeout`, `stop`, `status` などの Ruby API は維持しています。
  - ZMD の共通コマンド解析は mruby-x68k 独自実装ではなく libzm2 の `zm2_get_zmd_common_size()` を利用します。
- libzm2 の `COPYING` / `COPYING.RUNTIME` を同梱し、`THIRD_PARTY_NOTICES.md` に利用条件と作者確認済みの配布許可を記載しました。

