package main

import (
	"log"
	"net/http"
	"os"

	"github.com/99designs/gqlgen/handler"
	"github.com/vvakame/graphql-with-go-book/code/best_practice"
)

const defaultPort = "8080"

func main() {
	port := os.Getenv("PORT")
	if port == "" {
		port = defaultPort
	}

	http.Handle("/", handler.Playground("GraphQL playground", "/query"))
	http.Handle("/query", handler.GraphQL(
		best_practice.NewExecutableSchema(
			best_practice.Config{
				Resolvers:  best_practice.NewResolver(),
				Complexity: best_practice.NewComplexity(),
			},
		),
		handler.ComplexityLimit(best_practice.ComplexityLimit),
	),
	)

	log.Printf("connect to http://localhost:%s/ for GraphQL playground", port)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}
