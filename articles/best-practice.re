= ベストプラクティス・バッドプラクティス

はい。筆者がgqlgenとその周辺をやっていった知見をここに吐き出していきます。
この章がこの本のメインであり、ここまでの章はここにたどり着くまでの撒き餌みたいなものです。

== N+1問題

N+1問題が何かから説明していきましょう。
たとえば、技術書典の1イベントあたりサークルが500弱あり、そこに頒布物が複数あるわけです。
これを愚直にDBから取得しようとすると、1+500弱回のクエリが必要になります。
これがN+1問題です。
1+N問題の間違いでは…？

GraphQLでも、何も考えずにコードを書いていくとこの問題が発生します
GraphQLサーバはResolverの集まりですので、イベントを1件resolveし、それに紐づく500弱のサークルをresolveし、さらにそれぞれの頒布物をresolveします。
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
であれば、gqlgen+dataloadenのように処理をバッファする期間を一定時間とするのはある程度妥当であると思われます。

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
この問題については後ほどの節でもう少し詳しく言及します。

//comment{
具体的に、バッチ処理をExecした時、他のgoroutineがExecした直後だとタスクが空であるため、自分が積んだタスクの終了を待たずに制御が返ってきてしまうという問題がありました。
これにより、並列処理させると一部のstructがゼロ値で返ってくることがある、という気がつくまではまったく意味のわからないバグに悩まされました。
読者諸氏も、各Resolverが並行に処理されるということを念頭に正しいコードを書くように心がけましょう。
//}

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
対して、Query Languageの場合、syntax自体が正しければエラー無く返ってくるべきと我々は考えます。
SQLクエリをRDBに投げてみたら"bad request"が返ってきたら、普通ブチギレてそのDBMSを窓から放り投げるでしょう。

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

#@# prh:disable
めんどくさ仕様No.1の話です。
仕様についてざっくりと説明すると…。

 1. @<code>{interface Node { id: ID! \}}インタフェースを作ります
 2. @<code>{node(id: ID!): Node}をQueryの定義に生やします
 3. IDはシステム全体でデータを一意に定められるようにします

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

#@# prh:disable
多くのREST APIベースのシステムではテーブルのPKをIDとして扱い、URLにも組み込んでしまっているでしょう。
たとえば、@<href>{https://techbookfest.org/event/tbf05/circle/33000001}のような感じです。
このURLからIDを切り出し、@<code>{GET /api/circle/33000001}を叩いたりするわけです。
同じことをこの仕様のもとにやろうとした場合、@<code>{/event/tbf05/circle/Q2lyY2xlRXhoaWJpdEluZm86MzMwMDAwMDEK}のようなURL設計にするか、Global IDをアプリ側で合成できる必要があります。
そのため、Global IDの書式をどのようにするか、というのはなかなか意思決定が必要でしょう。
今の所、技術書典では@<code>{CircleExhibitInfo:33000001}を@<code>{base64.RawURLEncoding.EncodeToString}で処理する方式を採用する予定です。

しかし、メ社内でMTC2018@<fn>{mtc2018}を作る際、@<code>{CircleExhibitInfo:33000001}のままでよくね？
humanreadableで何か問題ある？という議論も行われ、現時点で最適解は定まっていません。
筆者的にも別にbase64にする必要はないのでは…？みたいな顔をし始めたところです。

#@# prh:disable
//footnote[mtc2018][@<href>{https://github.com/mercari/mtc2018-web}]

WebページとしてのURL設計については、RESTfulな構造から脱出するのは難しく、また脱出したところで益がありません。
よって、RESTfulなURL設計とGraphQLでのIDの設計についてどう折り合いをつけるか、というのは必ず気にするべきポイントです。
なんなら、Relay Global Object Identificationを捨てる、という選択も視野に入るかもしれません。

やるとわかりますが@<code>{interface Node { id: ID! \}}の実装もクソめんどくさいです。
gqlgen以外だとなんか楽な仕掛けがあったりするんだろうか…。

== GitHub v4 APIから設計を読み取る

ここまでの項目でもさんざん参考にしているので、みなさんもやっていきましょう。
GitHubのAPIはv3までは普通にREST APIで、サーバ側もRailsですので実装に苦闘の痕を伺うことができて面白いです。
歴史的経緯を想像し、あまり見習うべきではないパーツはしっかり避け、よい工夫を汲み取って学習していきましょう。

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
これは我々も見習うべき知見で、データが何個あるか知らんけど無限に返したろｗｗｗみたいな雑な死に方を回避できます。

gqlgenにも同様のコスト計算のための仕組みがあります。
それがcomplexityです@<fn>{gqlgen-complexity}。
complexity自体はgraphql-rubyにある@<fn>{graphql-ruby-complexity}考え方です。

//footnote[gqlgen-complexity][@<href>{https://gqlgen.com/reference/complexity/}]
//footnote[graphql-ruby-complexity][@<href>{http://graphql-ruby.org/queries/complexity_and_depth}]

それでは、試しにコスト計算をしてみます。
@<list>{complexity-query}のクエリについて考えます。
gqlgenのデフォルトの計算だとどれが件数なのかなどは判別してくれないため、ものすごい雑に足し算されてコスト4にされてしまいます。
なので、@<list>{complexity.go}のように件数を考慮する式を自分で定義します。

#@# prh:disable
//list[complexity-query][試しにコスト計算してみる]{
# このクエリのコストは200
query {
  # 10×子（20）で200
  eventList(first: 10) {
    # 10×子（2）で20
    circleList(first: 10) {
      # ↓ field1個で1、2個なので2
      id
      name
    }
  }
}
//}

#@# prh:disable
//list[complexity.go][件数指定のある箇所はそれを子要素のコストにかける]{
#@mapfile(../code/best-practice/complexity-sample/complexity.go)
package complexity_sample

const ComplexityLimit = 200

func NewComplexity() ComplexityRoot {

  f := func(childComplexity int, first *int) int {
    if first == nil {
      // 指定無しはエラーにしたいけどそれはResolver側に任せる
      return 1 + childComplexity
    }

    return *first * childComplexity
  }

  complexityRoot := ComplexityRoot{}
  complexityRoot.Query.EventList = f
  complexityRoot.Event.CircleList = f

  return complexityRoot
}
#@end
//}

めんどくさいですがこの定義を与えてやるとクエリ実行前にいい感じにコスト計算を行ってくれるようになります。

なんぼなんでも手実装はめんどくさいのでもうちょっと楽な仕組みを考えたいところではあります。
struct tagからコスト設定を読むとか、特定のネーミングルールのときにコスト計算を自動化するとかしたいですね。
また、深い箇所のフィールド数を増やすとコストにモロに響くので、DBから一括で取り出せるフィールドについてはコスト計算を甘くしたいなどの欲求があります。
これらは今のgqlgenではサポートされておらず、将来的には僕が頑張るかもしれないけど今のところやる余力がないので先延ばしです！

さらにいろいろ知りたい場合、How to GraphQLの解説@<fn>{hot-to-graphql-security}にはこの話題に限らず秀逸な解説が多いので目を通してみるとよいでしょう。

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

GraphQLには認証周りの仕様が存在していません。
というわけでCookieでセッションを使うなりOAuth2を使うなり好きなものをセレクトしましょう。

個人的にはOAuth2を採用していきたいところです。
モバイルアプリを考えるとCookieはちょっと…。
今のところ、Goで自分が@<kw>{IdP,Identity Provider}になれる楽な方法は模索中です。
仕事の都合で前に検討した時はory/fosite@<fn>{fosite}をベースに実装するのがよさそう…という感じでした。

//footnote[fosite][@<href>{https://github.com/ory/fosite}]

いい感じのソリューションを見つけた人はぜひサンプルコードのあるリポジトリのURLを筆者までお伝え下さい。

== 無理をしない型定義

REST APIで色々と"頑張った"型定義をしていた場合、GraphQL上で辛い目にあうことがあります。
具体的に、技術書典では@<code>{CircleExhibitInfo}と@<code>{CircleExhibitInfoSecret}という2つのKind@<fn>{whats-kind}があります。
1つは公開情報やら何やらをもつKind、もう片方がサークル主の個人情報などをもつKindです。
プログラミングミスで見えてはいけないものが見えてしまう危険性を減らすために分割しています。
さて、REST APIではこの2つをembeddedしたsturctをJSONにして返しています。
つまり、この2つを合体させたJSONが返るわけです。

//footnote[whats-kind][KindはDatastore用語で、ざっくりテーブルと同じ意味だと思ってください]

これに対して、さらにユーザの種類による可視性の制御が入ります。
たとえば、サークルが入力している持ち込み部数は現時点では外部に公開していません。
これは、サークル主とスタッフ権限持ちの人間のみが見ることができます。

さて、GraphQLにこれを落とす時、どうモデリングするのがよいでしょうか？
結論からいうと、Directiveを使って制御しつつ、Kindの構成を素直に反映した構造にするのが正解のようです。

筆者がgqlgenに取り組み始めた時はDirectiveサポートがまだなかったので、色々と試行錯誤しました。
CircleExhibitInfoを可視性毎にCircle、CircleForOwner、CircleForStaffに分割し提供するようにしてみました。
この構成の問題点は前述のRelay Global Object Identificationと組み合わせた時が辛いです。
DB上は同じモノを可視性に応じて別々のIDを振るというトリッキーな設計にする必要があり、狂気を感じます。
単純にハンドリングするコードを書く手間がすごいので普通に心が折れます。

で、心が折れたのでgqlgenがDirectiveをサポートするための手伝いをして、Directiveを使うことにしました。
Directiveであれば、統一的な仕組みでフィールド単位で見せたり見せなかったりを制御しやすいです。

== Directiveを使い倒せ

というわけでDirectiveを使いましょう。
gqlgenでのDirectiveについては@<href>{https://gqlgen.com/reference/directives/}で確認してください。
挙動は実際に自分で試して確かめてください。

gqlgenのDirectiveを実装する関数に、next関数が渡されます。
これを呼ぶと次のResolverを呼びにいってしまいます。
つまり、アクセス制御を行うのであれば基本はnextを呼ぶ前に行うべきです。
特に、Mutationのときにそれをやると副作用が発生した後に処理を行うことになり、意味がありません。
しっかりテストしよう！

Directiveは色々な用途に使うことができますが、筆者の主な用途は可視性制御です。
@<list>{has-role-directive}のように@<code>{@hasRole}というDirectiveを用意しています。
先のCircleExhibitInfoとSecretの例では、現時点ではCircleExhibitInfoに@<code>{secret: CircleExhibitInfoSecret @hasRole(requires: RESOURCE_OWNER)}というフィールドをもたせています。
これにより、サークル主かスタッフしか個人情報系のデータを参照できないようにしているわけです。

//list[has-role-directive][hasRoleで権限制御]{
enum Role {
  PUBLIC
  LOGGED_IN
  RESOURCE_OWNER
  STAFF
}

directive @hasRole(requires: Role) on OBJECT | FIELD_DEFINITION
//}

さて、Directiveを使うことでSchema上のどのフィールドがどういう制御を利用しているかが自動的にドキュメント化されることになります。
これはクエリを書く人にとっても、そのフィールドにアクセスするにはどういう権限が必要かが一目瞭然です。
まさにGraphQLらしい、GraphiQL上で完結する素晴らしい方法…かと思いきや、どういうDirectiveがあるかはInstrospectionでわかりますが、どこでそのDirctiveが使われているかはまったくわかりません@<fn>{directive-instrospection}。
と、いうことに原稿を書いていることに気が付きました。

//footnote[directive-instrospection][@<href>{https://github.com/facebook/graphql/issues/300}]

ふーざーけーるーなー！
ああああああああああああああ！
well documentedな計画があああああああああ！
オポチュニティがたりなくなるうううううう！
折れちゃう！
折れちゃううううううううううう！

対策として、ドキュメントをしっかり書く、権限チェックエラーが発生した時のエラーメッセージをわかりやすく、親切にする、などの対応をしていきましょう。

== カスタムスカラ型の扱い

int64などのカスタムのスカラ型を使う場合、自前でマーシャラを用意します。
JavaScript（JSONではない）の仕様として、64bit幅の整数値を数値として扱うことができません。
なので適当にStringにしてやる必要があります。
それが辛い人は祈ってください@<fn>{js-bigint}。
しらんけど。

//footnote[js-bigint][@<href>{https://github.com/tc39/proposal-bigint}]

gqlgenでの実装を見ていきます（@<list>{custom-scalars/schema.graphql}、@<list>{custom-scalars/custom_scalars.go}、@<list>{custom-scalars/gqlgen.yml}）。
要点は3点、スキーマ上で型を宣言する、変換用Goコードを準備する、宣言とコードを紐づけするための設定を書く、以上！
公式の説明も参照してください@<fn>{gqlgen-custom-scalars}。

//list[custom-scalars/schema.graphql][カスタムのスカラ型を用意する]{
#@mapfile(../code/best-practice/custom-scalars/schema.graphql)
type Query {
  sample: Sample
}

scalar Int64

type Sample {
    id: ID!
    value: Int64
}
#@end
//}

//list[custom-scalars/custom_scalars.go][自分でマーシャラを定義する]{
#@mapfile(../code/best-practice/custom-scalars/custom_scalars.go)
package custom_scalars

import (
  "fmt"
  "io"
  "strconv"

  "github.com/99designs/gqlgen/graphql"
)

// MarshalGraphQLInt64Scalar returns int64
// to GraphQL Scalar value marshaller.
func MarshalGraphQLInt64Scalar(v int64) graphql.Marshaler {
  return graphql.WriterFunc(func(w io.Writer) {
    s := fmt.Sprintf(`"%d"`, v)
    w.Write([]byte(s))
  })
}

// UnmarshalGraphQLInt64Scalar returns int64 value
// from GraphQL value.
func UnmarshalGraphQLInt64Scalar(v interface{}) (int64, error) {
  switch v := v.(type) {
  case string:
    return strconv.ParseInt(v, 10, 64)
  case int64:
    return v, nil
  default:
    return 0, fmt.Errorf("%T is not a int64", v)
  }
}
#@end
//}

//list[custom-scalars/gqlgen.yml][設定でInt64に対して使うマーシャラの指定をする]{
#@# #@mapfile(../code/best-practice/custom-scalars/gqlgen.yml)
schema: schema.graphql
models:
  Int64:
    model: github.com/中略/custom-scalars.GraphQLInt64Scalar
#@# #@end
//}

//footnote[gqlgen-custom-scalars][@<href>{https://gqlgen.com/reference/scalars/}]

細かくカスタマイズの手法が提供されているのはとても嬉しいですね。

== テストの書き方について

テストを書くのがめんどくさい…！
ということで、筆者がよくやるゴールデンテスティングというテスト技法を紹介します。
ある入力Aに対して得られた出力Bを保存しておきます。
そして、次のテストではまた同じ入力Aを与え、得られた出力B'がBと等しいかどうかをチェックします。
こうすることにより、出力が変わっていないことを簡単に確認出来、出力を変えた場合は出力を保存したファイルをすべて消して再出力すれば新しいマスタデータを作成できます。
この手法は何かを入力すると、大量の出力があるような、たとえばコンパイラやGraphQLサーバのような構造に対するテスト技法として便利です。

この技法のよい点は、入力データのパターンを量産するのが簡単で、出力変更時のテストパターンの修正がものすごく簡単なことです。
GraphQLの場合、入力はクエリで、出力はJSONです。
リゾルバの実装を増やすたびに新しいクエリを入力データのパターンに加えます。
後はテストを流すだけで新しいリゾルバの実装が利用された出力例が得られるわけです。

この技法のわるい点は、差分が発生するような変更を入れ、テストデータを再生成したときのdiffを人力で検証しなければいけないことです。
まぁこれは仕方がないですね。
他に、テストの入力となるクエリについて、何を確かめる目的のクエリなのかを書いておかないと後で困る場合があります。

ゴールデンテスティングを実施したときの実例は@<href>{http://bit.ly/2N8RQ4V}にあります。
#@# http://bit.ly/2N8RQ4V → https://github.com/mercari/mtc2018-web/blob/fa73af6c379acfe9be2e1b0e08d22c295cbab1f5/server/gqlapi/resolver_test.go#L19

技術書典の場合、未ログインユーザ、ログインユーザ、リソースオーナー、スタッフの4種類の権限持ちに対して同一のクエリを走らせ、権限制御が正しく行われているかのテストも行っています。

ゴールデンテスティング以外の、いわゆる普通の手法のテストは今の所めんどくさくてあまり実施していません。
Directiveのテストは個別にみっちりと書き、Resolverについての個別のテストはサボっています。

== Code to Schema vs Schema to Code

gqlgenを礼賛する節です。
なぜgqlgenを選ぶべきなのかというと、他によいものがないから、というのがもっとも大きな理由です。

GraphQLを実装するアプローチとして、Schemaからコードを生成するか、コードでSchemaを表現するかの選択肢があります。
どちらを選択するべきかは好みやチーム構造にフィットするかなどの観点がありますが、筆者は前者を選ぶことをお勧めします。
理由はいくつかあります。

1つ目はGraphQLのSchemaを変更するのが誰かわからないこと。
Go言語でSchemaを表現した場合、Schemaを変更できるのはGo言語での開発を行える人のみです。
これはSchemaの変更とResolverの実装タイミングが不可分になるからです@<fn>{job-change}。
GraphQLのSchemaそのものの修正であれば、サーバ側やクライント側のコードの変更タイミングを別にし、独立して追いかけることが可能になるでしょう。

//footnote[job-change][この辺りの考え方はチームメンバーがサーバもフロントもできたTG社から、分業しないと開発が成立しない規模になっているメル社に転職したことによる考え方の変化がありますね。自分のことながら興味深いことです。]

2つ目は、Go言語がDSLとして使うのには十分に柔軟ではないことです。
@<code>{graphql-go/graphql}@<fn>{graphql-go/graphql}などのコードにてSchemaを表現する形式のサンプルコードを見ると、なかなか苦しそうに見えますし、type safeなようにも見えません。
さらに、Go言語は@<code>{go:generate}などのコード生成を便利に活用する文化があり、そのための標準ライブラリも他の多くの言語に比べると圧倒的に充実しています。
よって、Schemaからコードを生成するアプローチを取るべき、となります。
当然、他の言語ではまた別の選択肢があることでしょう。

//footnote[graphql-go/graphql][@<href>{https://github.com/graphql-go/graphql}]

以上の理由により、筆者はSchemaからコードを生成するアプローチであり、もっとも高機能なgqlgen@<fn>{gqlgen-contribute}を選択したわけです。

//footnote[gqlgen-contribute][もっとも僕が目をつけた時点では十分に高機能ではなかったのですが僕がPRを送りまくったりして使い心地をよくしておきました（ドヤ）]

== BFF vs 愚直実装

@<kw>{Backend for Frontend,BFF}と愚直にDBと繋ぎこんだ実装について考えてみます。
BFFのレイヤでREST APIをラップする実装と、APIサーバで直接DBとの繋ぎ込みを行うパターンです。
この構成は今のところモノリシックな構造への対応を念頭においており、いわゆるMicroservice的構成についてはまだ知見がありません。

さて、これについては筆者は既存REST APIに対してラップをするより、直接APIサーバに組み込みDB周りのコードに手を加えたほうがよいと考えています。
これは単に実装の手間の問題で、エラーメッセージの改善やN+1問題への対処などを考えるとREST API側の変更もかなり必要になります。
そう考えるともうAPIサーバに組み込んじゃったほうが手間が少ない…という感じです。
REST APIの実装とDB周りのコードをシェアしてしまったほうが手間が少ないですし、キャッシュなどの構造も共有できたほうが都合がよいです。

N+1問題に対処しない場合、REST APIコールがアホみたいに増えて1個のGraphQLクエリを処理するのに1000回以上のHTTP GETが発生したりしたことがありました。
それでも性能的には2倍程度の酷さで済んだのでAppEngineの強さを実感しましたが不健全であることは間違いありません。

お仕事的にはMicroserviceバリバリなのである程度ナレッジがたまらないといけないところではあります。
この場合、GraphQLサーバとして動作するBFFを組み、裏側のAPIサーバにはBatchGet用のAPIを生やし、通信はGraphQLかgRPCで行う、とするのが良さそうに思えます。

基本方針としてRPCではGetは不要ですべてBatchGet相当で最初から実装しておけ！というのがあるんですがつい忘れがちです。
GetはあってもいいけどBatchGetのラッパにする。
これを守っておけば楽ができたのに…と思うことが多々ありますね。

サーバ間通信はミドルウェアの仕様やツールが充実しているgRPCでいいかと思うんですが、DBの構造とマッチングさせた構成を考える場合サーバ間通信もGraphQLのほうがいいのかも…という気がします。
その場合、GraphQL to GraphQLマッパがあったほうが便利そうですが今のところ便利なツールは存在していません。

というかGoからクエリ投げるのがそもそもめんどくさい！
ついでにいうとinterfaceがフィールド持てないのも辛い。

== お前は本当に並列に対して真剣に取り組んでいるのか？

俺は並列に対して真剣のつもりだったけど全然ガバガバだった…という話です。
筆者は@<href>{https://godoc.org/go.mercari.io/datastore,go.mercari.io/datastore}というライブラリを作って、使っています。
このライブラリはDatastoreに対する操作を一時的に積み立て、バッチ処理化する仕組みがあります。
gqlgenは各Resolverをgoroutineを用いて並列化するため、この仕組と実によくマッチしていて何も考えずにバッチ化処理しておけば複数のResolverからのDBアクセスが1本化されます。

さて問題が発生したのは、バッチ処理化した時の待ち合わせ処理です。
複数のResolverから積まれたバッチ処理を実行します。
このとき、ライブラリの内部的にはロックを取って積まれた処理を取り出し、ロックを解除し、その後に積まれた処理を実行し、それが終わったら処理を返します。

問題は、積まれた処理を取り出せなかったgoroutineです。
ロックを取って何も取り出さずにロックを解除し、積まれた処理はないので即座に処理を抜けます。
このgoroutineが積んでいた処理は終わっていることを期待していますが、実際には別のgoroutineが処理を肩代わりして実行中です。
このせいで処理が終わっていないのでstructの中身が空のままという不可解なバグが発生し、なおかつErrNoSuchEntityが発生した時もまったく別の場所からエラーが出現するのです。

この問題は今では修正され、取り出した処理すべてが完了するまでロックを返さないようにし、エラーのハンドリングもバッチ処理単位に改めることで意図通りの挙動を示すようになりました。
しかし、ここにたどり着くまでのデバッグが本当に大変だったのでもう二度とこの手のバグには出会いたくないです。

まとめ。
今までナァナァで動いてたコードが並列処理がまともに行われるようになったことを契機に牙を向いてくる時があるが頑張って生きてくれ！
健闘を祈る！
