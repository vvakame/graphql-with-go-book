# graphql-with-go-book

### PDF出力する

rbenvやnodebrewを利用してRubyやNode.jsの準備をしている前提です。
もしそうではない場合、適宜sudoを補うこと。
Node.jsは10.x系以上が必須です。

```
$ gem install bundler
$ git clone git@github.com:vvakame/graphql-with-go-book.git
$ cd graphql-with-go-book
$ npm install
$ npm run pdf
```

### Dockerを使う

TeXの環境構築が困難な場合、一式セットアップ済みの[dockerイメージ](https://registry.hub.docker.com/u/vvakame/review/)を用意してあるので使ってください。
Dockerがうまく動くようになっている場合、以下のコマンドで細かい準備なしにビルドを行うことができます。

```
$ ./build-in-docker.sh
```
