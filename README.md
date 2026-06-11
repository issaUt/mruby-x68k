# mruby-x68k

Sharp X68000 / Human68k 上で `mruby` を動かすための実験的なランタイム補助ライブラリとサンプル集です。

このリポジトリは、X68000 上で Ruby スクリプトを直接実行しながら、ファイル操作、キーボード/ジョイスティック入力、IOCS ベースの画面制御、グラフィック、スプライト、BG を少しずつ扱えるようにすることを目的にしています。

## 対象環境

- Sharp X68000 / Human68k
- elf2x68k + xdev68k 系のクロスビルド環境
- 動作確認: XM6 TypeG
- 開発環境: WSL / Windows からのクロスコンパイル

## 現在できること

- `mruby.x` による Ruby スクリプト実行
- `puts`, `print`, `p`, `printf`
- `File.exist?`, `File.read`, `File.write`, `File.size`, `File.open`
- IOCS ベースのグラフィック描画
- テキスト表示、カーソル制御、画面クリア
- キーボード入力、ジョイスティック入力
- スプライト表示、PCG 定義、パレット設定
- 1面BGのタイル表示、スクロール、マップ描画
- Ruby による簡単なゲームロジック

## サンプル

主なサンプルは `samples/` にあります。

- `HexCheck.rb`: HEXファイル変換系の実用ツール例
- `bg_main.rb`, `bg_map.rb`: BG表示とスクロールの確認
- `graph_demo.rb`: グラフィック画面への線・矩形・塗りつぶし描画
- `joy_spr.rb`, `spr_move.rb`: 入力とスプライト移動
- `map_chk.rb`: BGマップ表示確認
- `pcg_chk.rb`: PCGとBGコードの確認
- `maze_chase.rb`: パックマン風のゲームサンプル

`maze_chase.rb` は、現時点の「Rubyでどこまでゲームらしいものを書けるか」を見るためのサンプルです。
Gosu で書いたようなクラス中心のゲームロジックを、X68000 の BG / スプライト / ジョイスティック入力へ割り当てています。

実装済みの要素:

- BGによる迷路表示
- スプライトによる自機と敵
- ジョイスティック/キーボード移動
- 餌、パワー餌、スコア
- 敵の追跡、散開、反撃時の逃走
- 反撃時の青表示と終了直前の点滅
- 残機、ミス時の再開、全餌取得時のクリア表示
- 固定小数点による速度差

実行例:

```text
mruby maze_chase.rb joy 4
mruby maze_chase.rb key 2
mruby maze_chase.rb clear
```

`clear` はクリア表示確認用のテストモードです。
ファイル名は X68000 / Human68k で扱いやすいよう、拡張子込み21文字以内にしています。
実装上のメモは [docs/game-sample-notes.md](docs/game-sample-notes.md) にまとめています。

## ビルド

開発時に使っているローカル環境では、mruby ソースツリーに `mrbgems/mruby-x68k-stdio` を配置し、X68000向け設定でビルドします。

```sh
cd /home/utsu/work/elf2x68k/mruby-host
MRUBY_CONFIG=x68k rake -j2
```

生成される実行ファイル:

```text
build/x68k/bin/mruby.x
```

配布用の `mruby.x` では、`mruby --version` / `mruby -v` で `mruby-x68k` のバージョンも表示するため、
[patches/mruby-bin-mruby-x68k-version.patch](patches/mruby-bin-mruby-x68k-version.patch) を mruby 側の `mruby-bin-mruby` に適用しています。

詳細は [docs/build-wsl.md](docs/build-wsl.md) を参照してください。C拡張の追加方法は [docs/c-extension.md](docs/c-extension.md) にまとめています。

## X68000 と Ruby の相性メモ

今回の実験で、X68000 上でも Ruby でゲーム的なコードは十分書ける感触があります。クラス化した敵オブジェクト、状態管理、マップ処理、ジョイスティック入力などは、Ruby の書きやすさがかなり効きます。

一方で、MC68000 では以下の処理は重くなりやすいです。

- 毎フレームの大量オブジェクト生成
- 配列生成や文字列比較を含む壁判定
- グラフィック面への細かい描画呼び出しの連発
- 起動時の大きなテーブル生成

軽くするために有効だった方針:

- マップは起動時に数値化する
- 通行判定は必要な4セルだけを見る
- HUDはグラフィック描画ではなくBG/PCGで表示する
- スプライト移動はハードウェアに任せる
- 速度差は固定小数点で表現する
- 重い処理をC側APIへ寄せる余地を残す

68030/060turbo ではキャッシュと速度の恩恵で、Rubyらしいオブジェクト指向コードもさらに現実的になるはずです。

## ライセンス

このリポジトリ固有のコードは MIT License です。

mruby 本体も MIT License ですが、このリポジトリには mruby 本体は含めていません。ツールチェーンや外部プロジェクトも同梱していません。詳細は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。
