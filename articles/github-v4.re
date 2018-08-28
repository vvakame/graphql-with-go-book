= GraphQLに触れる！GitHub v4 API

さて、GraphQLでサーバを組む前に、GraphQLを使ってデータを取得してくることに慣れてみましょう。
GraphQLではGraphiQL@<fn>{graphiql}という大変出来のよいエクスプローラというかプレイグラウンドというかがあります。
GraphiQLは一瞬typoかと思いがちなんですが、GraphQLとは別のものを指します。

//footnote[graphiql][@<href>{https://github.com/graphql/graphiql}]

GraphQLは仕様として、introspectionというエンドポイントがどういうクエリや型などを持つか、GraphQLを使って問い合わせることができます。
クエリ言語なのに、普通のプログラミング言語でいうリフレクションのような機能を持っているのです。
この機能を使って、GraphiQL上でクエリを書くと入力補完が完備され、定義の参照も行うことができ、ドキュメントもすぐに参照できます。
実際、GraphQLを使ってクエリを試行錯誤する時にAPIの使い方や存在を調べて回ることはREST APIに比べて格段に少ないです。

== GitHub v4 APIとはなんぞや

さて、では早速やってみましょう。
これから遊ぶ場所はGitHub v4 API@<fn>{github-v4}です。
我々エンジニアがもっともよく使うサービスの代表格、GitHubはGraphQLエンドポイントを公開されています。
これを叩いて、色々試してみましょう。

//footnote[github-v4][@<href>{https://developer.github.com/v4/explorer/}]

Sign in with GitHubから認証を済ませたら準備完了です。

== GraphiQL…最高や！

やっていきましょう。
まずは@<list>{query-viewer}のクエリを投げてみます。
帰ってきたデータが、筆者の場合@<list>{query-viewer-result}です。

//list[query-viewer][viewerを調べる]{
#@mapfile(../code/github-v4/query-viewer.graphql)
{
  viewer {
    login
  }
}
#@end
//}

//list[query-viewer-result][クエリの結果]{
#@mapfile(../code/github-v4/query-viewer.graphql.result)
{
  "data": {
    "viewer": {
      "login": "vvakame"
    }
  }
}
#@end
//}

わかりやすいですね。
クエリの構造に対応した形式のJSONが返ってきます。

さて、これだけでは特に面白くもありません。
次にクエリの@<code>{viewer}部分をCommandキーやCtrlキーを押しながらクリックしてみます（@<img>{query-viewer}）。
まるでIDEのように、定義が右側に開いたり、tooltipが表示されどういう型でどういうドキュメントが書かれているかを確認することができます。

//image[query-viewer][ドキュメントが参照できる]{
//}

いい話だ…。
viwerはUser型であることがわかります。
User型にはどういうフィールドがあるのか、クエリには他にどんな要素があるのか、ドンドン進めていけます。
知りたいことがある時に、試行錯誤しながらドキュメントのページと行ったり来たりしなくて済むのはストレスフリーで最高ですね。

== Schemaを眺める

このまま、Schemaを眺めていきましょう。
右側のDocument Explorerですが、さきほどまで色々と定義をジャンプして遊んでいたはずですので、まずは左上の@<code>{＜}マークを押して最上層まで戻ります。

GitHubではOperationとして、QueryとMutationを持っているようです（@<img>{document-explorer}）。
Queryはデータの取得、Mutationはデータの変更、ここにはありませんがSubscribeはデータの監視を行うことができます。

//image[document-explorer][Query, Mutationが見える]{
//}

Queryの内容を見てみて、興味のありそうなものを適当に見て回ってみましょう。
筆者は@<code>{repositoryOwner(login: String!): RepositoryOwner}という引数付きフィールド（@<img>{document-query-repositoryOwner}）に興味を惹かれました。
先程、viewerを見た時にUserという型がありました、ここでの返り値がUser型ではないのはなぜでしょうか。

//image[document-query-repositoryOwner][RepositoryOwnerとは…？]{
//}

中身を見てみると、色々とフィールドがあり、最後にimplementations Organization, Userとなっています。
なるほど、RepositoryOwnerはinterfaceで、それを実装する型としてOrganizationとUserがあるわけです。
確かにGitHubではUserの他にOrganizationがRepositoryの持ち主になる場合もあるのでした。

GitHubも長い歴史があり、途中からGraphQLを導入するためにさまざまな苦労がありそうだと察することができます。
我々もやっていきましょう。

== ドキュメントを元にQueryを書いてみる

TODO

== Goでクライアントを書く

TODO
