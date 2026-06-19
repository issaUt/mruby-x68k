# mruby-x68k

Sharp X68000 / Human68k 上で `mruby` を動かすための実験的なランタイム補助ライブラリとサンプル集です。

このリポジトリは、X68000 上で Ruby スクリプトを直接実行しながら、ファイル操作、キーボード/ジョイスティック入力、IOCS ベースの画面制御、グラフィック、スプライト、BG を少しずつ扱えるようにすることを目的にしています。

## 更新履歴

v0.3.0 では kg68k / TcbnErik さんの [libzm2](https://github.com/kg68k/libzm2) を利用させていただきました。その他主な変更点は [CHANGELOG.md](CHANGELOG.md) にまとめています。

## 対象環境

- Sharp X68000 / Human68k
- elf2x68k + xdev68k 系のクロスビルド環境
- 動作確認: X68000 SUPER(PhantomX), XM6 TypeG
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
- CRTC垂直表示/垂直帰線期間のチェックと待機
- VSync/Raster割り込みのCハンドラ計数とRuby側poll
- スーパーバイザ切り替え、割り込み禁止、汎用IOCS呼び出し
- Z-MUSIC常駐時のZMD再生、効果音再生、停止、フェードアウト、PCM8状態確認
- Ruby による簡単なゲームロジック

## サンプル

主なサンプルは `samples/` にあります。

- `HexCheck.rb`: HEXファイル変換系の実用ツール例
- `bg_main.rb`, `bg_map.rb`: BG表示とスクロールの確認
- `graph_demo.rb`: グラフィック画面への線・矩形・塗りつぶし描画
- `joy_spr.rb`, `spr_move.rb`: 入力とスプライト移動
- `map_chk.rb`: BGマップ表示確認
- `crtc_chk.rb`: CRTC垂直表示/垂直帰線待ちの確認
- `sys_iocs_chk.rb`: スーパーバイザ切り替え、割り込み禁止、汎用IOCS呼び出しの確認
- `int_poll_chk.rb`: VSync割り込みをpoll方式で確認するテスト
- `int_raster_chk.rb`: Raster割り込みをpoll方式で確認するテスト
- `int_timer_chk.rb`: Timer-D割り込みをpoll方式で確認するテスト
- `zm_test.rb`: Z-MUSIC常駐確認とZMD再生の最小テスト
- `zm_se.rb`, `zm_adpcm.rb`: Z-MUSIC効果音/ADPCM再生の最小テスト
- `zm_game_audio.rb`: BGM再生中にキー入力でZMD効果音/ADPCMを鳴らすテスト
- `maze_chase.rb`: BGM/SE/VSync/BG/スプライトを使ったパックマン風ゲームサンプル

Z-MUSIC用のサンプル音源は `samples/ZMD/` に含めています。

```text
samples/ZMD/simplebgm.ZMD   通常BGM
samples/ZMD/attackbgm.ZMD   パワー餌中BGM
samples/ZMD/eat.ZMD         ドット取得SE
samples/ZMD/charge.ZMD      パワー餌/敵撃破SE
samples/ZMD/miss.ZMD        ミス時ジングル
samples/ZMD/laser.ZMD       Z-MUSIC SEテスト用
samples/ZMD/snare.pcm       ADPCMテスト用
```

`maze_chase.rb` は、X68000 の BG / スプライト / ジョイスティック入力 / Z-MUSIC を使用して「mrubyでゲームらしいものを書けるか」を見るためのサンプルです。

実装済みの要素:

- BGによる迷路表示
- スプライトによる自機と敵
- ジョイスティック/キーボード移動
- 餌、パワー餌、スコア
- 敵の追跡、散開、反撃時の逃走
- 反撃時の青表示と終了直前の点滅
- 残機、ミス時の再開、全餌取得時のクリア表示
- 左右ワープ通路と敵のトンネル減速
- VBlank待ちによるフレーム同期
- 通常BGM、パワー餌中BGM、ドット/敵撃破/ミス音
- 固定小数点による速度差表現

実行例:

```text
mruby maze_chase.rb
mruby maze_chase.rb joy
mruby maze_chase.rb 2
mruby maze_chase.rb wait=2
mruby maze_chase.rb speed=30
mruby maze_chase.rb noaudio
mruby maze_chase.rb clear
```

無指定時はキーボード操作、敵4体で起動します。
`joy` / `joystick` を指定するとジョイスティック操作になります。
`clear` はクリア表示確認用のテストモードです。
`wait=N` はゲームループごとに待つVBlank数です。高速な実機では `wait=2`〜`wait=4` あたりで速度を落とせます。
`speed=30` は `wait=2` 相当の指定です。
`noaudio` / `no-audio` を指定すると Z-MUSIC なしで実行できます。
ファイル名は X68000 / Human68k で扱いやすいよう、拡張子込み21文字以内にしています。
実装上のメモは [docs/game-sample-notes.md](docs/game-sample-notes.md) にまとめています。

## 低レベル制御

スーパーバイザモード切り替え、割り込み禁止、汎用IOCS呼び出しを Ruby から扱えます。

```ruby
p X68k::Sys.super?

X68k::Sys.with_super do
  # I/Oレジスタや特権領域を読む処理
end

old_sr = X68k::Sys.interrupt_disable
# 短いクリティカルセクション
X68k::Sys.interrupt_enable(old_sr)

X68k::Sys.with_interrupt_disabled do
  # 割り込み禁止中に行う短い処理
end

p X68k::Iocs.call(0x01) # B_KEYSNS
```

`X68k::Sys.super(mode = true)` はスーパーバイザモードを切り替え、切り替え前の状態を true/false で返します。
`X68k::Sys.with_super` はブロック内だけスーパーバイザモードに入り、例外が発生しても元の状態へ戻します。

`X68k::Sys.interrupt_disable` は現在のSRを返して割り込みを禁止します。
復帰するときは、その戻り値を `X68k::Sys.interrupt_enable(old_sr)` に渡します。
`X68k::Sys.with_interrupt_disabled` はブロック内だけ割り込み禁止にし、例外が発生しても元のSRへ戻します。

汎用IOCS呼び出しは `X68k::Iocs.call(d0, d1 = 0, d2 = 0, d3 = 0, d4 = 0, d5 = 0, a1 = 0, a2 = 0, rd = 0, ra = 0)` です。
戻り値は通常 d0 です。
`rd` / `ra` に戻したいデータレジスタ/アドレスレジスタの個数を指定すると、`[d0, d1..., a1...]` の配列を返します。

文字列を `a1` / `a2` に渡す場合は、文字列オブジェクトのデータ先頭アドレスが渡されます。

```ruby
buf = "Hello\x00"
X68k::Iocs.call(0x21, 0, 0, 0, 0, 0, buf) # B_PRINT
```

低レベルAPIはX68000本体の状態を直接触れるため、スーパーバイザモードや割り込み禁止状態を戻し忘れないようにしてください。

## CRTC

CRTCの垂直表示/垂直帰線期間を Ruby から確認できます。

```ruby
p X68k::Crtc.vdisp?
p X68k::Crtc.vblank?
X68k::Crtc.wait_vblank
```

`vdisp?` は垂直表示期間中に true を返します。
`vblank?` は垂直帰線期間中に true を返します。
`wait_vblank` は次の垂直帰線期間開始まで待ちます。
すでに垂直帰線期間中に呼ばれた場合は、一度表示期間へ抜けてから次の垂直帰線期間を待ちます。
ゲームループで画面更新後に呼ぶと、同じ帰線期間中に複数フレーム進むことを避けられます。

## 割り込みpoll

割り込みハンドラからRubyコードを直接呼ばず、C側でカウンタだけを増やしてRuby側からpollできます。

```ruby
X68k::Interrupt.vsync_enable

loop do
  if X68k::Interrupt.vsync?
    n = X68k::Interrupt.vsync_count
    X68k::Interrupt.vsync_clear
    puts n
  end
end
```

現在の確認済み対象は VSync と Raster です。

```ruby
X68k::Interrupt.vsync_enable(disp = false, cycle = 1)
X68k::Interrupt.raster_enable(raster)
```

各割り込みには `*_disable`, `*_count`, `*_clear`, `*?` があります。
Timer-D は `timer_d_*` APIを用意していますが、現時点のXM6/Human68k確認環境ではIOCS経由の発火を確認できていません。
OPM割り込みは Z-MUSIC と衝突しやすいため、現時点では有効化していません。
Timer-DやOPMが必要な処理は、環境差を確認するまではVSync pollやVBlank待ちで代替してください。

割り込みハンドラ内ではRuby VMを触らないため安全寄りですが、登録した割り込みは終了前に必ず disable してください。

## Z-MUSIC

Z-MUSIC.X が常駐している環境では、Z-MUSIC v2 ファンクションコールを Ruby から呼び出せます。
Z-MUSIC 呼び出しには、kg68k / TcbnErik さんの header-only ライブラリ [libzm2](https://github.com/kg68k/libzm2) を利用しています。
libzm2 は `third_party/libzm2/` に同梱しており、Z-MUSIC の `trap #3` ファンクションコールや ZMD の共通コマンド処理は libzm2 側の実装に委ねています。
ライセンスと配布条件は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。
`.ZMS` は事前に `ZMSC.X` で `.ZMD` に変換しておき、mruby からは `.ZMD` を再生します。

```ruby
p X68k::ZMusic.available?
printf("%04X\n", X68k::ZMusic.version)
X68k::ZMusic.play_bgm("ZMD/simplebgm.ZMD", 100)
p X68k::ZMusic.status
p X68k::ZMusic.pcm8?
X68k::ZMusic.play_se(7, "ZMD/laser.ZMD", 80)
X68k::ZMusic.play_adpcm("ZMD/snare.pcm")
X68k::ZMusic.play_adpcm_note(60)
X68k::ZMusic.fadeout(20)
X68k::ZMusic.stop
```

`play_bgm(path, volume = 100, fast = false)` は ZMD を C 側のバッファに保持し、Z-MUSIC の `play_cnv_data` を呼びます。
`volume` は 0～100 のパーセントで、ZMD内の絶対音量コマンドを再生前にスケールします。
`fast` が true の場合は Z-MUSIC ドライバへデータ転送せず、常駐バッファを直接参照する高速応答モードを使います。

`play_se(track, path, volume = 100, slot = 0)` は効果音ZMDを最大4スロット保持し、Z-MUSIC の `se_play` を呼びます。
SEとして使うZMDは、基本的に短く、テンポ変更を含まないデータにするのが扱いやすいです。
`play_se` は Z-MUSIC の通常トラックへ効果音ZMDを割り込ませるため、テンポはBGM/SEで共通です。
SE側ZMDに `T` / `@T` などのテンポコマンドが含まれると、BGM側のテンポにも影響します。

`play_adpcm(path, pan = 3, frq = 4, priority = 0, slot = 0)` はADPCMデータを最大4スロット保持し、Z-MUSIC の `se_adpcm1` を呼びます。
`frq` のデフォルトは 4 です。
`play_adpcm_note(note, pan = 3, frq = 4, priority = 0)` はZ-MUSIC内に登録済みのADPCM音を `se_adpcm2` で再生します。

`status` は Z-MUSIC の状態テーブルから MIDI / PCM8 / 再生状態などを Hash で返します。

ゲーム用途では、BGMやジングルは `play_bgm`、短いZMD効果音は `play_se`、テンポに依存しない効果音は `play_adpcm` と切り分けると扱いやすくなります。
`maze_chase.rb` では、通常BGM/パワー餌中BGM/ミス時ジングルを `play_bgm`、ドット取得や敵撃破を `play_se` で鳴らしています。

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

## 参考にしたもの

開発にあたって以下のものを参考にさせていただきました。ありがとうございます。

- Human68k v3.02 by SHARP/Hudson
- Z-MUSIC の公開ドキュメント / ファンクションコール仕様
- [ぷにぐらま～ずまにゅある](https://github.com/kg68k/puni) / [libzm2](https://github.com/kg68k/libzm2) by TcbnErik / kg68k 氏
- [micropython-x68k](https://github.com/yunkya2/micropython-x68k) by yunkya2 氏

libzm2 のライセンスについては [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。

## ライセンス

このリポジトリ固有のコードは MIT License です。

mruby 本体も MIT License ですが、このリポジトリには mruby 本体は含めていません。ツールチェーンや外部プロジェクトも同梱していません。詳細は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。
