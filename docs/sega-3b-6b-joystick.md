# SEGA 3B/6Bジョイパッド対応メモ

このメモは、X68000上のmrubyからSEGA Mega Drive/Genesis系の3ボタン/6ボタンパッドを読むための調査内容と実装方針を残すものです。

対象API:

```ruby
X68k::Joy.sega3b(port = 0, wait = 80)
X68k::Joy.sega3b_raw(port = 0, wait = 80, sel_bit = auto)
X68k::Joy.sega6b(port = 0, wait = 80)
X68k::Joy.sega6b_raw(port = 0, wait = 80, sel_bit = auto)
X68k::Joy.sega6b_scan_raw(port = 0, wait = 80, sel_bit = auto)
```

## 前提

SEGA 3B/6Bパッドは、TH/Select信号を切り替えることで同じ入力線から異なるボタン群を読みます。

Arduino等で読む場合は、MDパッドのTH/SelectピンをGPIO出力として直接High/Lowに切り替え、各フェーズでデータ線を読むのが基本です。

X68000の場合もジョイスティックインターフェースに8255互換のポートがあり、Port Cの一部を出力できます。ただし、MDパッドとX68000ジョイスティック端子はピンの意味が完全には一致しません。

## X68000ジョイスティック端子と8255

ジョイスティック資料のブロック図から、X68000側の接続は以下のように読めます。

### ジョイスティック #1

```text
8255 PA0 -> pin 1  F / Up
8255 PA1 -> pin 2  B / Down
8255 PA2 -> pin 3  L / Left
8255 PA3 -> pin 4  R / Right
        -> pin 5  +5V
8255 PA5 -> pin 6  TRG-A
8255 PA6 -> pin 7  TRG-B
8255 PC4 -> pin 8
        -> pin 9  GND
```

### ジョイスティック #2

```text
8255 PB0 -> pin 1  F / Up
8255 PB1 -> pin 2  B / Down
8255 PB2 -> pin 3  L / Left
8255 PB3 -> pin 4  R / Right
        -> pin 5  +5V
8255 PB5 -> pin 6  TRG-A
8255 PB6 -> pin 7  TRG-B
8255 PC5 -> pin 8
        -> pin 9  GND
```

8255のI/Oアドレスは以下です。

```text
$e9a001  Port A: ジョイスティック #1入力
$e9a003  Port B: ジョイスティック #2入力
$e9a005  Port C: ジョイスティック/ADPCM制御
$e9a007  8255 control word / bit set-reset
```

Port Cのbit操作は、`$e9a007`へbit set/reset形式で書き込みます。mruby側では該当MMIOアクセスをsupervisorモードで行います。

## TH/Select線の扱い

MDパッドのTH/Selectは一般的にMD側のpin 7です。一方、X68000のpin 7はTRG-B入力です。

このため、標準的な直結ではX68000側からMDパッドのTHを振れません。

今回動作確認した構成では、変換アダプタ側で以下のようにシフトしています。

```text
MD pin 7  TH/Select
   -> X68000 joystick pin 8
   -> X68000 #1: PC4
   -> X68000 #2: PC5
```

この変換がある場合、X68000側からPC4/PC5をHigh/Lowに切り替えることで、SEGA 3B/6Bパッドを読めます。

```text
port 0: PC4をTH/Selectとして使用
port 1: PC5をTH/Selectとして使用
```

## SEGA 3B/6B読み取りシーケンス

SEGA 3B/6Bでは、THをHigh/Lowに切り替えながら入力を読みます。

基本的な3ボタンページは以下です。

```text
TH=High: Up Down Left Right B C
TH=Low : Up Down 0    0     A Start
```

6ボタンパッドでは、TH切り替えを続けると判定用ページと拡張ボタンページが出ます。

```text
TH=Low : low nibbleが0000になるフェーズが出る
TH=High: Z/Y/X/Mode相当の拡張ページが出る
```

3B専用APIでは、最初の2フェーズだけを読みます。

```text
0 TH=High  base high
1 TH=Low   base low
```

方向キー、A/B/C、Startだけが必要な場合はこの2フェーズで足ります。6B拡張ページへ入らないため、読み取り回数は6Bの9回に対して2回で済みます。TH切り替え後の待ちも2回分だけになるので、Ruby側のポーリングではこちらのほうが軽くなります。

通常の6B APIでは、判定に必要な5フェーズだけを読みます。9フェーズ分を観測したい場合は、診断用の`sega6b_scan_raw`を使います。最後のTH=Lowフェーズはボタン判定に使わないため、どちらのAPIでも読み取り対象から外しています。

通常API:

```text
0 TH=High  base high
1 TH=Low   base low
2 TH=High
3 TH=Low   6B marker
4 TH=High  extension page
```

診断用RAW API:

```text
0 TH=High  base high
1 TH=Low   base low
2 TH=High
3 TH=Low   6B marker
4 TH=High  extension page
5 TH=Low
6 TH=High
7 TH=Low
8 TH=High
```

6B判定は、フェーズ3の下位4bitが0かどうかで見ています。

```c
(values[3] & 0x0f) == 0
```

## 実装API

### X68k::Joy.sega3b

```ruby
mask = X68k::Joy.sega3b(port = 0, wait = 80)
```

SEGA 3B入力を読み、ゲームで扱いやすいビットマスクを返します。

```text
bit0   Up
bit1   Down
bit2   Left
bit3   Right
bit4   A
bit5   B
bit6   C
bit10  Start
```

6B拡張シーケンスには入らないため、`X/Y/Z/Mode`と`6B detected`は返しません。3ボタンパッドとして扱えば十分な場面、または読み取り負荷を少しでも下げたい場面向けです。

### X68k::Joy.sega3b_raw

```ruby
raw = X68k::Joy.sega3b_raw(port = 0, wait = 80, sel_bit = auto)
```

2フェーズ分の生値を配列で返します。

```ruby
raw[0] # TH=High base
raw[1] # TH=Low base
```

3B動作の確認や、変換アダプタ上のTH/Select配線確認に使います。

### X68k::Joy.sega6b

```ruby
mask = X68k::Joy.sega6b(port = 0, wait = 80)
```

SEGA 6B入力を5フェーズ読んで、ゲームで扱いやすいビットマスクを返します。通常のゲーム入力ではこれを使います。

```text
bit0   Up
bit1   Down
bit2   Left
bit3   Right
bit4   A
bit5   B
bit6   C
bit7   X
bit8   Y
bit9   Z
bit10  Start
bit11  Mode
bit15  6B detected
```

`wait`はTH切り替え後の待ちループ回数です。実機およびエミュレータでは`80`で動作確認しています。

### X68k::Joy.sega6b_raw

```ruby
raw = X68k::Joy.sega6b_raw(port = 0, wait = 80, sel_bit = auto)
```

5フェーズ分の生値を配列で返します。

```ruby
raw[0] # TH=High base
raw[1] # TH=Low base
raw[3] # 6B marker
raw[4] # extension page
```

`sel_bit`を省略した場合は、portに応じて自動選択します。

```text
port 0 -> PC4
port 1 -> PC5
```

RAW表示は、実機・エミュレータ・USBアダプタ・変換ケーブルの違いを確認するために残しています。

### X68k::Joy.sega6b_scan_raw

```ruby
raw = X68k::Joy.sega6b_scan_raw(port = 0, wait = 80, sel_bit = auto)
```

9フェーズ分の生値を配列で返します。通常の入力判定では使わず、コントローラや変換アダプタの挙動確認に使います。

## ボタンマップ

実機とRAWビット列で確認した標準マップです。

```text
A     -> base low  bit5
B     -> base high bit5
C     -> base high bit6
X     -> ext high  bit2
Y     -> ext high  bit1
Z     -> ext high  bit0
Start -> base low  bit6
Mode  -> ext high  bit3
```

方向キーはbase highのbit0..3です。

```text
Up    -> base high bit0
Down  -> base high bit1
Left  -> base high bit2
Right -> base high bit3
```

## サンプル

3BのRAWとデコード結果を確認するサンプル:

```text
samples/input/joy_sega3b_test.rb
```

実行例:

```text
mruby samples/input/joy_sega3b_test.rb 0 80
```

引数:

```text
1: port      0 or 1
2: wait      既定値80
```

6B通常読み取りのRAWとデコード結果を確認するサンプル:

```text
samples/input/joy_sega6b_test.rb
```

実行例:

```text
mruby samples/input/joy_sega6b_test.rb 0 80
```

6B診断読み取りのRAWとデコード結果を確認するサンプル:

```text
samples/input/joy_sega6b_scan.rb
```

実行例:

```text
mruby samples/input/joy_sega6b_scan.rb 0 80
```

引数:

```text
1: port      0 or 1
2: wait      既定値80
```

表示内容:

```text
mask: 入力ビットマスク
6B:   6ボタン判定
buttons: デコード結果
raw sequence: 5フェーズまたは9フェーズ分のRAW値
```

## 注意点

- MD pin 7(TH/Select)をX68000 pin 8(PC4/PC5)へ接続する変換が必要です。
- 標準的な直結では、X68000 pin 7は入力のTRG-Bであり、THを振る出力線として使えません。
- 8255/ジョイスティックMMIOはユーザーモードから直接触るとバスエラーになるため、supervisorモードでアクセスします。
- 6Bパッドは読み取りシーケンスに状態があるため、デコード用とRAW表示用に別々に読むと結果がずれることがあります。サンプルではRAWを1回だけ読み、そのRAWから表示用マスクを作っています。
- 通常の6B読み取りは5フェーズで止めます。9フェーズ読み取りは`sega6b_scan_raw`としてRAW観測や相性確認用に残しています。
- エミュレータで実機とボタン配置が異なる場合は、エミュレータ側のジョイパッド割り当てを実機標準マップに合わせます。
- 高速な実機やアクセラレータ環境では`wait`の調整が必要になる可能性があります。ただし確認時点では実機でも`wait=80`で動作しました。
