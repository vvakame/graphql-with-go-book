= ベストプラクティス・バッドプラクティス

はい。筆者がgqlgenとその周辺をやっていった知見をここに吐き出していきます。
この章がこの本のメインであり、ここまでの章はここにたどり着くまでの撒き餌みたいなものです。

== N+1問題

N+1問題が何かから説明していきましょう。
たとえば、技術書典のイベント1件につき、サークルが500弱あるわけです。
これを愚直にDBから取得しようとすると、1+500弱回のクエリが必要になります。
これがN+1問題といわれるものです。
1+N問題の間違いでは…？

GraphQLでも、何も考えずにコードを書いていくとこの問題が発生します
GraphQLサーバはResolverの集まりですので、イベントを1件resolveし、そこから派生し500弱のサークルをresolveすることになります。
これはN+1問題そのものです。

これを解決するために、複数のResolverでのクエリを一定数バッファリングし、バッチ化する必要があります。
Goでこれを行う一般的な方法はありませんが、gqlgenの作者であるvektahさんがdataloaden@<fn>{dataloaden}というライブラリを作っています。
これは、FacebookがNode.js向けに作っているDataLoader@<fn>{dataloader}からインスパイアされたものです。
dataloadenでは、特定の種類のデータへのリクエストを一定時間スリープしてバッファリングします。
その一定期間の間に溜まったリクエストをバッチ化し効率化を図ります。

//footnote[dataloaden][@<href>{https://github.com/vektah/dataloaden}]
//footnote[dataloader][@<href>{https://github.com/facebook/dataloader}]

この方法が効率的かといわれるとなかなか難しいのですが、妥当な落とし所ではあるでしょう。
gqlgenでは厄介な（素晴らしい）ことに、Resolverは並行に動作します。
さすがGo言語だぜ！
ここで各Resolverがなるべく並行して動作し続けられるようにすることを考えます。
バッチ処理を実際に行うタイミングをResolverの兄弟Resolverがすべて終わったタイミングとすると、もっとも遅いResolverの処理速度に律速されてしまいます。
とすると、処理をバッファする期間を一定時間とするのはある程度妥当であると思われます。

#@# prh:disable
さて、筆者はgqlgenを技術書典Webサイトで動かそうとしていますが、dataloadenを使っていません。
技術書典WebサイトはAppEngine/Go上で動いています。
もちろんDBはDatastoreです。

Datastoreを操作するためのライブラリとして、go.mercari.io/datastore@<fn>{mercari/datastore}を使っています。
このライブラリにはDatastoreへの各種リクエストをバッチ処理化するためのAPIがデフォルトで準備@<fn>{batch-example}されています。
そして、既存のREST APIも内部ではこのバッチ処理を使っていたため、何も考えずともN+1問題への対策ができていました。

//footnote[mercari/datastore][@<href>{https://godoc.org/go.mercari.io/datastore}]
//footnote[batch-example][@<href>{https://godoc.org/go.mercari.io/datastore#example-package--Batch}]

面白かったトラブルとして、go.mercari.io/datastoreのバッチ処理は何十並列という並行環境に晒されたことがなかったため、バグが潜んでいました。
具体的に、バッチ処理をExecした時、他のgoroutineがExecした直後だとタスクが空であるため、自分が積んだタスクの終了を待たずに制御が返ってきてしまうという問題がありました。
これにより、並列処理させると一部のstructがゼロ値で返ってくることがある、という気がつくまではまったく意味のわからないバグに悩まされました。
読者諸氏も、各Resolverが並行に処理されるということを念頭に正しいコードを書くように心がけましょう。

== REST APIからの移行

すでにREST APIのシステムを抱えているは多いと思います。
技術書典Webも、ucon@<fn>{ucon}というWebフレームワークでSwaggerとセットのREST APIを作成しています。
これに対して、GraphQLのAPIも別途作っていきました。

//footnote[ucon][@<href>{https://github.com/favclip/ucon/}]

この節で言いたいことは、"GraphQLはQuery LanguageであってRemote Procedure Callではない"ということです。
Mutationについては普通にRPCだと思うのでREST APIとさほどの違いは無いと言っていいでしょう。

=== エラーメッセージの粒度

RPCとQuery Languageでは、返すべきエラーメッセージの粒度が異なります。
たとえば、APIを叩いて、素っ気ない"bad request"の文字が返ってきたとします。
RPCであれば、ドキュメントを調べたり、ソースコードを読んで理由を確かめる作業を耐えることができました。
しかし、不思議なことにQuery Languageではこの粒度のエラーメッセージが返ってくると耐え難い苦痛を感じます。

これについてよくよく考えると、RPCというのは関数であり、個々の関数の仕様を把握し正しく呼び出すのは呼び出し側の責務であるという意識があるためでしょう。
対して、Query Languageの場合、syntax自体が正しければエラー無く返ってくるべきと我々は思います。
SQLをRDBに投げてみたら"bad request"が返ってきたら、普通ブチギレてそのDBMSに悪態をつくでしょう。

よって、GraphQLではエラーを返す場合、そのクエリのどこが悪かったのか、明快に分かるようなエラーメッセージを返すべきです。

=== Query Languageとエラー

1つ前の項目で述べたように、REST APIはRPCでGraphQLはQuery Languageです。
ということで、GraphQLでは可能な限りvalidなsyntaxを書いたらちゃんとエラーではないレスポンスが返ってくる、という状況を作る必要があります。

実際に、GitHubのGraphQLエンドポイントを叩いてみて、rate limitやnode limit以外のエラーが返ってきたことは無いように思います。
そして、rate limitやnode limitの場合も、詳細なエラーメッセージが返ってきます。
かしこい！
我々もこれに倣っていかねばなりません。

REST APIからの移行の場合、特定の検索条件の組み合わせはエラーにしなければならない、というパターンもあるでしょう。
その場合、詳細なエラーメッセージを出力し、GraphiQLの外に出ずにデバッグできるようにします。
GraphiQLの外のドキュメントを見に行かねばならない場合、GraphQLの嬉しさがかなり減じてしまいます。
クエリを書く人が自走しやすくするためには、1にエラーメッセージ、2にSchemaに書かれたドキュメントの充実を心がけるとよいでしょう。

技術書典Webでも、エラーハンドラ周りを改修し、詳細なエラーメッセージを簡単に出力できるように整備しました。

エラーメッセージの他に、Directiveを使って制御を行う手法があります。
これについてはこの後の項目で触れます。

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
