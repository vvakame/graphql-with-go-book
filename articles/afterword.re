= よもやま話

最後にどうでもいい与太話をば…

自分で色々とやってみて、GraphQLサーバはDatabase+Directiveによる権限制御+MutationをうまくこなせればOKらしいという肌感が得られてきました。
これについて考えると、モノリシックな設計で理想的なものを妄想します。
すると、Firebase Realtime DBかFirestoreがGraphQLをサポートし、Directiveの制御やMutation周りをCloud Functionsで実装する…というのがお手軽そうです。
何も考えずにSubscriptionも使えるだろうし…最高やな！
とはいえ、Google先生は今のところGraphQLにまったく関心がない@<fn>{rejoiner}ように見えます。
…かと思いきや、先日行われていたサーベイ@<fn>{firestore-survey}でFirestoreにほしいものある？の一覧にGraphQLがあったのは興奮しました。

//footnote[rejoiner][例外→@<href>{https://github.com/google/rejoiner}]
//footnote[firestore-survey][10/5で終了 @<href>{https://groups.google.com/forum/#!topic/google-cloud-firestore-discuss/8Ub8v1a8o-M}]

この本で触れられていない点として、ツール類の話をしていません。
ので、一瞬ここで話をします。
その1！graphql-schema-linter@<fn>{graphql-schema-linter}！
その2！prettier@<fn>{prettier}！
以上！終了！
必要に応じて探しただけなのでこれら以外に有名なものがあるのかはよく知らないです。
他にいいツールがあったら筆者までお知らせください！

//footnote[graphql-schema-linter][@<href>{http://npmjs.com/package/graphql-schema-linter}]
//footnote[prettier][@<href>{https://www.npmjs.com/package/prettier}]

はい。
あとマイクロサービス上でのGraphQL運用についても今後経験値を積んでいきたいですね。

最後にエディタの話をしたかったんですが特に推したいものがなかったのであった…。
エディタ上で動くPlayground的なもの欲しい気がするんだけどね。
Visual Studio CodeかGoLandのプラグインや拡張でよいものがあったら筆者までお伝えください。
