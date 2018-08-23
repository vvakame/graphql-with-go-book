= はじめに

本書はGraphQLサーバをGo言語で構築していく本です。
筆者もコミッタになっている@<href>{https://github.com/99designs/gqlgen,github.com/99designs/gqlgen}の利用を前提として解説します。

残念ながら、筆者はまだGraphQLを使ったなんらかのAPIサーバをリリースできていません。
よって、ここで語られる知見は今後アップデートされ、変わっていく可能性があります。

もしあなたが次の範囲の人であれば、ぜひ本書を読み進めていっていただきたいです。

 * GraphQLに興味がある
 ** 特にサーバ側視点の話が知りたい
 ** 罠とかも知りたい
 * Go言語でやっていきたい
 * schemaありきで開発を進めたい
 * ボイラープレートなコードは嫌いです

逆に、次のような話題については深く言及しません。

 * 特定のクライアント側ツールに関する話
 * クライアント側から見た設計のベストプラクティス

//comment{
 * GraphQLとは？
 ** どういう目的に使われるのか
 ** GraphQLは使い物になるのか
 *** クライアントをどう構成するか
 *** ページとコンポーネント、そしてクエリとフラグメント
 ** どういう特性があるのか
 ** 実装の背景はどういうものなのか
 *** Resolverがすべてのベースである という話入れたい
 * GraphQLで遊んでみよう
 ** GitHub v4 APIを触る
 ** GraphiQLの話
 ** Schemaを眺める
 ** Queryを書く
 ** Goでクライアントを書いて適当に投げてみる
 * GraphQLの仕様
 ** GraphQLの仕様の話
 *** 成功と失敗の概念(200 OKとか)
 *** 構文の話
 *** どういう仕組があるのか
 *** どういうトランスポートレイヤを想定しているのか
 *** Query, Mutation, Subscription
 ** GraphQL関連仕様
 *** 主にRelay仕様3種
 ** Relayをなぜ勉強するべきか
 ** FacebookとApollo
 ** ファイルアップロードとか
 * gqlgenを使ってみよう
 ** なぜgqlgenか？
 *** 他ツールとの比較
 ** gqlgenの特徴
 ** gqlgenをインストールする
 ** gqlgen init する
 ** gqlgen gen する
 ** .gqlgen.yml の話
 ** Queryの話
 ** Directiveの話
 ** ErrorPresenter
 ** RequestMiddleware
 ** ResolverMiddleware
 ** BREAKING CHANGEは ま ちょと覚悟しておけ
 ** godocもお察し
 * ベストプラクティス・バッドプラクティス
 ** N+1問題
 *** 実はconcurrentに動いてるので難しい
 ** REST APIからの移行
 *** エラーメッセージの粒度
 *** 処理の一貫性の話（クエリに対して内部で分岐処理してエラー返すとかSQLならやらない）
 ** Global ID的なやつの話
 *** databaseID
 ** GitHub v4 APIから学ぶ
 ** セキュリティ
 ** Auth
 ** 凝ったことはしない DBの1テーブル=1typeくらいの感じで
 *** 可視性とかによってCircle, CircleForOwner, CircleForStaffとかすると @<code>{node(id: ID!)} で詰む
 ** Directiveを使い倒せ
 *** スキーマから読み取れる情報が増えれば増えるほどベネでは仮説
 ** 生成されるコードと実装のコードはpackageを分けるべき？
 ** REST APIはRPCだがGraphQLはクエリである
 *** 返ってくるエラーに対するメンタルモデルが違うという話
 ** Int64とか
 ** テストの書き方について
 ** コードからSchemaとかコードからクエリは辛さの臭いしかしない
 ** BFF vs 愚直実装
 *** BFFした時の裏側のREST APIコール数がエグいことになる的なやつ
 ** お前は本当に並列さについて真摯に取り組んでいるのか？
 *** DatastoreでアレしようとしたらExecを並列に実行した時うまくいかない話
 *** エラーハンドリングについて正しく考える話
 * 運用の話
 ** メトリクス取りたい
 ** ログ取りたい
 ** stats分析したい
 * よもやま
 ** FirestoreがGraphQLサポートしねぇかなぁ
 ** GraphcoolとかprismaとかAmazonなんとかの話
 * 参考文献の紹介
 ** https://www.howtographql.com/advanced/4-security/
//}
