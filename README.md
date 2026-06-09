# mruby-x68k

Sharp X68000 / Human68k 上で `mruby` を動かすための実験的なランタイム補助ライブラリとサンプル集です。

このリポジトリには、X68000 向けの小さな `mruby` gem と、動作確認用の Ruby スクリプトを含めています。テキスト、グラフィック、スプライト、キー入力、ジョイスティック、ファイル操作、BG タイルマップ描画などを少しずつ実装しています。

## 対象環境

- X68000 / Human68k
- elf2x68k + xdev68k 系のクロスビルド環境
- 動作確認用エミュレータ: XM6 TypeG

## 現在できること

現時点では、小さなツールや簡単なゲーム風の画面実験に使える程度まで動いています。

- `mruby.x` で Ruby スクリプトを X68000 上で実行
- `puts`, `print`, `p`
- ファイルの存在確認、読み書き、サイズ取得
- IOCS ベースのグラフィック描画
- テキスト表示、カーソル制御、画面クリア
- スプライト表示
- キーボード / ジョイスティック入力
- 1面 BG のタイルマップ描画とスクロール

BG 周りは、現時点では安全側に寄せています。安定して使える API は、`gameB` サンプルで確認できた実用ルートに合わせて、BG テキスト page 1 (`0xEBE000`) と scroll register 0 を使う 1面 BG です。BG0/BG1 の独立2面スクロールは、まだ仕様確認中です。

## 構成

```text
mrbgems/
  mruby-x68k-stdio/
    mrbgem.rake
    src/x68k_stdio.c
    mrblib/
samples/
  bg_main.rb
  bg_map.rb
  spr_move.rb
  joy_spr.rb
  HexCheck.rb
docs/
```

## ビルド

`mrbgems/mruby-x68k-stdio` を mruby のソースツリーへ配置し、X68000 向けビルド設定から gem を有効化してビルドします。

開発時に使っているローカル環境では、以下のようにビルドしています。

```sh
cd /home/utsu/work/elf2x68k/mruby-host
MRUBY_CONFIG=x68k rake -j2
```

生成される実行ファイル:

```text
build/x68k/bin/mruby.x
```

ローカル環境では、実行用フォルダへ次のようにコピーしています。

```sh
cp build/x68k/bin/mruby.x /home/utsu/work/elf2x68k/mruby.x
```

詳しい作業メモは [docs/build-wsl.md](docs/build-wsl.md) を参照してください。

## 実行例

Human68k / XM6 上で:

```text
mruby bg_map.rb
```

`bg_map.rb` は 64x64 のタイルマップを `X68k::Bg.map_draw_full` で BG に展開し、スクロールします。初期版では Ruby からタイルを1個ずつ描画していましたが、現在は C 側の一括転送 API を使っています。

## ライセンス

このリポジトリ固有のコードは MIT License で公開します。

mruby 本体も MIT License ですが、このリポジトリには mruby 本体は含めていません。ツールチェーンや外部プロジェクトも同梱しません。詳細は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。

## 現在の方針

まずは「X68000上で Ruby スクリプトを書いて、画面・入力・ファイルを扱える」ことを優先しています。完全な Ruby 互換や、X68000 の全機能を一気に網羅することは目標にしていません。

当面の重点:

- 実機/エミュレータで安定して使える API の整理
- BG タイルマップとスプライトを組み合わせたサンプル
- ファイル処理ツールとして使えるサンプルの追加
- ドキュメント整備
