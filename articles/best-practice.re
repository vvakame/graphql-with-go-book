= ベストプラクティス・バッドプラクティス

TODO

== N+1問題

TODO
実はconcurrentに動いてるので難しい
mercari/datastoreがハマった話

== REST APIからの移行

TODO

=== エラーメッセージの粒度

TODO

=== 処理の一貫性

TODO
クエリに対して内部で分岐処理してエラー返すとかSQLならやらない
RPCとQueryの違い

== Relay Global Object Identification

TODO
https://facebook.github.io/relay/graphql/objectidentification.htm
databaseIDを作った話
クライアント側でURLからIDを切り出したうんぬん

== GitHub v4 APIから設計を読み取る

TODO

== GraphQLのセキュリティ

TODO
まぁcomplexity

== ユーザの認証について

TODO

== 無理をしない型定義

TODO
DBの1テーブル=1typeくらいの感じで
変な工夫すると辛い目にあう
可視性とかによってCircle, CircleForOwner, CircleForStaffとかすると @<code>{node(id: ID!)} で詰む

== Directiveを使い倒せ

TODO

=== スキーマから読み取れる情報を増やす

TODO
多いと幸せである仮説がある

== 生成されるコードと実装のコードはpackageを分けるべき？

TODO
gqlgenでの話

== REST APIはRPCだがGraphQLはクエリである

TODO
返ってくるエラーに対するメンタルモデルが違うという話

== Int64とか

TODO

== テストの書き方について

TODO
golden testing
その他？

== コードからSchemaとかコードからクエリは辛さの臭いしかしない

TODO
見出し変えたいね

== BFF vs 愚直実装

TODO
BFFした時の裏側のREST APIコール数がエグいことになる的なやつ

== お前は本当に並列に対して真剣に取り組んでいるのか？

TODO
DatastoreでアレしようとしたらExecを並列に実行した時うまくいかない話
エラーハンドリングについて正しく考える話
