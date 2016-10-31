# mfcc

Calculate Mel-Frequency Cepstrum Coefficients from .wav file

[![Build Status](https://travis-ci.org/hiroyam/mfcc.svg?branch=master)](https://travis-ci.org/hiroyam/mfcc)

---

#### これは何？

音色の特徴量であるメルケプストラムをwavファイルから計算するコードです。C++によるフルスクラッチです。

#### アルゴリズムの概要

##### データをロードしてノーマライズして
![](https://github.com/hiroyam/mfcc/blob/master/images/resize.png)

##### プリエンファシスフィルタをかける
![](https://github.com/hiroyam/mfcc/blob/master/images/pre_emphasis.png)

##### ハニング窓をかけて
![](https://github.com/hiroyam/mfcc/blob/master/images/window_hanning.png)

##### フーリエ変換して振幅スペクトルを計算する
![](https://github.com/hiroyam/mfcc/blob/master/images/amplitude.png)

##### ナイキスト周波数でカットする
![](https://github.com/hiroyam/mfcc/blob/master/images/nyquist.png)

##### メルフィルタバンクを計算して
![](https://github.com/hiroyam/mfcc/blob/master/images/mel_filter_bank.png)

##### 振幅スペクトルとメルフィルタバンクの相関をとる
![](https://github.com/hiroyam/mfcc/blob/master/images/mel.png)

##### 対数をとって
![](https://github.com/hiroyam/mfcc/blob/master/images/log_spectrum.png)

##### 離散コサイン変換（DCT-II）でケプストラムにする
![](https://github.com/hiroyam/mfcc/blob/master/images/mel_cepstrum.png)

##### リフタリングで定常成分と高次成分を除去してMFCCを得る
![](https://github.com/hiroyam/mfcc/blob/master/images/mfcc.png)
