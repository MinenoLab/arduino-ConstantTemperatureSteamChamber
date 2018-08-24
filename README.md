# arduino-ConstantTemperatureSteamChamber（恒温槽制御プログラム ソースコード）
## 概要
Interface 2018年10月号 特集第3部 第1章 電子機器の天敵「高温・高湿度」簡易試験チェンバの製作のソースコードです。


## 前提条件
開発環境はArduino IDE 1.8.5を使用しターゲットとしてArduino UNOを使用しています。


## 使用しているライブラリ
以下のライブラリを使用していますので取り込んでください。

1. [TimerOne](https://github.com/PaulStoffregen/TimerOne)
2. [Adafruit_MAX31855](https://github.com/adafruit/Adafruit-MAX31855-library)


## マイコンへの書込み
1. プログラムをダウンロードする
    ```
    $ git clone https://github.com/MinenoLab/arduino-ConstantTemperatureSteamChamber.git
    ```
1. ArduinoUNOをPCにUSBケーブルで接続する
1. ArduinoIDEを起動する
1. [ファイル]-[開く] から steam.inoを開く
    - steam.inoはsteamという名前のフォルダの中にある必要がある旨のダイアログが出るので[OK]を押す
1. [スケッチ]-[検証・コンパイル]を実行する
    - エラーが出なければ良い
1. [スケッチ]-[マイコンボードに書き込む]を実行する
    - エラーが出なければ良い
1. Arduino UNOをPCから取り外す


## 使い方
### ログ取得方法
ログを取得する場合はPCにターミナルエミュレータが必要です。
WindowsはTeraTerm、MacやLinuxなどはMinicomがWeb上に情報が多くおすすめです。
シリアルポートの設定パラメータは次のようにします。

|パラメータ|設定値|
|:--------|:-----|
|データ転送速度|9600 bps|
|キャラクタビット長|8|
|パリティチェック|なし|
|ストップビット数|1|
|フロー制御|なし|

### 装置の使い方
Arduinoに電源を投入した時点で制御が開始されます。

