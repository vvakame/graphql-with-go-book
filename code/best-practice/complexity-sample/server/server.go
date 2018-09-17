package main

import (
	"log"
	"net/http"
	"os"

	"github.com/99designs/gqlgen/handler"
	"github.com/vvakame/graphql-with-go-book/code/best-practice/complexity_sample"
)

const defaultPort = "8080"

func main() {
	port := os.Getenv("PORT")
	if port == "" {
		port = defaultPort
	}

	http.Handle("/", handler.Playground("GraphQL playground", "/query"))
	http.Handle("/query", handler.GraphQL(
		complexity_sample.NewExecutableSchema(
			complexity_sample.Config{
				Resolvers:  complexity_sample.NewResolver(),
				Complexity: complexity_sample.NewComplexity(),
			},
		),
		handler.ComplexityLimit(complexity_sample.ComplexityLimit),
	),
	)

	log.Printf("connect to http://localhost:%s/ for GraphQL playground", port)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}
