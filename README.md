# mfcc

Calculate Mel-Frequency Cepstrum Coefficients from .wav file

[![Build Status](https://travis-ci.org/hiroyam/mfcc.svg?branch=master)](https://travis-ci.org/hiroyam/mfcc)


#### アルゴリズムの概要

##### データをロードして
![](https://github.com/hiroyam/mfcc/blob/master/images/resize.png)

##### プリエンファシスフィルタをかける
![](https://github.com/hiroyam/mfcc/blob/master/images/pre_emphasis.png)

##### ハニング窓をかけて
![](https://github.com/hiroyam/mfcc/blob/master/images/window_hanning.png)

##### フーリエ変換して振幅スペクトルを計算して
![](https://github.com/hiroyam/mfcc/blob/master/images/amplitude.png)

##### ナイキスト周波数でカットする
![](https://github.com/hiroyam/mfcc/blob/master/images/nyquist.png)

##### メルフィルタバンクを計算して
![](https://github.com/hiroyam/mfcc/blob/master/images/mel_filter_bank.png)

##### 振幅スペクトルとメルフィルタバンクの内積をとる
![](https://github.com/hiroyam/mfcc/blob/master/images/mel.png)

##### 対数を取って
![](https://github.com/hiroyam/mfcc/blob/master/images/log_spectrum.png)

##### 離散コサイン変換でケプストラム領域に移す
![](https://github.com/hiroyam/mfcc/blob/master/images/mel_cepstrum.png)

##### リフタリングで定常成分と高次成分を除去してMFCCを得る
![](https://github.com/hiroyam/mfcc/blob/master/images/mfcc.png)
