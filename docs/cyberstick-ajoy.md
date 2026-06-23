# CyberStick / AJOY.X 対応メモ

このメモは、X68000上のmrubyから CyberStick 系のアナログジョイスティックを扱うための実装方針と、AJOY.X 利用時の注意点をまとめたものです。

## 方針

CyberStick のアナログモードは、ジョイスティックポートを直接ポーリングして読むにはタイミングがかなり厳しいです。

mruby側で直接プロトコルを読む実装も試しましたが、実機とエミュレータで挙動差が大きく、Rubyから安定して読むには向きませんでした。
そのため、mruby-x68k では CyberStick の低レベル読み取りを自前実装せず、外部常駐ドライバ AJOY.X を使い、mruby側では AJOY.X の IOCS `$F2` を薄く呼び出します。

これは Z-MUSIC.X を常駐させて `X68k::ZMusic` から呼ぶ構成と同じ考え方です。

## 直接ポーリング調査メモ

AJOY.X を使う方針にする前に、ジョイスティックポートを mruby 側から直接読む実験を行いました。

CyberStick は、SEGA 3B/6B パッドのような単純な TH 切り替えだけではなく、REQ/ACK のようなタイミングを見ながら複数ニブルを受け取る必要があります。
Arduino での CyberStick プロトコル読み取りは、ztto 氏の [usbCyberstick](https://github.com/ztto/usbCyberstick) を参考にしています。
この実装でプロトコルを読めていたため、X68000 側でもポートを直接操作すれば読める可能性はありました。

ただし、mruby から直接扱うには以下の問題がありました。

- Ruby からフェーズごとに制御すると、処理時間と画面更新の影響が大きい。
- C 実装、inline asm 実装の両方を試したが、エミュレータと実機で安定度に差が出た。
- 実機ではある程度データを拾えても、エミュレータではフェーズが進まない、または値が安定しないケースがあった。
- ボタン情報だけ拾える状態と、アナログ軸まで正しく復元できる状態が一致しなかった。
- 読み取り待ちを長くすると安定性は上がるが、Ruby側でゲームループに組み込むには負荷が重い。

調査中は、主に以下のようなデータを見ていました。

```text
raw nibbles
port direct value
control register value
REQ/ACK edge count
phase/index count
decoded x/y/throttle/buttons
```

SEGA 3B/6B パッドは、必要なフェーズだけ読めばゲーム用途でも十分軽く扱えました。
一方 CyberStick のアナログモードは、読み取り自体を常駐ドライバ側に任せたほうが現実的でした。

そのため、mruby-x68k では直接ポーリング実装を公開APIにはせず、AJOY.X 経由の読み取りに整理しています。
直接ポーリングの調査結果は、将来専用ドライバを作る場合の参考情報という位置づけです。


### 直接ポーリング実験コードについて

直接ポーリング実験用のコードは `samples/input/cyber_poll.rb` と `X68k::Joy.cyber_*` 系APIとして残しています。
これは CyberStick のプロトコル調査や、REQ/ACK の挙動確認を目的にしたものです。

ただし、通常のアプリケーションやゲームから使う入力APIとしては推奨しません。
Ruby 側からフェーズ進行を追う方式では、実機とエミュレータでタイミング差が出やすく、描画やゲームループの負荷にも影響されます。

実用上は、AJOY.X を常駐させて `X68k::Ajoy` 経由で読む構成を推奨します。
直接ポーリングコードは、低レベル動作の確認や将来の専用ドライバ実装のための参考実装という位置づけです。


## サンプル

`samples/input/ajoy_chk.rb` は、AJOY.X の常駐確認、読み取り値、ボタン名、スロットル反転などを確認するための基本サンプルです。

`samples/games/cyber_flight.rb` は、AJOY.X から得たスティック、スロットル、トリガー入力を使うワイヤーフレーム風のフライト確認サンプルです。
グラフィック画面は `X68k::Screen.apage` / `X68k::Screen.vpage` でページ切り替えし、テキスト画面には入力値と押下ボタンを表示します。

実行前に AJOY.X を常駐させてください。

```text
AJOY.X
mruby samples/games/cyber_flight.rb
```

## AJOY.X

AJOY.X は以下から入手できます。

<http://retropc.net/x68000/software/hardware/analog/ajoy/>

mruby-x68k には AJOY.X 本体やAJOY.Xのソースは含めていません。
利用する場合は、別途 AJOY.X を入手し、Human68k 上で常駐させてください。

AJOY.X が常駐していない場合、`X68k::Ajoy.available?` は `false` を返します。

## AJOY.X の呼び出し

AJOY.X は IOCS `$F2` を登録します。mruby-x68k はこの IOCS を呼び出して、10バイトの読み取りバッファを受け取ります。

読み取りバッファの内容は以下です。

```text
+0 word  Stick U/D
+2 word  Stick L/R
+4 word  Throttle U/D
+6 word  Option U/D
+8 word  Trigger
```

アナログ値は概ね `0..255` です。

```text
Stick U/D    0=上,   255=下
Stick L/R    0=左,   255=右
Throttle     0=上,   255=下
```

AJOY.X のバッファ上には `Option U/D` もありますが、現在確認している環境では常に `0` のため mruby API の戻り値からは外しています。

センター値は `127` または `128` 付近ですが、実機ではある程度ぶれます。ゲーム等で使う場合はデッドゾーンを設ける前提にしてください。

## mruby API

### 常駐確認

```ruby
X68k::Ajoy.available?
```

AJOY.X が常駐していれば `true` を返します。

### 読み取り

```ruby
raw = X68k::Ajoy.read_raw
val = X68k::Ajoy.read
```

`read_raw` は AJOY.X の読み取り値をそのまま配列で返します。

```ruby
raw[0] # stick U/D
raw[1] # stick L/R
raw[2] # throttle
raw[3] # trigger raw, active-low
```

`read` は通常利用向けの読み取りです。現在は `throttle_reverse` 設定が反映されます。

```ruby
X68k::Ajoy.throttle_reverse = true
val = X68k::Ajoy.read
```

`throttle_reverse` が `true` の場合、スロットル値だけ `255 - raw` に反転されます。
エミュレータ側のスロットル方向が実機と逆になる場合などに使います。

### モードと速度

```ruby
X68k::Ajoy.mode
X68k::Ajoy.mode = 1

X68k::Ajoy.speed
X68k::Ajoy.speed = 0
```

`mode` は AJOY.X の動作モードです。

```text
0 digital
1 analog
```

`speed` は AJOY.X の通信速度設定です。通常は `0` を使います。

## トリガー

AJOY.X の trigger は active-low です。つまり、押されているビットが `0` になります。

mruby-x68k では以下のAPIを用意しています。

```ruby
X68k::Ajoy.trigger_mask
X68k::Ajoy.buttons
```

`trigger_mask` は通常利用向けのボタンマスクです。A+ では A の基本ビットと A+ の追加ビット、B+ では B の基本ビットと B+ の追加ビットが立ちます。

mask のビット配置は以下です。

```text
bit0 a
bit1 b
bit2 c
bit3 d
bit4 e1
bit5 e2
bit6 start
bit7 select
bit8 a_plus
bit9 b_plus
```

`buttons` は現在押されているボタン名を文字列配列で返します。

```ruby
X68k::Ajoy.button_preset = "real"
X68k::Ajoy.buttons
# => ["a"]
# => ["a_plus"]
# => ["start"]
```

## ボタンプリセット

今回の調査結果をもとに、mruby側に以下のプリセットを内包しています。

```ruby
X68k::Ajoy.button_preset = "real"
X68k::Ajoy.button_preset = "usbCyber"
```

### real

実機向けです。A/A+/B/B+ を区別します。

```text
A      -> a
A+     -> a, a_plus
B      -> b
B+     -> b, b_plus
C      -> c
D      -> d
E1     -> e1
E2     -> e2
Start  -> start
Select -> select
```

### usbCyber

USB版CyberStick向けです。エミュレータ側では A/A+、B/B+ が同じ値として見えるため、それに合わせたパターンです。

`samples/input/ajoy_chk.rb` では、`usbCyber` 指定時に `throttle_reverse = true` も設定しています。

## 調査時の raw trigger 値

以下は、今回の確認で得られた raw trigger 値です。
値は AJOY.X が返す active-low の `raw[4]` です。

### USB CyberStick / エミュレータ

```text
Non     0FFF
A       057F
B       0ABF
C       0FDF
D       0FEF
E1      0FF7
E2      0FFB
Start   0FFD
Select  0FFE
```

エミュレータでは、A/A+、B/B+ が同じ値として見えます。これはエミュレータ側の入力割り当てやUSBコントローラ経由の都合によるものと思われます。

### 実機

```text
Non     0FFF
A       0EBF
A+      0BBF
B       0D7F
B+      077F
C       0FDF
D       0FEF
E1      0FF7
E2      0FFB
Start   0FFD
Select  0FFE
```

実機では、本体側トリガー A/B と操縦桿側トリガー A+/B+ が別の複合ビットとして見えます。

## 確認サンプル

```text
mruby samples/input/ajoy_chk.rb
mruby samples/input/ajoy_chk.rb usbCyber
```

`samples/input/ajoy_chk.rb` は、以下を表示します。

```text
stick U/D
stick L/R
throttle
thr raw
trigger raw
mask
buttons
```

`real` は実機向けで A/A+/B/B+ を区別します。
`usbCyber` はUSB版CyberStick向けで、スロットル反転も有効にします。

## 注意点

- AJOY.X は外部常駐ドライバです。mruby-x68k には含めていません。
- AJOY.X が常駐していない状態では `X68k::Ajoy.available?` が `false` になります。
- `read_raw` はAJOY.Xの生値確認用、`read` は通常利用向けです。
- スロットル方向は環境により逆に感じることがあるため、`throttle_reverse` で調整してください。
- アナログ値のセンターはぶれるため、ゲーム側ではデッドゾーンを設けるのが無難です。
