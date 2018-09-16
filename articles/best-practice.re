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
さて、筆者はgqlgenを技術書典Webサイトで動かそうとしていますが、dataloadenを使っていません。
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

めんどくさ仕様No.1の話です。
こいつがどういう仕様かというと、@<code>{interface Node { id: ID! \}}インタフェースを作りましょう。
@<code>{node(id: ID!): Node}をQueryの定義に生やしましょう。
IDはシステム全体でデータを一意に定められるようにしましょう。
というものです。
そうすると、データの更新やらなにやらで便宜をはかってあげますよ、というものらしいです。
仕様自体は@<href>{https://facebook.github.io/relay/graphql/objectidentification.htm}にあります。

さて、こいつのめんどくさいところはご利益がわかりにくいところです。
筆者はまだこの仕様をサポートすることによりどういうメリットが得られるのか体感できていません。
そのくせ、実装がすこぶるめんどくさいので、やる気がでません@<fn>{global-id-pros}。

//footnote[global-id-pros][IDが一意になればMutationやSubscriptionの返り値にしたがって部分木を自動更新… とかはまぁ分かるんだけど…]

IDでデータを一意に定められるようにする、というのが特にめんどくさいポイントです。
Userテーブルがあり、CircleExhibitInfoテーブルがあり、CircleTicketテーブルがあり、コレ以外にもさまざまなテーブルがあります。
これらに含まれるすべてのデータを一意に識別できなければいけないわけで、いわゆるAUTO INCREMENT的なPKの運用をしているとそれ単体ではテーブルが識別できなくて辛い気持ちになります。

GitHubのGraphQLエンドポイントはすでにこの仕様をサポートします。
試しに自分のIDを調べてみると、@<code>{MDQ6VXNlcjEyNTMzMg==}というIDが返ってきます。
これをBase64 decodeしてみると@<code>{04:User125332}が得られます。
さらに、databaseIdというフィールドが別途存在していて、125332という値が入っています。
これはつまり、v4 API経由のUserテーブルの125332というPKのデータ、という意味なのでしょう。

多くのREST APIベースのシステムではテーブルのPKをIDとして扱い、URLにも組み込んでしまっているでしょう。
たとえば、@<href>{https://techbookfest.org/event/tbf05/circle/33000001}のような感じです。
このURLからIDを切り出し、@<code>{GET /api/circle/33000001}を叩いたりするわけです。
同じことをこの仕様のもとにやろうとした場合、@<code>{/event/tbf05/circle/Q2lyY2xlRXhoaWJpdEluZm86MzMwMDAwMDEK}のようなURL設計にするか、Global IDをアプリ側で合成できる必要があります。
そのため、Global IDの書式をどのようにするか、というのはなかなか難しいであると思われます。
今の所、技術書典では@<code>{base64.RawURLEncoding.EncodeToString("CircleExhibitInfo:33000001")}方式を採用する予定です。
しかし、メル社内でMTC2018@<fn>{mtc2018}を作る際、@<code>{"CircleExhibitInfo:33000001"}的な形式でよくね？humanreadableで何か問題ある？という議論も行われ、現時点で最適解は定まっていません。
筆者的にも別にbase64にする必要はないのでは…？みたいな顔をし始めたところです。

//footnote[mtc2018][@<href>{https://github.com/mercari/mtc2018-web}]

WebページとしてのURL設計については、RESTfulな構造から脱出するのは難しく、また脱出したところで益がありません。
よって、RESTfulなURL設計とGraphQLでのIDの設計についてどう折り合いをつけるか、というのは必ず気にするべきポイントです。
なんなら、Relay Global Object Identificationを捨てる、という選択も視野に入るかもしれません。

やるとわかりますが@<code>{interface Node { id: ID! \}}の実装もクソめんどくさいです。
gqlgen以外だとなんか楽な仕掛けがあったりするんだろうか…。

== GitHub v4 APIから設計を読み取る

ここまでの項目でもさんざん参考にしているので、みなさんもやっていきましょう。
GitHubのAPIはv3までは普通にREST APIで、サーバ側もRailsですので実装に苦闘の痕を伺うことができて面白いです。
逆に、あまり見習うべきではないパーツもありますので注意しましょう。

TODO

== GraphQLのセキュリティ

セキュリティ面についても少し触れます。
GraphQLでも通常のREST API的な開発と同様の内容は踏襲していきます。
入力された引数を信じない、プレースホルダ的なものをしっかり使う、などはGraphQLでも普通に気をつけましょう。

GraphQL固有の問題はというと、"柔軟なクエリを書けてしまう"問題があります。
たとえば、過去のイベントを5件取得し、それにぶら下がるサークルを100件取得し、そのサークルが属するイベントを取得し、それにぶらさがる（略）、のような乱暴なクエリを書いたりしていくとあっという間にコストが爆発します。
GraphQLの場合、ツリーの節が増えるごとに掛け算式に取得するデータが増えていきます。
このようなDDoSを意図していなくてもDDoSになりがちなクエリに対して、どのように実行する前に防御するか、というところが肝心です。

GitHubが採用している方式として、Node limitとRate limitがあります。
それぞれざっくり、指定したクエリが得られる最大のツリーのNodeの数、そして1時間あたりに使えるコスト（≒Node limitsの和）の数です。

//footnote[github-limits][@<href>{https://developer.github.com/v4/guides/resource-limitations/}]

これを踏まえて見ると、GitHubのクエリ系は必ず件数のlimitを指定させる設計になっていて、クエリ実行前に最大のNode数が計算できるよう工夫されています。
これは我々も見習うべき知見で、データが何個あるか知らんけど無限に返したろｗｗｗ みたいなことをするとコスト計算ができずにめちゃめちゃ負荷のかかるクエリを実行してしまう可能性があります。

gqlgenにも同様のコスト計算のための仕組みがあります。
それがcomplexityです@<fn>{gqlgen-complexity}。
complexity自体はgraphql-rubyにもある@<fn>{graphql-ruby-complexity}考え方です。

//footnote[gqlgen-complexity][@<href>{https://gqlgen.com/reference/complexity/}]
//footnote[graphql-ruby-complexity][@<href>{http://graphql-ruby.org/queries/complexity_and_depth} gqlgenよりこっちのほうが先]

それでは、試しにコスト計算をしてみます。
@<list>{complexity-query}のクエリについて考えます。
gqlgenのデフォルトの計算だとどれが件数なのかなどは判別してくれないため、ものすごい雑に足し算されてコスト4にされてしまいます。
なので、@<list>{complexity.go}のように件数を考慮する式を自分で定義します。

//list[complexity-query][試しにコスト計算してみる]{
query {
  # このクエリのコストは200
  # 子（20）×10 で 200
  eventList(first: 10) {
    # 子（2）×10 で 20
    circleList(first: 10) {
      # ↓ field1個で1 2個なので2
      id
      name
    }
  }
}
//}

//list[complexity.go][件数指定のある箇所はそれを子要素のコストにかける]{
#@mapfile(../code/best-practice/complexity.go)
package best_practice

const ComplexityLimit = 200

func NewComplexity() ComplexityRoot {
  complexityRoot := ComplexityRoot{}

  complexityRoot.Query.EventList = func(childComplexity int, first *int) int {
    if first == nil {
      // 指定無しはエラーにしたいけどそれはResolver側に任せる
      return 1 + childComplexity
    }

    return *first * childComplexity
  }

  complexityRoot.Event.CircleList = func(childComplexity int, first *int) int {
    if first == nil {
      // 指定無しはエラーにしたいけどそれはResolver側に任せる
      return 1 + childComplexity
    }

    return *first * childComplexity
  }

  return complexityRoot
}
#@end
//}

めんどくさいですがこの定義を与えてやるとクエリ実行前にいい感じにコスト計算を行ってくれるようになります。

流石になんぼなんでもめんどくさいのでもうちょっと楽な仕組みを考えたいところではあります。
struct tagからコスト設定を読むとか特定のネーミングルールのときにコスト計算を自動化するとかしたいですね。
また、深い箇所のフィールド数を増やすとコストにモロに響くので、DBから一括で取り出せるフィールドについてはコスト計算を甘くしたいなどの欲求があります。
これらは今のgqlgenではサポートされておらず、将来的には僕が頑張るかもしれないけど今のところやる余力がないので先延ばしです！

さらにいろいろ知りたい場合、How to GraphQLの解説@<fn>{hot-to-graphql-security}にはセキュリティに限らず秀逸な解説が多いので目を通してみるとよいでしょう。

//footnote[hot-to-graphql-security][@<href>{https://www.howtographql.com/advanced/4-security/}]

//comment{
TODO gqlgenの実装について
 * スタッフの場合無制限に実行させるオプションがほしい
 * クエリのcomplexityをロギングする方法がほしい
 * GitHub方式の計算方法がほしい
   * 手で頑張ればいけそうな気がするが
 * struct tagからコスト読み取るやつほしい
 * Relay Connectionの仕様に則ってそれっぽい計算を自動でやってくれる仕組みほしい
//}

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
